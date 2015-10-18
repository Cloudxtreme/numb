//
// C++ Implementation: httpserver
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <ctype.h>
#include <assert.h>

#include "httpserver.h"

HttpServer::HttpServer(HttpContent *_httpContent, int _maxConnectionsAuthorized, int _readTimeout, int _writeTimeout, int _shapping, char _burst, int *_httpSlot, Mutex *_slotMutex, char *_aesKey, int aesKeySize, char *_aesVHost, char *_noByteRange, unsigned short listeningPort) : Server(SOCK_STREAM, IPPROTO_IP, listeningPort, _maxConnectionsAuthorized) {
	httpContent = _httpContent;
	maxConnectionsAuthorized = _maxConnectionsAuthorized;
	httpContextList.setDestroyData(2);
	kQueue = 0;

	// Create a kqueue descriptor for receiving kevents
	kQueue = kqueue();

	// Timeout per Kqueue HTTP connection in ms
	readTimeout = _readTimeout;
	writeTimeout = _writeTimeout;
	shapping = _shapping;
	burst = _burst;

	//httpSessionsIndex = new HttpSession *[maxConnectionsAuthorized+1];
	//for (int i = 0; i < (maxConnectionsAuthorized*2); i++)
	//	httpSessionsIndex[i] = new HttpSession();

	aesEnabled = false;
	aesKey = NULL;
	if (_aesKey) {
		if (aesKeySize == 32) {
		aesKey = (char *)malloc(aesKeySize);
		memcpy(aesKey, _aesKey, aesKeySize);
		}
		else
			systemLog->sysLog(ERROR, "invalid size '%d' for AES key, ignored\n", strlen(_aesKey));
	}

	aesVHost = NULL;
	if (_aesVHost) {
		int len = strlen(_aesVHost);
		aesVHost = (char *)malloc(len + 1);
		memcpy(aesVHost, _aesVHost, len);
	}
	
	noByteRange = NULL;
	if (_noByteRange) {
		noByteRange = strdup(_noByteRange);
	}	

	if (aesVHost && aesKey)
		aesEnabled = true;

	return;
}

HttpServer::~HttpServer() {
	for (int i = 0; i < maxConnectionsAuthorized; i++)
		if (httpSessionsIndex[i])
			delete httpSessionsIndex[i];

	if (aesKey)
		free(aesKey);

	if (aesVHost)
		free(aesVHost);
	
	if (noByteRange)
		free(noByteRange);

	close(kQueue);

	return;
}

int HttpServer::setKqueueEvents(void) {
	struct kevent kChange;

	EV_SET(&kChange, sd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, 0);
	kevent(kQueue, &kChange, 1, NULL, 0, NULL);

	return 0;
}

int HttpServer::sendChunk(HttpSession *httpSession, char *buffer, int bufferSize) {
	ssize_t bytesSent;

	bytesSent = send(httpSession->httpExchange->outputDescriptor, buffer, bufferSize, 0);
	if (bytesSent < 0) {
		if ((errno != EAGAIN) && (errno != EINTR)) {
			systemLog->sysLog(ERROR, "[%d] cannot send on socket: %s", httpSession->httpExchange->outputDescriptor, strerror(errno));
			return -1;
		}
	}
	else {
		//httpSession->fileSize -= bufferSize;
		httpSession->fileOffset += bytesSent;
	}

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "httpSession->fileOffset is %d, httpSession->fileSize is %d", httpSession->fileOffset, httpSession->fileSize);
#endif

	if (httpSession->fileOffset >= httpSession->fileSize) 
		httpSession->endOfAnswer = true;

	return 0;
}

int HttpServer::sendChunk(HttpSession *httpSession) {
	off_t bytesSent;
	size_t sizeToSend;
	int returnCode = 0;
	
	if ((httpSession->fileSize - httpSession->fileOffset) < sendLoWat)
		sizeToSend = (httpSession->fileSize - httpSession->fileOffset);
	else
		sizeToSend = sendLoWat;

	returnCode = sendfile(httpSession->httpExchange->inputDescriptor, httpSession->httpExchange->outputDescriptor, httpSession->fileOffset, sizeToSend, NULL, &bytesSent, 0);

#ifdef STDOUTDEBUG
	printf("bytesSent: %u, returnCode = %d, errno = %d\n", bytesSent, returnCode, errno);
#endif

	if (returnCode < 0) {
		if ((errno == EBUSY) && (! bytesSent)) {
			return -2;
		}
		if ((errno != EAGAIN) && (errno != EINTR)) {
			systemLog->sysLog(ERROR, "[%d] (%d) cannot sendfile on socket/file descriptor: %s", httpSession->httpExchange->outputDescriptor, httpSession->httpExchange->inputDescriptor, strerror(errno));
			return -1;
		}
	}
	//httpSession->fileSize -= bytesSent;
	httpSession->fileOffset += bytesSent;

	//systemLog->sysLog(ERROR, "[%d] (%d) bytesToRead %d fileSize %d mmapedFile %X", clientSocket, fileDescriptor, bytesToRead, fileSize, mmapedFile);

#ifdef STDOUTDEBUG
	printf("fileSize is %d\n", httpSession->fileSize);
#endif

	if (httpSession->fileOffset >= httpSession->fileSize)
		httpSession->endOfAnswer = true;

	return 0;
}

void HttpServer::initHeader(HttpSession *httpSession, const char *additionalHeader) {
	const char *jpegMimeType = "image/jpeg";
	const char *pngMimeType = "image/png";
	const char *flvMimeType = "video/x-flv";
	const char *mp4MimeType = "video/mp4";
	const char *xmlMimeType = "text/xml";
	const char *wmvMimeType = "video/x-ms-wmv";
	const char *oggMimeType = "video/ogg";
	const char *webmMimeType = "video/webm";
	//const char *htmlMimeType = "text/html";
	const char *defaultMimeType = "application/octet-stream";
	const char *connectionClose = "Connection: close";
	const char *connectionKeepAlive = "Connection: Keep-Alive";
	const char *connectionHeader = connectionKeepAlive;
	const char *http404NotFound = "<html><body><b><h2>HTTP/1.1 404 Not Found</h2><br/>The object cannot be found, snif :~(</body></html>";
	const char *http400BadRequest = "<html><body><b><h2>HTTP/1.1 400 Bad Request</h2><br/>The HTTP request is invalid or not recognized</body></html>";
	const char *http403Forbidden = "<html><body><b><h2>HTTP/1.1 403 Forbidden</h2><br/>You don't have permission to access</b>";
	const char *http500InternalServerError = "<html><body><b><h2>HTTP/1.1 500 Internal Server Error</h2><br/>Oh Oh, I have some difficulties to complete your request</b>";
	const char *mimeType = flvMimeType;
	struct tm *localTime;
	struct tm *localTimeLastModified;
	struct tm *expiresTime;
	time_t currentTime;
	time_t expireTime;
	char localTimeFormatted[128];
	char localTimeLastModifiedFormatted[128];
	char expiresTimeFormatted[128];
	
	if ((httpSession->httpCode == 200) && (httpSession->fileSize <= 0) &&
(httpSession->noDataToSend == false)) {
		httpSession->httpCode = 500;
		httpSession->noDataToSend = true;
	}

	if (strstr(httpSession->httpFullRequest, ".jpg")) {
		mimeType = jpegMimeType;
		httpSession->mimeType = 0;
	}
	else
	if (strstr(httpSession->httpFullRequest, ".png")) {
		mimeType = pngMimeType;
		httpSession->mimeType = 1;
	}
	else
	if (strstr(httpSession->httpFullRequest, ".xml")) {
		mimeType = xmlMimeType;
		httpSession->mimeType = 2;
	}
	else
	if (strstr(httpSession->httpFullRequest, ".flv")) {
		mimeType = flvMimeType;
		httpSession->keepAliveConnection = false;
		httpSession->mimeType = 3;
	}
	else
	if (strstr(httpSession->httpFullRequest, ".mp4")) {
		mimeType = mp4MimeType;
		httpSession->keepAliveConnection = false;
		httpSession->mimeType = 4;
	}
	else
	if (strstr(httpSession->httpFullRequest, ".wmv")) {
		mimeType = wmvMimeType;
		httpSession->keepAliveConnection = false;
		httpSession->mimeType = 5;
	}
	else
	if (strstr(httpSession->httpFullRequest, ".ogg")) {
		mimeType = oggMimeType;
		httpSession->keepAliveConnection = false;
		httpSession->mimeType = 6;
	}
	else
	if (strstr(httpSession->httpFullRequest, ".webm")) {
		mimeType = webmMimeType;
		httpSession->keepAliveConnection = false;
		httpSession->mimeType = 7;
	}
	else {
		mimeType = defaultMimeType;
		httpSession->keepAliveConnection = false;
		httpSession->mimeType = -1;
	}
	if (httpSession->downloadMimeType) {
		mimeType = defaultMimeType;
	}

	if (httpSession->keepAliveConnection == false)
		connectionHeader = connectionClose;

	currentTime = time(NULL);
	expireTime = currentTime + 86400;
	expiresTime = gmtime(&expireTime);
	strftime(expiresTimeFormatted, sizeof(expiresTimeFormatted), "%a, %d %b %Y %T GMT", expiresTime);
	localTime = gmtime(&currentTime);
	strftime(localTimeFormatted, sizeof(localTimeFormatted), "%a, %d %b %Y %T GMT", localTime);
	switch (httpSession->httpCode) {
		case 200:
			localTimeLastModified = gmtime(&httpSession->sourceFileStat.st_mtime);
			strftime(localTimeLastModifiedFormatted, sizeof(localTimeLastModifiedFormatted), "%a, %d %b %Y %T GMT", localTimeLastModified);
			snprintf(httpSession->httpHeader, sizeof(httpSession->httpHeader), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nExpires: %s\r\nContent-Length: %u\r\n%s\r\nDate: %s\r\nServer: numb/2.0\r\n%s\r\n\r\n", mimeType, localTimeLastModifiedFormatted, expiresTimeFormatted, httpSession->fileSize, connectionHeader, localTimeFormatted, additionalHeader);
			
			break;
		case 206:
			localTimeLastModified = gmtime(&httpSession->sourceFileStat.st_mtime);
			strftime(localTimeLastModifiedFormatted, sizeof(localTimeLastModifiedFormatted), "%a, %d %b %Y %T GMT", localTimeLastModified);
			snprintf(httpSession->httpHeader, sizeof(httpSession->httpHeader), "HTTP/1.1 %d Partial Content\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nContent-Range: bytes %d-%d/%d\r\nContent-Length: %u\r\n%s\r\nDate: %s\r\nServer: numb/2.0\r\n%s\r\n\r\n", httpSession->httpCode, localTimeLastModifiedFormatted, httpSession->byteRange.start, httpSession->byteRange.end, (int)httpSession->sourceFileStat.st_size, httpSession->fileSize, connectionHeader, localTimeFormatted, additionalHeader);

			break;
		case 301:
			localTimeLastModified = gmtime(&httpSession->sourceFileStat.st_mtime);
			strftime(localTimeLastModifiedFormatted, sizeof(localTimeLastModifiedFormatted), "%a, %d %b %Y %T GMT", localTimeLastModified);
			snprintf(httpSession->httpHeader, sizeof(httpSession->httpHeader), "HTTP/1.1 301 Moved Permanently\r\nConnection: close\r\nLocation: http://%s\r\nContent-Length: 0\r\nDate: %s\r\nServer: numb/2.0\r\n\r\n", httpSession->redirectUrl, localTimeFormatted);
			break;
		case 304:
			snprintf(httpSession->httpHeader, sizeof(httpSession->httpHeader), "HTTP/1.1 304 Not Modified\r\nDate: %s\r\nServer: numb/2.0\r\n%s\r\n\r\n", localTimeFormatted, connectionHeader);
			break;
		case 400:
			snprintf(httpSession->httpHeader, sizeof(httpSession->httpHeader), "HTTP/1.1 400 Bad Request\r\nDate: %s\r\nServer: numb/2.0\r\nContent-Length: %u\r\nContent-Type: text/html\r\n%s\r\n\r\n%s", localTimeFormatted, (unsigned int)strlen(http400BadRequest), connectionHeader, http400BadRequest);
			break;
		case 403:
			// Forbidden
			snprintf(httpSession->httpHeader, sizeof(httpSession->httpHeader), "HTTP/1.1 403 Forbidden\r\nDate: %s\r\nServer: numb/2.0\r\nContent-Length: %u\r\nContent-Type: text/html\r\n%s\r\n\r\n%s", localTimeFormatted, (unsigned int)strlen(http403Forbidden), connectionHeader, http403Forbidden);
			break;
		case 404:
			// XXX Implement 404 Not Found with default file sent...
			snprintf(httpSession->httpHeader, sizeof(httpSession->httpHeader), "HTTP/1.1 404 Not Found\r\nDate: %s\r\nServer: numb/2.0\r\nContent-Length: %u\r\nContent-Type: text/html\r\n%s\r\n\r\n%s", localTimeFormatted, (unsigned int)strlen(http404NotFound), connectionHeader, http404NotFound);
			break;
		case 500:
		default:
			// XXX Internal Server Error correctly
			snprintf(httpSession->httpHeader, sizeof(httpSession->httpHeader), "HTTP/1.1 500 Internal Server Error\r\nDate: %s\r\nServer: numb/2.0\r\nContent-Length: %u\r\nContent-Type: text/html\r\n%s\r\n\r\n%s", localTimeFormatted, (unsigned int)strlen(http500InternalServerError), connectionHeader, http500InternalServerError);
			httpSession->endOfAnswer = true;
			break;
	}

#ifdef DEBUGOUTPUT
	fprintf(stderr, "[=== Header ===]\n%s", httpSession->httpHeader);
#endif

	return;
}

int HttpServer::sendHeader(HttpSession *httpSession) {
	ssize_t bytesSent;

	bytesSent = send(httpSession->httpExchange->outputDescriptor, httpSession->httpHeader, strlen(httpSession->httpHeader), 0);
	if ((bytesSent < 0) && ((errno != EWOULDBLOCK) && (errno != EAGAIN))) {
		systemLog->sysLog(ERROR, "[%d] cannot send HTTP header: %s", httpSession->httpExchange->outputDescriptor, strerror(errno));
		return -1;
	}

	return 0;
}

int HttpServer::readRequest(HttpSession *httpSession) {
	if (! httpSession->httpExchange) {
		return -1;
	}
	httpSession->smsg = recvMessage(httpSession->httpExchange->outputDescriptor);
	if (httpSession->smsg) {
		if ((httpSession->requestSize + httpSession->smsg->brecv) >= MAXHTTPREQUESTSIZE) {
			systemLog->sysLog(ERROR, "[ %d ] Request is larger than %d bytes, closing connection", httpSession->httpExchange->outputDescriptor, MAXHTTPREQUESTSIZE);
			free(httpSession->smsg);
			httpSession->smsg = NULL;
			return -1;
		}
		strncat(httpSession->httpFullRequest, httpSession->smsg->recvmsg, MAXHTTPREQUESTSIZE - httpSession->requestSize);
		httpSession->requestSize += httpSession->smsg->brecv;
		free(httpSession->smsg);
		httpSession->smsg = NULL;
#ifdef DEBUGOUTPUT
		fprintf(stderr, "[DEBUG] httpSession smsg is %s\n", httpSession->httpFullRequest);
#endif
		if (((httpSession->requestSize >= 4) && (! strcmp(&httpSession->httpFullRequest[httpSession->requestSize - 4], "\r\n\r\n"))) || ((httpSession->requestSize >= 2) && (! strcmp(&httpSession->httpFullRequest[httpSession->requestSize - 2], "\n\n")))) {
			httpSession->endOfRequest = true;
			return verifyQuery(httpSession);
		}
	}
	else
		if ((errno == EINTR) || (errno == EWOULDBLOCK))
			return 0;
		else {
			return -1;
		}

	return 0;
}

int HttpServer::decodeBase64(char *text, char **textDecoded, int *textDecodedLength) {
	BIO *b64, *bio, *bmem;
	char outBuffer[2048];

	if (! text) {
		systemLog->sysLog(ERROR, "cannot base64 decode a null pointer text");
		return NULL;
	}

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_new_mem_buf(text, strlen(text));
	bio = BIO_push(b64, bmem);
	*textDecodedLength = BIO_read(b64, outBuffer, 2048);

	if (! *textDecodedLength)
		return -1; 

	*textDecoded = (char *)malloc(*textDecodedLength + 1);
	if (! *textDecoded) {
		systemLog->sysLog(CRITICAL, "cannot create returnBuffer object: %m", strerror(errno));
		BIO_free_all(bio);
		return -1;
	}
	memcpy(*textDecoded, outBuffer, *textDecodedLength);

	BIO_free_all(bio);

	return 0;
}

void HttpServer::initAES(EVP_CIPHER_CTX *context, unsigned char *iv, unsigned char *key) {
	EVP_CIPHER_CTX_init(context);
	EVP_CIPHER_CTX_set_padding(context, 0);
	EVP_DecryptInit_ex(context, EVP_aes_256_cbc(), NULL, key, iv);

	return;
}

char *HttpServer::decryptAES(EVP_CIPHER_CTX *context, char *cipherText, int cipherLength) {
	int bufferSize = MAXHTTPREQUESTSIZE - 1;
	unsigned char outBuffer[MAXHTTPREQUESTSIZE];
	char *plainText;

	//EVP_DecryptInit_ex(context, NULL, NULL, NULL, NULL);
	EVP_DecryptUpdate(context, outBuffer, &bufferSize, (unsigned char *)cipherText, cipherLength);
	EVP_DecryptFinal_ex(context, outBuffer, &bufferSize);

	bufferSize = strlen((char *)outBuffer);

	plainText = (char *)malloc(bufferSize + 1);
	memcpy(plainText, outBuffer, bufferSize);
	plainText[bufferSize] = '\0';

	return plainText;
}

void HttpServer::freeAES(EVP_CIPHER_CTX *context) {
	EVP_CIPHER_CTX_cleanup(context);

	return;
}

int HttpServer::verifyQuery(HttpSession *httpSession) {
	int i = 0;
	int j;
	int minRequestSize = 0;
 	char saveChar;
	char *stringPtr;

#ifdef DEBUGOUTPUT
	fprintf(stderr, "[=== VERIFY QUERY ===]\n%s", httpSession->httpFullRequest);
	fprintf(stderr, "\tRequestSize is %d\n", httpSession->requestSize);
	fprintf(stderr, "\tnoDataToSend is %d\n", httpSession->noDataToSend);
#endif
  
  if (! strncmp(httpSession->httpFullRequest, "GET http://", 11)) {
		stringPtr = httpSession->httpFullRequest;
    stringPtr += 11;
    char * stringPtr2 = strcasestr(stringPtr, "/");
    unsigned int len = stringPtr2 - stringPtr;
    strncpy(httpSession->virtualHost, httpSession->httpFullRequest + 11, len);
    memcpy(httpSession->httpFullRequest + 4, httpSession->httpFullRequest + 11 + len, strlen(httpSession->httpFullRequest) - len - 7);      
  }
	if (! strncmp(httpSession->httpFullRequest, "GET /", 5)) {
		httpSession->httpRequestType = 1;
		minRequestSize = 16;
		i = 5;
	}
	if (! strncmp(httpSession->httpFullRequest, "HEAD /", 6)) {
		httpSession->httpRequestType = 2;
		httpSession->noDataToSend = true;
		minRequestSize = 17;
		i = 6;
	}

	if (httpSession->requestSize >= minRequestSize) {
		httpSession->httpCode = 200;
		j = i;
		while ((httpSession->httpFullRequest[j] != ' ') && httpSession->httpFullRequest[j]) j++;
		saveChar = httpSession->httpFullRequest[j];
		httpSession->httpFullRequest[j] = '\0';
		strncpy(httpSession->httpRequest, &httpSession->httpFullRequest[i], sizeof(httpSession->httpRequest)-1);
		httpSession->httpRequest[sizeof(httpSession->httpRequest)-1] = '\0';
		httpSession->httpFullRequest[j] = saveChar;
		stringPtr = strcasestr(httpSession->httpFullRequest, "host: ");
		if (stringPtr) {
			j = 0;
			while ((stringPtr[j] != '\r') && (stringPtr[j] != '\n') && stringPtr[j]) j++;
			saveChar = stringPtr[j];
			stringPtr[j] = '\0';
			strncpy(httpSession->virtualHost, &stringPtr[6], sizeof(httpSession->virtualHost)-1);
			httpSession->virtualHost[sizeof(httpSession->virtualHost)-1] = '\0';
			stringPtr[j] = saveChar;
		}
		else if (strcmp(httpSession->virtualHost, "") == 0) {
			strncpy(httpSession->virtualHost, "none", sizeof(httpSession->virtualHost)-1);
			httpSession->virtualHost[sizeof(httpSession->virtualHost)-1] = '\0';
		}
		stringPtr = strcasestr(httpSession->httpFullRequest, "referer: ");
		if (stringPtr) {
			j = 0;
			while ((stringPtr[j] != '\r') && (stringPtr[j] != '\n') && stringPtr[j]) j++;
			saveChar = stringPtr[j];
			stringPtr[j] = '\0';
			strncpy(httpSession->referer, &stringPtr[9], sizeof(httpSession->referer)-1);
			httpSession->referer[sizeof(httpSession->referer)-1] = '\0';
			stringPtr[j] = saveChar;
		}
		else {
			httpSession->referer[0] = '-';
			httpSession->referer[1] = '\0';
		}
		stringPtr = strcasestr(httpSession->httpFullRequest, "user-agent: ");
		if (stringPtr) {
			j = 0;
			while ((stringPtr[j] != '\r') && (stringPtr[j] != '\n') && stringPtr[j]) j++;
			saveChar = stringPtr[j];
			stringPtr[j] = '\0';
			strncpy(httpSession->userAgent, &stringPtr[12], sizeof(httpSession->userAgent)-1);
			httpSession->userAgent[sizeof(httpSession->userAgent)-1] = '\0';
			stringPtr[j] = saveChar;
		}
		else {
			strncpy(httpSession->userAgent, "none", sizeof(httpSession->userAgent)-1);
			httpSession->userAgent[sizeof(httpSession->userAgent)-1] = '\0';
		}
		stringPtr = strcasestr(httpSession->httpFullRequest, "range: bytes=");
		if (stringPtr) {
			j = 0;
			while (stringPtr[j] && (stringPtr[j] != '-')) j++;
			if (stringPtr[j]) {
				saveChar = stringPtr[j];
				stringPtr[j] = '\0';
				httpSession->byteRange.start = atoi(&stringPtr[13]);
				stringPtr[j] = saveChar;
				if (isdigit(stringPtr[j + 1]))
				  httpSession->byteRange.end = atoi(&stringPtr[j + 1]);
				else
				  httpSession->byteRange.end = -1;
				httpSession->seekPosition = httpSession->byteRange.start;
			}
		}
		else {
			if (! strcmp(noByteRange, httpSession->virtualHost)) {
#ifdef DEBUGOUTPUT
				fprintf(stderr, "VirtualHost %s is not authorized to do request without ByteRange\n", httpSession->virtualHost);
				fprintf(stderr, "noByteRange argument is: %s\n", noByteRange);
#endif
				httpSession->httpCode = 403;
				httpSession->noDataToSend = true;
			}
		}
		if (strcasestr(httpSession->httpFullRequest, "connection: keep-alive")) {
			httpSession->keepAliveConnection = true;
		}
		stringPtr = strcasestr(httpSession->httpFullRequest, "x-forwarded-for:");
		if (stringPtr) {
			j=16;
			while (stringPtr[j] && (stringPtr[j] == ' ')) j++;
			if (stringPtr[j]) {
				int k;
				k = 0;
				while (stringPtr[k] && (stringPtr[k] != '\n') && (stringPtr[k] != '\r')) k++;
				stringPtr[k] = '\0';
				memcpy(httpSession->ipSource, &stringPtr[j], strlen(&stringPtr[j]));
				httpSession->ipSource[strlen(&stringPtr[j])] = '\0';
			}
                }
		//if (strcasestr(httpSession->httpFullRequest, "if-modified-since")) {
		//	httpSession->httpCode = 304;
		//	httpSession->noDataToSend = true;
		//}

		// Ok now if this is a secured stream (Eg: base64 + AES) we decode it
		if ((aesEnabled == true) && strstr(httpSession->virtualHost, aesVHost)) {
			char *httpRelativeDecoded;
			int httpRelativeDecodedLength;
			int returnCode;

			returnCode = decodeBase64(httpSession->httpRequest, &httpRelativeDecoded, &httpRelativeDecodedLength);

			// Now decode AES
			if (! returnCode) {
				unsigned char iv[16];
				EVP_CIPHER_CTX context;

				memcpy(iv, httpRelativeDecoded, 16);
				initAES(&context, iv, (unsigned char *)aesKey);
				char *urlDecrypted = decryptAES(&context, &httpRelativeDecoded[16], httpRelativeDecodedLength - 16);
				if (urlDecrypted && (*urlDecrypted))
					snprintf(&httpSession->httpFullRequest[5], MAXHTTPREQUESTSIZE - 5, "%s", urlDecrypted);
				else
					systemLog->sysLog(ERROR, "cannot decrypt URL with AES key: %m", strerror(errno));

				freeAES(&context);

				if (urlDecrypted)
					free(urlDecrypted);
				free(httpRelativeDecoded);
			}
		}

#ifdef DEBUGOUTPUT
		fprintf(stderr, "HTTP Request type is: %d", httpSession->httpRequestType);
#endif
		if (httpSession->httpRequestType)
			return 0;
		else
			return -1;
	}

	return -1;
}

void HttpServer::combinedLog(HttpSession *httpSession, char *message, int priority) {
	char requestString[2048];
	time_t currentTime;
	struct tm *localTime;
	char accessTime[128];
	const char *getType = "GET /";
	const char *headType = "HEAD /";
	const char *unknownType = "<UNKNOWN> /";
	const char *httpType;

	if (httpSession->httpRequestType == 1)
		httpType = getType;
	else
	if (httpSession->httpRequestType == 2)
		httpType = headType;
	else
		httpType = unknownType;

	currentTime = time(NULL);
	localTime = localtime(&currentTime);
	strftime(accessTime, sizeof(accessTime), "%d/%b/%Y:%T %z", localTime);

	if (! message)
		snprintf(requestString, sizeof(requestString), "%s %s - - [%s] \"%s%s\" %d %d \"%s\" \"%s\"", httpSession->virtualHost, httpSession->ipSource, accessTime, httpType, httpSession->httpRequest, httpSession->httpCode, httpSession->fileSize, httpSession->referer, httpSession->userAgent);
	else
		snprintf(requestString, sizeof(requestString), "%s - - [%s] \"%s\" %d %d \"%s\" \"%s\"", httpSession->ipSource, accessTime, message, httpSession->httpCode, httpSession->fileSize, httpSession->referer, httpSession->userAgent);

	if (message)
		systemLog->sysLog("numbstat", priority, "%s", requestString);
	else
		systemLog->sysLog(priority, "%s", requestString);
	

	return;
}

int HttpServer::acceptConnection(void) {
	struct kevent kChange[2];
	int clientSocket;
	int returnCode;
	struct sockaddr_in saddr;

	clientSocket = acceptSocket(&saddr);
	if (clientSocket < 0)
		return -1;

	if (getNumberOfConnections() >= maxConnectionsAuthorized) {
		systemLog->sysLog(ERROR, "[%d] no more slots available, total connection pool exceeded (%d), aborting connection", clientSocket, maxConnectionsAuthorized);
		// XXX Implement a Busy Code Server with a default page
		closeSocket(clientSocket);
		return -1;
	}

#ifndef SENDFILE
	// XXX must set a param on httpsession to tell that we can't set blocking mode and then deactivate the multiple accept code on run()
	returnCode = fcntl(clientSocket, F_GETFL, 0);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot get fcntl flags for the socket %d: %s", clientSocket, strerror(errno));
		systemLog->sysLog(ERROR, "cannot set blocking mode for client socket %d", clientSocket);
	}
	else {
		returnCode = returnCode - O_NONBLOCK;
		returnCode = fcntl(clientSocket, F_SETFL, returnCode);
		if (returnCode < 0)
			systemLog->sysLog(ERROR, "cannot set blocking mode for client socket %d", clientSocket);
	}
#endif

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "[%d] Connection accepted, numberOfConnections = %d", clientSocket, getNumberOfConnections());
#endif

	httpSessionsIndex[clientSocket] = new HttpSession();
	httpSessionsIndex[clientSocket]->mutex->lockMutex();
	httpSessionsIndex[clientSocket]->init(clientSocket, &saddr);
	httpSessionsIndex[clientSocket]->mutex->unlockMutex();
	httpSessionsIndex[clientSocket]->burst = burst;
	if (shapping > 0) {
		httpSessionsIndex[clientSocket]->shapping = shapping;
		httpSessionsIndex[clientSocket]->shappingTimeout = (int)((((float)(this->getSendBuffer() * 8)) / shapping));
	}

	EV_SET(&kChange[0], clientSocket, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, readTimeout, 0);
	EV_SET(&kChange[1], clientSocket, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ONESHOT, 0, 0, 0);

	returnCode = kevent(kQueue, kChange, 2, NULL, 0, NULL);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] (%d) problem during kevent: %s\n", httpSessionsIndex[clientSocket]->httpExchange->outputDescriptor, httpSessionsIndex[clientSocket]->httpExchange->inputDescriptor, strerror(errno));
		return -1;
	}

	return 0;
}

int HttpServer::endConnection(HttpSession *httpSession) {
	int returnCode;
 	HttpContext *httpContext;
	int i;
	struct kevent kChange;

#ifdef DEBUGOUTPUT
	printf("endConnection is called on kEvent.ident %d\n", httpSession->kEvent.ident);
#endif
	EV_SET(&kChange, httpSession->kEvent.ident, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
	returnCode = kevent(kQueue, &kChange, 1, NULL, 0, NULL);
	if (httpSession->shappingTimeout > 0) {
		EV_SET(&kChange, httpSession->kEvent.ident + 100000, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
		returnCode = kevent(kQueue, &kChange, 1, NULL, 0, NULL);
	}

	// Call virtual methods of HttpProtocol object
	i = 1;
	httpContext = httpContextList.getFirstElement();
	while (httpContext && strcmp(httpContext->getVirtualHost(), httpSession->virtualHost) && strcmp(httpContext->getVirtualHost(), "*")) {
#ifdef DEBUGOUTPUT	
		fprintf(stderr, "[DEBUG] endConnection virtual is %s, context path is %s\n", httpSession->virtualHost, httpContext->getVirtualHost());
#endif
		httpContext = httpContextList.getElement(i);
		i++;
	}
	if (httpContext) {
		returnCode = httpContext->getHandler()->closeEvent(this, httpSession);
		// XXX test the return code ?
	}

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "httpSession->destroy(true) is called, socket number %d", httpSession->httpExchange->outputDescriptor);
#endif

	httpSession->mustCloseConnection = true;

	return 0;
}

int HttpServer::writeEvent(HttpSession *httpSession) {
	struct kevent kChange[4];
	int returnCode;
	int i;
	HttpContext *httpContext;

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "writeEvent has been called !");
#endif
	
	if (httpSession->kEvent.flags & EV_EOF) {
#ifdef STDOUTDEBUG
		printf("writeEvent : ending connection %d\n", httpSession->kEvent.ident);
#endif
		endConnection(httpSession);
		return 0;
	}

	// Call virtual methods of HttpProtocol object
	i = 1;
	httpContext = httpContextList.getFirstElement();
	while (httpContext && strcmp(httpContext->getVirtualHost(), httpSession->virtualHost) && strcmp(httpContext->getVirtualHost(), "*")) {
#ifdef DEBUGOUTPUT	
		fprintf(stderr, "[DEBUG] i = %d, writeEvent virtual is %s, context path is %s\n", i, httpSession->virtualHost, httpContext->getVirtualHost());
#endif
		httpContext = httpContextList.getElement(i);
		i++;
	}
	if (httpContext) {
		returnCode = httpContext->getHandler()->handle(this, httpSession);
#ifdef DEBUGOUTPUT
		systemLog->sysLog(DEBUG, "httpContext->getHandler()->handle() returns %d and httpSession->endOfAnswer is %d", returnCode, httpSession->endOfAnswer);
#endif
		if (returnCode == 1) {
			EV_SET(&kChange[0], httpSession->kEvent.ident, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
			returnCode = kevent(kQueue, kChange, 1, NULL, 0, NULL);

			return 0;
		}
		if (returnCode == 2) {
			endConnection(httpSession);
			return 0;
		}
		if (returnCode == 3) {
			//EV_SET(&kChange[1], httpSession->kEvent.ident, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ONESHOT, NOTE_LOWAT, sendLoWat, 0);
			EV_SET(&kChange[0], httpSession->kEvent.ident, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, 0);
			returnCode = kevent(kQueue, kChange, 1, NULL, 0, NULL);
			return 0;
		}
	}
	else {
		systemLog->sysLog(ERROR, "no virtualhost declared for '%s'. Ending connection", httpSession->virtualHost);
		endConnection(httpSession);
		return -1;
	}
	
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] (%d) problem while writing event", httpSession->kEvent.ident, httpSession->fileDescriptor);
#ifdef STDOUTDEBUG
		printf("writeEvent : returnCode < 0 : ending connection %d\n", httpSession->kEvent.ident);
#endif
		endConnection(httpSession);
		return -1;
	}

	if (httpSession->endOfAnswer == true) {
		// Ok now we can read again :)
		if (httpSession->keepAliveConnection == true) {
			httpContext->getHandler()->closeEvent(this, httpSession);

			EV_SET(&kChange[0], httpSession->kEvent.ident, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
			//EV_SET(&kChange[1], httpSession->kEvent.ident, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, readTimeout, 0);
			//EV_SET(&kChange[2], httpSession->kEvent.ident, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ONESHOT, 0, 0, 0);

			returnCode = kevent(kQueue, kChange, 1, NULL, 0, NULL);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "[%d] (%d) problem while changing events: %s", httpSession->kEvent.ident, httpSession->fileDescriptor, strerror(errno));
			}
		
			//httpSession->destroy(false);
			//httpSession->reinit();
			endConnection(httpSession);			
			return 0;
		}
		endConnection(httpSession);
		return 0;
	}
	EV_SET(&kChange[0], httpSession->kEvent.ident, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
	EV_SET(&kChange[1], httpSession->kEvent.ident, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, writeTimeout, 0);
	if ((httpSession->shappingTimeout > 0) && (! httpSession->burst))
		EV_SET(&kChange[2], httpSession->kEvent.ident + 100000, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, httpSession->shappingTimeout, 0);
	else {
		if (httpSession->burst)
			httpSession->burst--;
		EV_SET(&kChange[2], httpSession->kEvent.ident, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, 0);
	}
	returnCode = kevent(kQueue, kChange, 3, NULL, 0, NULL);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] (%d) problem while changing events: %s", httpSession->kEvent.ident, httpSession->fileDescriptor, strerror(errno));
	}

	return 0;
}

int HttpServer::readEvent(HttpSession *httpSession) {
	struct kevent kChange[4];
	int returnCode;

#ifdef STDOUTDEBUG
	printf("kEvent.ident is %d\n", httpSession->kEvent.ident);
#endif

	if (httpSession->kEvent.flags & EV_EOF) {
#ifdef STDOUTDEBUG
		printf("readEvent : ending connection %d\n", httpSession->kEvent.ident);
#endif
		endConnection(httpSession);
		return 0;
	}

	returnCode = readRequest(httpSession);
	if (returnCode == -1) {
		systemLog->sysLog(ERROR, "[ %d ] cannot read request %s", httpSession->kEvent.ident, httpSession->httpFullRequest);
#ifdef STDOUTDEBUG
		printf("readEvent : cannot read request : ending connection %d\n", httpSession->kEvent.ident);
#endif
		//endConnection(httpSession);
		//return 0;
		httpSession->httpCode = 400;
		httpSession->noDataToSend = true;
	}

	// If the request is completed then passing kevent to write waiting...
	if (httpSession->endOfRequest == true) {
		EV_SET(&kChange[0], httpSession->kEvent.ident, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ONESHOT, 0, 0, 0);
		EV_SET(&kChange[1], httpSession->kEvent.ident, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
		EV_SET(&kChange[2], httpSession->kEvent.ident, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, writeTimeout, 0);

		returnCode = kevent(kQueue, kChange, 3, NULL, 0, NULL);
		if (returnCode < 0) {
			systemLog->sysLog(ERROR, "[%d] (%d) problem while changing events: %s", httpSession->kEvent.ident, httpSession->fileDescriptor, strerror(errno));
		}

		return 0;
	}
	EV_SET(&kChange[0], httpSession->kEvent.ident, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
	EV_SET(&kChange[1], httpSession->kEvent.ident, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, readTimeout, 0);
	EV_SET(&kChange[2], httpSession->kEvent.ident, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, 0);

	returnCode = kevent(kQueue, kChange, 3, NULL, 0, NULL);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] (%d) problem while deleting an event: %s", httpSession->kEvent.ident, httpSession->fileDescriptor, strerror(errno));
	}

	return 0;
}

int HttpServer::run(void *arguments) {
	struct kevent kEvents[1000];
	int returnCode;
	int i;
	struct timespec kEventTimeout;
	int numberOfEvents;
	struct kevent kChange[1];
#ifdef DEBUGOUTPUT
	struct rusage ru;
#endif
	int clientSocket;
	
	kEventTimeout.tv_sec = 1;
	kEventTimeout.tv_nsec = 0;
	for (;;) {
#ifdef DEBUGOUTPUT
		getrusage(RUSAGE_SELF, &ru);
		systemLog->sysLog(DEBUG, "memory usage: ru_maxrss %d ru_ixrss %d ru_idrss %d ru_isrss %d ru_minflt %d ru_majflt %d", ru.ru_maxrss, ru.ru_ixrss, ru.ru_idrss, ru.ru_isrss, ru.ru_minflt, ru.ru_majflt);
#endif
		numberOfEvents = kevent(kQueue, NULL, 0, kEvents, 1, NULL);
		if (numberOfEvents < 0) {
			systemLog->sysLog(ERROR, "error while receiving an event from kqueue: %s", strerror(errno));
			continue;
			// Remplacer ca par l'extraction de l'evenement en erreur et continuer ensuite...
		}

#ifdef STDOUTDEBUG
		printf("numberOfEvents = %d\n", numberOfEvents);
#endif

		i = 0;
		while (i < numberOfEvents) {
#ifdef STDOUTDEBUG
			printf("EventNumber = %d, event Type = %d\n", i, kEvents[i].filter);
#endif
			// Server socket event
			if (kEvents[i].ident == (unsigned int)sd) {
				returnCode = 0;
				while (! returnCode) {
					returnCode = acceptConnection();
				}
				i++;
				
				continue;
			}
			else {
				if (kEvents[i].ident < 100000) {
					httpSessionsIndex[kEvents[i].ident]->mutex->lockMutex();
					if (httpSessionsIndex[kEvents[i].ident]->initialized == false) {
						httpSessionsIndex[kEvents[i].ident]->mutex->unlockMutex();
						i++;
						continue;
					}

					// Copy Event to the Session
					memcpy(&httpSessionsIndex[kEvents[i].ident]->kEvent, &kEvents[i], sizeof(kEvents[i]));
				}

				switch (kEvents[i].filter) {
					case EVFILT_READ:
#ifdef DEBUGOUTPUT
						systemLog->sysLog(DEBUG, "event read on the socket");
#endif
						returnCode = readEvent(httpSessionsIndex[kEvents[i].ident]);
						break;
					case EVFILT_WRITE:
#ifdef DEBUGOUTPUT
						systemLog->sysLog(DEBUG, "event write on the socket");
#endif
						returnCode = writeEvent(httpSessionsIndex[kEvents[i].ident]);
						break;
					case EVFILT_TIMER:
#ifdef DEBUGOUTPUT
						systemLog->sysLog(DEBUG, "timer expired with event number %d", kEvents[i].ident);
#endif
						if (kEvents[i].ident > 100000) {
							//returnCode = writeEvent(httpSessions[kEvents[i].ident - 100000]);
							EV_SET(&kChange[0], httpSessionsIndex[kEvents[i].ident - 100000]->kEvent.ident, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, 0);
							returnCode = kevent(kQueue, kChange, 1, NULL, 0, NULL);
							if (returnCode < 0) {
								systemLog->sysLog(ERROR, "[%d] (%d) problem while changing events: %s", httpSessionsIndex[kEvents[i].ident - 100000]->kEvent.ident, httpSessionsIndex[kEvents[i].ident - 100000]->fileDescriptor, strerror(errno));
							}
							break;
						}
						if (httpSessionsIndex[kEvents[i].ident]->endOfRequest == true) {
							systemLog->sysLog(ERROR, "[%d] transfer timeout (%d bytes) on socket for %s %s...", httpSessionsIndex[kEvents[i].ident]->httpExchange->outputDescriptor, httpSessionsIndex[kEvents[i].ident]->fileOffset, httpSessionsIndex[kEvents[i].ident]->httpRequest, httpSessionsIndex[kEvents[i].ident]->ipSource);
#ifdef STDOUTDEBUG
							printf("TIMEOUT : ending connection %d\n", httpSessionsIndex[kEvents[i].ident]->kEvent.ident);
#endif
							endConnection(httpSessionsIndex[kEvents[i].ident]);
						}
						else {
							systemLog->sysLog(ERROR, "[%d] READ timeout on socket for %s %s...", httpSessionsIndex[kEvents[i].ident]->httpExchange->outputDescriptor, httpSessionsIndex[kEvents[i].ident]->httpFullRequest, httpSessionsIndex[kEvents[i].ident]->ipSource);
#ifdef STDOUTDEBUGq
							printf("TIMEOUT : ending connection %d\n", httpSessionsIndex[kEvents[i].ident]->kEvent.ident);
#endif
							endConnection(httpSessionsIndex[kEvents[i].ident]);
						}
						break;
					default:
						systemLog->sysLog(ERROR, "kevent filter is unknown: %d", kEvents[i].filter);
						break;
				}
				if (kEvents[i].ident < 100000) {
					if ((httpSessionsIndex[kEvents[i].ident]->mustCloseConnection == true) && (httpSessionsIndex[kEvents[i].ident]->initialized == true)) {	
						clientSocket = httpSessionsIndex[kEvents[i].ident]->httpExchange->getOutput();
						httpSessionsIndex[kEvents[i].ident]->destroy(true);
						httpSessionsIndex[kEvents[i].ident]->mutex->unlockMutex();
						delete httpSessionsIndex[kEvents[i].ident];
						httpSessionsIndex[kEvents[i].ident] = NULL;
						closeSocket(clientSocket);
					}
					else
						httpSessionsIndex[kEvents[i].ident]->mutex->unlockMutex();
				}
			}
			i++;
		}
	}

	// Never executed
	return 0;
}

int HttpServer::startServer(void) {
	// Appeler un run qui va creer une thread pour gerer le Context

	return 0;
}

int HttpServer::stopServer(void) {
	// Arreter les objets qui implementent run

	return 0;
}

HttpContext *HttpServer::createContext(const char *path, HttpHandler *httpHandler) {
	// Creer un Context du serveur (documentroot, Handler object qui implemente handle() etc...)
	// On peux creer plusieurs Context du style:
	// ctx1 "/"
	// ctx2 "/v/0"
	// etc...
	HttpContext *httpContext;

	httpContext = new HttpContext(path, httpHandler);
	httpContextList.addElement(httpContext);

	return httpContext;
}

int HttpServer::removeContext(char *pathId) {
	// Retirer un Context a partir de son documentRoot

	return 0;
}

int HttpServer::getAddress(void) {
	// Retourne l'adresse de binding du serveur HTTP

	return 0;
}

HttpContent *HttpServer::getContent(void) {
	return httpContent;
}

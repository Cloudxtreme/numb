//
// C++ Implementation: httpconnection
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "httpconnection.h"

#include <fcntl.h>

HttpConnection::HttpConnection(CacheObject *_cacheObject, Configuration *_configuration) {
	curl = new Curl();
	cacheObject = _cacheObject;
	configuration = _configuration;
	cantSendMore = false;

	return;
}

HttpConnection::~HttpConnection() {
	return;
}

// Must refactorize on a new class... LogError ?
void HttpConnection::combinedLog(HttpSession *httpSession, char *message, int downloadSize, int priority) {
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
		snprintf(requestString, sizeof(requestString), "%s %s - - [%s] \"%s%s\" %d %d \"%s\" \"%s\"", httpSession->virtualHost, httpSession->ipSource, accessTime, httpType, httpSession->httpRequest, httpSession->httpCode, downloadSize, httpSession->referer, httpSession->userAgent);
	else
		snprintf(requestString, sizeof(requestString), "%s - - [%s] \"%s\" %d %d \"%s\" \"%s\"", httpSession->ipSource, accessTime, message, httpSession->httpCode, downloadSize, httpSession->referer, httpSession->userAgent);

	if (message)
		systemLog->sysLog("numbstat", priority, "%s", requestString);
	else
		systemLog->sysLog(priority, "%s", requestString);
	

	return;
}

size_t callbackFunctionProxy(void *ptr, size_t size, size_t nmemb, void *objects) {
	int **arguments = (int **)objects;
	HttpConnection *httpConnection;
	HttpSession *httpSession;
	ssize_t bytesSent = 0;
	Curl *curl;
	CurlSession *curlSession;
	char *header;
	
	if ((! ptr) || (! objects)) {
		systemLog->sysLog(ERROR, "ptr or objects is NULL in callbackDatas. Cannot continue");
		return 0;
	}

	httpSession = (HttpSession *)arguments[0];
	httpConnection = (HttpConnection *)arguments[1];
	curl = (Curl *)arguments[2];
	curlSession = (CurlSession *)arguments[3];
	header = (char *)arguments[4];

	if ((httpConnection->cantSendMore == false) && curl && curlSession) {
		if (curl->getHttpCode(curlSession) != 404) {
			bytesSent = send(httpSession->httpExchange->outputDescriptor, ptr, size * nmemb, 0);
			if (bytesSent <= 0) {
				httpConnection->cantSendMore = true;
				bytesSent = size * nmemb;
			}
			else
				if (! *header)
					httpSession->fileOffset += bytesSent;
		}
		else
			bytesSent = size * nmemb;
	}

	return bytesSent;
}

size_t callbackFunctionCopyCache(void *ptr, size_t size, size_t nmemb, void *objects) {
	int **arguments = (int **)objects;
	HttpConnection *httpConnection;
	HttpSession *httpSession;
	Curl *curl;
	CurlSession *curlSession;
	
	if ((! ptr) || (! objects)) {
		systemLog->sysLog(ERROR, "ptr or objects is NULL in callbackDatas. Cannot continue");
		return 0;
	}

	httpSession = (HttpSession *)arguments[0];
	httpConnection = (HttpConnection *)arguments[1];
	curl = (Curl *)arguments[2];
	curlSession = (CurlSession *)arguments[3];

	if (httpConnection->cacheObject) {
		if (curl && curlSession) {
			if (curl->getHttpCode(curlSession) == 200)
				httpConnection->cacheObject->put(httpSession, (char *)ptr, size * nmemb);
		}
	}

	return size * nmemb;
}

void HttpConnection::proxyize(HttpSession *httpSession) {
	CurlSession *curlSession;
	char fullUrl[2048];
	char httpArguments[256];
	int *arguments[5];
	int *arguments2[5];
	int returnCode;
	struct timeval sendTimeout;
	socklen_t sendTimeoutSize = sizeof(sendTimeout);
	int clientSocket;
	int httpReturnCode;
	char vxferLog[64];
	int originServerUrlNumber = 0;
	char header = 1;
	char data = 0;
	struct curl_slist *slist = NULL;
	char headerByteRange[64];
	char *pChar;

	if (httpSession->byteRange.start != -1) {
		httpSession->seekPosition = httpSession->byteRange.start;
		if (httpSession->byteRange.end != -1)
			snprintf(headerByteRange, sizeof(headerByteRange), "Range: bytes=%d-%d", httpSession->byteRange.start, httpSession->byteRange.end);
		else
			snprintf(headerByteRange, sizeof(headerByteRange), "Range: bytes=%d-", httpSession->byteRange.start);
		slist = curl_slist_append(slist, headerByteRange);
	}
	while (originServerUrlNumber < 16) {
		sendTimeout.tv_sec = 30;
		sendTimeout.tv_usec = 0;
		returnCode = setsockopt(httpSession->httpExchange->getOutput(), SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sendTimeoutSize);
		if (returnCode < 0) {
			systemLog->sysLog(ERROR, "cannot setsockopt with SO_SNDTIMEO on socket %d: %s", httpSession->httpExchange->getOutput(), strerror(errno));
			clientSocket = httpSession->httpExchange->getOutput();
			httpSession->destroy(true);
			close(clientSocket);
			return;
		}
		curlSession = curl->createSession();
		if (! curlSession) {
			clientSocket = httpSession->httpExchange->getOutput();
			httpSession->destroy(true);
			close(clientSocket);
			return;
		}

		// Add Additional Headers
		if (httpSession->byteRange.start != -1)
			returnCode = curl->setRequestHeader(curlSession, slist);

		// XXX Boundary checking
		/*if (httpSession->seekSeconds) {
			snprintf(httpArguments, sizeof(httpArguments), "seekseconds=%f", httpSession->seekSeconds);
			httpArgumentsPresent = true;
		}
		if (httpSession->seekPosition) {
			snprintf(httpArguments, sizeof(httpArguments), "seek=%d", httpSession->seekPosition);
			httpArgumentsPresent = true;
		}*/
		if (! strstr(httpSession->httpRequest, "getSmil")) {
			pChar = strstr(httpSession->httpRequest, "?");
			if (pChar) {
				snprintf(httpArguments, sizeof(httpArguments), "%s", pChar);
				pChar = strstr(httpArguments, " ");
				if (pChar)
					*pChar = '\0';
				snprintf(fullUrl, sizeof(fullUrl), "%s%s%s", configuration->originServerUrl[originServerUrlNumber], httpSession->videoName, httpArguments);
			}
			else
				snprintf(fullUrl, sizeof(fullUrl), "%s%s", configuration->originServerUrl[originServerUrlNumber], httpSession->videoName);
		}
		else
			snprintf(fullUrl, sizeof(fullUrl), "%s", httpSession->httpFullRequest);

		arguments[0] = (int *)httpSession;
		arguments[1] = (int *)this;
		arguments[2] = (int *)curl;
		arguments[3] = (int *)curlSession;
		arguments[4] = (int *)&data;
		arguments2[0] = (int *)httpSession;
		arguments2[1] = (int *)this;
		arguments2[2] = (int *)curl;
		arguments2[3] = (int *)curlSession;
		arguments2[4] = (int *)&header;
		returnCode = curl->fetchHttpUrlWithCallback(curlSession, (void *)callbackFunctionProxy, (void *)arguments, (void *)callbackFunctionProxy, (void *)arguments2, fullUrl);
		if (returnCode < 0) {
			curl->deleteSession(curlSession);
                        delete curlSession;
			httpSession->destroy(true);
			close(clientSocket);

			return;
		}
		httpReturnCode = curl->getHttpCode(curlSession);

		if (httpReturnCode == 200) {
			if (httpSession->multicastData) {
				unsigned int position;
				bool mustLog = false;

				if (httpSession->mp4Position)
					position = httpSession->mp4Position;
				else
					position = httpSession->seekPosition;
				
				switch (httpSession->multicastData->commandType) {
					case 0:
						char *typeId;
						if (httpSession->multicastData->tagId >= 1 && httpSession->multicastData->tagId <= 3)
							typeId = stringType[httpSession->multicastData->tagId];
						else
							typeId = stringType[0];

						snprintf(vxferLog, sizeof(vxferLog), "VXFER %u;%s;%u;%s;%d;%u;%u", httpSession->multicastData->itemId, httpSession->multicastData->countryCode, httpSession->multicastData->productId, typeId, (int)httpSession->fileOffset, position, httpSession->multicastData->encodingFormatId);
						mustLog = true;
						break;
					case 1:
						snprintf(vxferLog, sizeof(vxferLog), "VX %u;%u;%u;%d;%u;%u", httpSession->multicastData->itemId, httpSession->multicastData->productId, httpSession->multicastData->tagId, (int)httpSession->fileOffset, position, httpSession->multicastData->encodingFormatId);
						mustLog = true;
						break;
				}
				if (mustLog == true)
					combinedLog(httpSession, vxferLog, curl->getContentLength(curlSession), LOG_NOTICE);
			}

			if (slist)
				curl_slist_free_all(slist);
			curl->deleteSession(curlSession);

			clientSocket = httpSession->httpExchange->getOutput();
			delete curlSession;
			httpSession->destroy(true);
			close(clientSocket);
			return;
		}
		else {
			curl->deleteSession(curlSession);
			delete curlSession;

			originServerUrlNumber++;
		}
		if (configuration->originServerUrl[originServerUrlNumber][0] == '\0')
			break;
	}

	if (slist)
		curl_slist_free_all(slist);
	systemLog->sysLog(ERROR, "cannot found file '%s' on origin server urls", httpSession->videoName);
	clientSocket = httpSession->httpExchange->getOutput();
	httpSession->destroy(true);
	close(clientSocket);

	return;
}

void HttpConnection::cache(HttpSession *httpSession) {
	CurlSession *curlSession;
	char fullUrl[2048];
	int *arguments[4];
	int *arguments2[4];
	char videoNameTmpFilePath[2048];
	int returnCode;
	struct timeval sendTimeout;
	int httpReturnCode;
	int originServerUrlNumber = 0;
	long fileTime;
	struct timeval tval[2];
	time_t t;
	struct tm lt;

	while (originServerUrlNumber < 16) {
		sendTimeout.tv_sec = 30;
		sendTimeout.tv_usec = 0;
		curlSession = curl->createSession();
		if (! curlSession) {
			delete httpSession;
			return;
		}
		// XXX Boundary checking
		strcpy(fullUrl, configuration->originServerUrl[originServerUrlNumber]);
		if (httpSession->videoName[0] != '/')
			strcat(fullUrl, "/");
		strcat(fullUrl, httpSession->videoName);

		arguments[0] = (int *)httpSession;
		arguments[1] = (int *)this;
		arguments[2] = (int *)curl;
		arguments[3] = (int *)curlSession;
		arguments2[0] = (int *)httpSession;
		arguments2[1] = (int *)this;
		arguments2[2] = NULL;
		arguments2[3] = NULL;
		returnCode = curl->fetchHttpUrlWithCallback(curlSession, (void *)callbackFunctionCopyCache, (void *)arguments, (void *)callbackFunctionCopyCache, (void *)arguments2, fullUrl);
		httpReturnCode = curl->getHttpCode(curlSession);
		curl->deleteSession(curlSession);

		strcpy(videoNameTmpFilePath, httpSession->videoNameFilePath);
		strcat(videoNameTmpFilePath, ".tmp");

		if ((! returnCode) && (httpReturnCode == 200)) {
			// Move the temp file to the original name (strip .tmp at the end of the file)
			// XXX sanity checks
			fileTime = curl->getLastModified(curlSession);
			if (fileTime > 0) {
				t = fileTime;
				localtime_r(&t, &lt);
				fileTime = mktime(&lt);
				tval[0].tv_sec = fileTime;
				tval[0].tv_usec = 0;
				tval[1].tv_sec = fileTime;
				tval[1].tv_usec = 0;
				utimes(videoNameTmpFilePath, tval);
			}
			returnCode = rename(videoNameTmpFilePath, httpSession->videoNameFilePath);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "[descriptor %d] cannot rename '%s' to '%s': %s", httpSession->httpExchange->getInput(), videoNameTmpFilePath, httpSession->videoNameFilePath, strerror(errno));
				returnCode = unlink(videoNameTmpFilePath);
				if (returnCode < 0) {
					systemLog->sysLog(ERROR, "[descriptor %d] cannot delete the file '%s': %s", httpSession->httpExchange->getInput(), videoNameTmpFilePath, strerror(errno));
				}
			}
			else
				systemLog->sysLog(INFO, "[descriptor %d] File '%s' copied on cache", httpSession->httpExchange->getInput(), httpSession->videoNameFilePath);
			delete curlSession;
			delete httpSession;

			return;
		}
		else {
			systemLog->sysLog(ERROR, "[descriptor %d] file not found at URL '%s', trying another URL...", httpSession->httpExchange->getInput(), configuration->originServerUrl[originServerUrlNumber]);
			delete curlSession;
			originServerUrlNumber++;
		}
		if (configuration->originServerUrl[originServerUrlNumber][0] == '\0')
			break;
	}

	systemLog->sysLog(ERROR, "Cannot found file on origin server urls, can't cache");
	returnCode = unlink(videoNameTmpFilePath);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[descriptor %d] cannot delete the file '%s', oops... admin guys help !: %s", httpSession->httpExchange->getInput(), videoNameTmpFilePath, strerror(errno));
	}
	delete httpSession;

	return;
}

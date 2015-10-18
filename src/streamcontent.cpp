//
// C++ Implementation: streamcontent
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
#include <ctype.h>
#include <netdb.h>

#include "streamcontent.h"
#include "multicastdata.h"
#include "../src/catalogdata.h"
#include "../src/multicastpacketcatalog.h"

StreamContent::StreamContent(Configuration *_configuration, HashTable *_keyHashtable, KeyHashtableTimeout *_keyHashtableTimeout, HashTable *_catalogHashtable, CatalogHashtableTimeout *_catalogHashtableTimeout, MulticastServerCatalog *_multicastServerCatalog, CacheManager *_cacheManager) : HttpContent() {
	char delimiter[2];

	cacheManager = _cacheManager;
	keyHashtable = _keyHashtable;
	catalogHashtable = _catalogHashtable;
	multicastServerCatalog = _multicastServerCatalog;
	keyHashtableTimeout = _keyHashtableTimeout;
	catalogHashtableTimeout = _catalogHashtableTimeout;
	configuration = _configuration;
	delimiter[0] = '&';
	delimiter[1] = '\0';
	parser = new Parser(delimiter);
	if (! parser) {
		systemLog->sysLog(CRITICAL, "cannot malloc a Parser object: %s", strerror(errno));
		return;
	}

	return;
}

StreamContent::~StreamContent() {
	if (parser)
		delete parser;

	return;
}

// urlLength argument is the length in strlen style (without \0)
char *StreamContent::urlDecode(char *url, unsigned int urlLength)
{
	char *urlPtr;
	char *decodedUrl;
	char c1, c2;
	int position = 0;

	decodedUrl = (char *)malloc(urlLength);
	if (! decodedUrl) {
		systemLog->sysLog(CRITICAL, "cannot malloc a string for decodedUrl: %s", strerror(errno));
		return NULL;
	}
	urlPtr = url;
	while (*urlPtr && (*urlPtr != ' ')) {
		if ((*urlPtr == '%') && ((urlLength - position) > 2)) {
			c1 = urlPtr[1];
			c2 = urlPtr[2];
			c1 = tolower(c1);
			c2 = tolower(c2);

			if ((! isxdigit(c1)) || (! isxdigit(c2))) {
				systemLog->sysLog(ERROR, "impossible to decode url : bad isxdigit");
				return NULL;
			}

			if (c1 <= '9')
				c1 = c1 - '0';
			else
				c1 = c1 - 'a' + 10;

			if (c2 <= '9')
				c2 = c2 - '0';
			else
				c2 = c2 - 'a' + 10;

			decodedUrl[position] = 16 * c1 + c2;
			urlPtr += 2;
		}
		else {
			if (*urlPtr == '+')
				decodedUrl[position] = ' ';
			else
				decodedUrl[position] = *urlPtr;
		}
		position++;
		urlPtr++;
	
	}
	decodedUrl[position] = '\0';

	return decodedUrl;
}

bool StreamContent::isDigitString(char *str) {
	int i = 0;

	while (str[i]) {
		if (! isdigit(str[i]))
			return false;
		i++;
	}

	return true;
}

int StreamContent::extractFileName(HttpSession *httpSession) {
	int i;
	char *str;
	char *decodedUrl;
	int listCounter;
	List<String *> *httpArgumentsList = NULL;
	String *httpArgument;
	char *urlArguments = NULL;
	char *key = NULL;
	HashTableElt *hashtableElt;
	struct CatalogData *catalogData;
	struct in_addr inetAddress;
	size_t len;
	char *ch = NULL;

	switch (httpSession->httpRequestType) {
		case 1:
			i = 4;
			break;
		case 2:
			i = 5;
			break;
		default:
			return -1;
			break;
	}
	for (; (httpSession->httpFullRequest[i] != '\n') && (httpSession->httpFullRequest[i] != ' ') && (httpSession->httpFullRequest[i] != '?') && (i < (MAXMSGSIZE - 1)); i++) {
		if ((i < (MAXMSGSIZE-2)) && (((httpSession->httpFullRequest[i] == '.') && (httpSession->httpFullRequest[i+1] == '.')) || ((httpSession->httpFullRequest[i] == '/') && (httpSession->httpFullRequest[i+1] == '.')))) {
			systemLog->sysLog(WARNING, "hacking tentative detected with '/.' or '..' method, Request: %s", httpSession->httpFullRequest);
			return -1;
		}
	}
	if (i <= 0)
		return -1;
	//if (httpSession->httpFullRequest[i-1] == '/')
	//	return -1;
	if (i == MAXMSGSIZE) {
 		systemLog->sysLog(WARNING, "hacking tentative detected or URL too long: %s", httpSession->httpFullRequest);
		return -1;
	}

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "url is %s\n", httpSession->httpFullRequest);
#endif

	// Extract the streaming argument if it's present
	str = strstr(&httpSession->httpFullRequest[5], "?");
	if (str) {
		urlArguments = str;
		str++;
		// Parse http arguments
		decodedUrl = urlDecode(str, strlen(str));
		if (! decodedUrl)
			systemLog->sysLog(ERROR, "cannot decode url '%s'", httpSession->httpFullRequest);
		else {
			httpArgumentsList = parser->tokenizeString(decodedUrl, strlen(decodedUrl));
			if (decodedUrl)
				free(decodedUrl);
			listCounter = 0;
			while (listCounter < httpArgumentsList->listSize) {
				httpArgument = httpArgumentsList->getElement(listCounter + 1);
				if (! strncmp(httpArgument->bloc, "key=", 4)) {
					key = &httpArgument->bloc[4];
				}
				else if (! strncmp(httpArgument->bloc, "seek=", 5)) {
					if (this->isDigitString(&httpArgument->bloc[5]) == true)
						httpSession->seekPosition = atoi(&httpArgument->bloc[5]);
				}
				else if (! strncmp(httpArgument->bloc, "seekseconds=", 12)) {
					httpSession->seekSeconds = strtod(&httpArgument->bloc[12], NULL);
				}
				else if (! strncmp(httpArgument->bloc, "dl=", 3)) {
					if (httpArgument->bloc[3] == '1')
						httpSession->downloadMimeType = true;
				}
				else if (! strncmp(httpArgument->bloc, "sh=", 3)) {
					if (! strncmp(&httpArgument->bloc[3], "auto", 4))
						httpSession->shappingAuto = true;
					else
						if (this->isDigitString(&httpArgument->bloc[3]) == true)
							httpSession->shapping = atoi(&httpArgument->bloc[3]);
				}
				else if (! strncmp(httpArgument->bloc, "kd_", 3)) { // put in requestArgs
				  httpSession->requestArgs.push_back(httpArgument->bloc);
				}
				
				listCounter++;
			}
		}
	}
#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] seekPosition is %d\n", httpSession->seekPosition);
#endif

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "key transmited in url is #%s#\n", key);
#endif
	// Search the key in hashtable
	if (keyHashtable && key) {
		hashtableElt = keyHashtable->search(key);
#ifdef DEBUGOUTPUT
		if (hashtableElt)
			systemLog->sysLog(DEBUG, "key FOUND in hashtable\n");
		else
			systemLog->sysLog(DEBUG, "key NOT FOUND in hashtable\n");
#endif
		if (! hashtableElt) {
			if (configuration->noKeyCheck == false) {
				httpSession->httpCode = 403;
				httpSession->noDataToSend = true;
			}
		}
		else {
			// Storing hashtable informations on the HttpSession object for further use
			httpSession->multicastData = (struct MulticastData *)hashtableElt->getData();
			keyHashtableTimeout->remove(hashtableElt);
			keyHashtable->remove(key);
		}
	}
	else {
		if ((configuration->noKeyCheck == false) && (strstr(httpSession->httpFullRequest, ".flv") || strstr(httpSession->httpFullRequest, ".mp4"))) {
			httpSession->httpCode = 403;
			httpSession->noDataToSend = true;
		}
	}

	// Extract and save the video Name
	if (httpSession->httpRequestType == 1) {
		strncpy(httpSession->videoName, &httpSession->httpFullRequest[4], i - 4);
		httpSession->videoName[i-4] = '\0';
	}
	else {
		strncpy(httpSession->videoName, &httpSession->httpFullRequest[5], i - 5);
		httpSession->videoName[i-5] = '\0';
	}

	if (configuration->shareCatalog == true) {
#ifdef DEBUGCATALOG
		systemLog->sysLog(DEBUG, "trying to search key %s in catalog", httpSession->videoName);
#endif
		catalogHashtable->lock();
		hashtableElt = catalogHashtable->search(httpSession->videoName);
		if (hashtableElt) {
#ifdef DEBUGCATALOG
			systemLog->sysLog(DEBUG, "key %s found in catalog", httpSession->videoName);
#endif
			catalogData = (CatalogData *)hashtableElt->getData();
#ifdef DEBUGCATALOG
			systemLog->sysLog(DEBUG, "host in catalog is #%s#", inet_ntoa(*((struct in_addr *)&catalogData->host)));
			systemLog->sysLog(DEBUG, "configuration->proxyIp is #%s#", inet_ntoa(*((struct in_addr *)&configuration->proxyIp)));
#endif
			if (catalogData->host != configuration->proxyIp.s_addr) {
				len = strlen(inet_ntoa(inetAddress))+strlen(hashtableElt->getKey());
				if (urlArguments) {
					if ((ch = strstr(urlArguments, " ")))
						*ch = '\0';
					len += strlen(urlArguments);
				}
				httpSession->redirectUrl = new char[len + 1];	
				if (httpSession->redirectUrl) {
					httpSession->httpCode = 301;
					inetAddress.s_addr = catalogData->host;
					strcpy(httpSession->redirectUrl, inet_ntoa(inetAddress));
					strcat(httpSession->redirectUrl, hashtableElt->getKey());
					if (urlArguments) {
						strcat(httpSession->redirectUrl, urlArguments);
						if (ch)
							*ch = ' ';
					}
					httpSession->noDataToSend = true;
				}
				else
					systemLog->sysLog(CRITICAL, "cannot allocate httpSession->redirectUrl object: %s", strerror(errno));
			}
		}
		catalogHashtable->unlock();
	}

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "httpSession->httpCode is %d\n", httpSession->httpCode);
#endif

	if (httpArgumentsList)
		delete httpArgumentsList;

	return 0;
}

ssize_t StreamContent::get(HttpSession *httpSession, char *buffer, int bufferSize) {
	ssize_t size;

	if (httpSession->preBuffer && (httpSession->preBufferSent == false)) {
		size = httpSession->preBufferSize - httpSession->preBufferOffset;
		if (size > bufferSize)
			size = bufferSize;
		else
			httpSession->preBufferSent = true;
		
		memcpy(buffer, &httpSession->preBuffer[httpSession->preBufferOffset], size);
		httpSession->preBufferOffset += size;
	}
	else {
		if (httpSession->byteRange.start != -1 && (httpSession->byteRange.end - httpSession->byteRange.start + 1) < bufferSize) {
			httpSession->fileSize = httpSession->byteRange.end - httpSession->byteRange.start + 1;
			size = cacheManager->get(httpSession, buffer, httpSession->byteRange.end - httpSession->byteRange.start + 1);
		}
		else
			size = cacheManager->get(httpSession, buffer, bufferSize);
	}

	return size;
}

int StreamContent::getBitRate(char *videoName) {
        char buffer[140];
        FILE *in;
        int bitrate = 0;
        char cmd[2048];

        snprintf(cmd, sizeof(cmd), "ffmpeg -i %s 2>&1 | grep bitrate | sed 's/.*bitrate: //g' | cut -f1 -d ' '", videoName);
	in = popen(cmd, "r");
        if (in != NULL) {
                fgets(buffer, sizeof(buffer), in);
                bitrate = atoi(buffer);
                pclose(in);
        }

        return bitrate;
}

int StreamContent::initialize(HttpSession *httpSession) {
	int returnCode;
	char *key;
	struct CatalogData *catalogData;
	char hostName[256];
	uint32_t hashPosition;
	HashTableElt *hashtableElt;
	char *buffer;
	size_t bufferLength;

	returnCode = extractFileName(httpSession);
	if (returnCode < 0) {
		return -1;
	}
	if ((httpSession->httpCode == 403) || (httpSession->httpCode == 301)) {
		return 0;
	}
	returnCode = cacheManager->initialize(httpSession);

	// This is a proxyfied request so, dont treat it in kqueue/kevent model
	// And send catalog multicast if needed
	if (returnCode == 1) {
		if ((configuration->shareCatalog == true) && (strstr(httpSession->videoName, ".flv") || strstr(httpSession->videoName, ".mp4"))) {
			key = (char *)malloc(strlen(httpSession->videoName)+1);
			if (! key) {
				systemLog->sysLog(CRITICAL, "cannot allocate key object: %s", strerror(errno));
				return 1;
			}
			strcpy(key, httpSession->videoName);
			returnCode = gethostname(hostName, sizeof(hostName)-1);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "cannot get hostname: %s", strerror(errno));
				free(key);
				return 1;
			}
			catalogData = (struct CatalogData *)malloc(sizeof(struct CatalogData));
			if (! catalogData) {
				systemLog->sysLog(CRITICAL, "cannot create a CatalogData object: %s", strerror(errno));
				free(key);
				return 1;
			}
			catalogData->host = configuration->proxyIp.s_addr;
			catalogData->counter = 1;
#ifdef DEBUGCATALOG
			systemLog->sysLog(DEBUG, "adding entry #%s# with proxyname #%s# in catalog", key, inet_ntoa(*((struct in_addr *)&catalogData->host)));
#endif
			catalogHashtable->lock();
			hashtableElt = catalogHashtable->add(key, catalogData, &hashPosition);
			if (! hashtableElt) {
				systemLog->sysLog(ERROR, "cannot add a redirect on the catalog hashtable");
				free(catalogData);
				catalogHashtable->unlock();
				return 1;
			}
			returnCode = catalogHashtableTimeout->add(hashPosition, hashtableElt);
			if (returnCode < 0) {
				catalogHashtable->remove(key);
				free(catalogData);
				catalogHashtable->unlock();
				return 1;
			}
			catalogHashtable->unlock();
			bufferLength = 2+strlen(key)+1+strlen(inet_ntoa(*((struct in_addr *)&catalogData->host)));
			buffer = new char[bufferLength+1];
			if (! buffer) {
				systemLog->sysLog(CRITICAL, "cannot allocate buffer object with %d bytes: %s", bufferLength+1, strerror(errno));
				catalogHashtableTimeout->remove(hashtableElt);
				catalogHashtable->lock();
				catalogHashtable->remove(key);
				catalogHashtable->unlock();
				free(catalogData);
				return 1;
			}
			snprintf(buffer, bufferLength+1, "%d\n%s\n%s", CATALOGADD, inet_ntoa(*((struct in_addr *)&catalogData->host)), key);
#ifdef DEBUGCATALOG
			systemLog->sysLog(DEBUG, "sending multicast packet on network: #%s#", buffer);
#endif
			multicastServerCatalog->sendPacket(buffer, bufferLength);
			delete buffer;
		}

		return 1;
	}

#ifdef DEBUGSTREAMD
	systemLog->sysLog(ERROR, "[%d] (%d) New file descriptor opened", httpSession->httpExchange->getOutput(), httpSession->httpExchange->getInput());
#endif

	if (httpSession->shappingAuto == true) {
		int bitrate;
		bitrate = getBitRate(httpSession->videoNameFilePath);
		if (bitrate) {
			systemLog->sysLog(INFO, "File '%s' requested, setting shapping to %d\n", httpSession->videoNameFilePath, bitrate);
			httpSession->shapping = bitrate + 256;
		}
	}

	if (httpSession->httpExchange->getInput() < 0) {
		// file not found... 404 HTTP Error
		systemLog->sysLog(ERROR, "[%d] (%d) File '%s' not found", httpSession->httpExchange->getOutput(), httpSession->httpExchange->getInput(), httpSession->videoNameFilePath);
		httpSession->noDataToSend = true;
		httpSession->httpCode = 404;
		return 0;
	}

	if (configuration->shareCatalog == true) {
		// count download number
		catalogHashtable->lock();
		hashtableElt = catalogHashtable->search(httpSession->videoName, &hashPosition);
		if (hashtableElt) {
			catalogData = (struct CatalogData *)hashtableElt->getData();
			if (catalogData) {
				catalogData->counter++;
			}
			// Remove and readd timeout
			catalogHashtableTimeout->remove(hashtableElt);
			returnCode = catalogHashtableTimeout->add(hashPosition, hashtableElt);
			if (returnCode < 0) {
				// XXXFIXME: destroy key from hashtable and remove cache object
				catalogHashtable->unlock();
				return -1;
			}
		}
		catalogHashtable->unlock();
	}

	return returnCode;
}

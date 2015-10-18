//
// C++ Implementation: httpclientconnection
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "../src/multicastdata.h"
#include "../src/mp4streaming.h"

#include "httpclientconnection.h"

#include <cassert>
#include <iostream>
#include <iterator>
#include <algorithm>

#define VXFER_LOG_SIZE 512

HttpClientConnection::HttpClientConnection(Configuration *_configuration) {
	configuration = _configuration;

	return;
}


HttpClientConnection::~HttpClientConnection() {
	return;
}

int HttpClientConnection::handleConnection(HttpServer *httpServer, HttpSession *httpSession) {
	int returnCode = 0;
	ssize_t bytesRead;
	ssize_t bytesToSent;
	Mp4Streaming *mp4Streaming = NULL;

	// Write event received...
	// Test if we have sent HTTP Header already, if not, send it
	if (httpSession->HTTPHeaderInitialized == false) {
		returnCode = httpServer->getContent()->initialize(httpSession);
		if (returnCode < 0) {
			httpSession->httpCode = 404;
			httpSession->noDataToSend = true;
		}
		if (returnCode == 1) {
			httpServer->deleteAConnection();
			return 1;
		}

		// If all is normal continue to construct header
		if (httpSession->httpCode == 200) {
			if (httpSession->seekPosition) {
				if (httpSession->mimeType == 3) {
					if (httpSession->seekPosition < httpSession->fileSize)
						httpSession->fileSize -= httpSession->seekPosition;
					else
						httpSession->seekPosition = 0;
				}
			}
			if (httpSession->seekSeconds) {
				mp4Streaming = new Mp4Streaming(httpSession);
				if (! mp4Streaming)
					systemLog->sysLog(CRITICAL, "cannot allocate a mp4Streaming object, cannot seek file: %s", strerror(errno));
				else {
					mp4Streaming->seek();
					delete mp4Streaming;
				}
			}
			if (httpSession->byteRange.start != -1) {
				httpSession->httpCode = 206;
				if (httpSession->byteRange.end == -1)
				  httpSession->byteRange.end = httpSession->sourceFileStat.st_size > 0 ? httpSession->sourceFileStat.st_size - 1 : 0;
				httpSession->fileSize = httpSession->byteRange.end - httpSession->byteRange.start + 1;
			}
			if (httpSession->shapping) {
				int shappingTimeout = httpServer->getSendBuffer();
				httpSession->shappingTimeout = (int)((((float)(shappingTimeout * 8)) / httpSession->shapping));
			}
		}

		char additionalHeader[1024] = "Cache-Control: max-age=86400";
		if (configuration->aesVHost[0] && (! strcmp(httpSession->virtualHost, configuration->aesVHost))) {
			snprintf(additionalHeader, sizeof(additionalHeader), "Cache-Control: max-age=0,no-store,no-transform");
		}
		if (httpSession->downloadMimeType) {
			snprintf(additionalHeader, sizeof(additionalHeader), "%s\r\nContent-Disposition: attachment; filename=\"%s\"", additionalHeader, httpSession->videoName);
		}

		httpServer->initHeader(httpSession, additionalHeader);
		httpServer->combinedLog(httpSession, NULL, LOG_NOTICE);
		httpSession->HTTPHeaderInitialized = true;
#ifdef DEBUGSTREAMD
		systemLog->sysLog(ERROR, "[%d] HTTP request is %s", httpSession->httpExchange->outputDescriptor, httpSession->httpFullRequest);
#endif
	}

	if (httpSession->HTTPHeaderSent == false) {
		// Writing HTTP Header
		returnCode = httpServer->sendHeader(httpSession);
		if (returnCode < 0)
			return -1;
		// XXX test if the total header was sent... because of EINTR or partial sending...
		httpSession->HTTPHeaderSent = true;
		if (httpSession->noDataToSend == true) {
			httpSession->endOfAnswer = true;
			return 0;
		}
		if (httpSession->seekPosition && strstr(httpSession->videoName, ".flv")) {
			// XXX check the return of send
			send(httpSession->httpExchange->outputDescriptor, "FLV\x01\x01\x00\x00\x00\x09\x00\x00\x09", 13, 0);
			httpSession->fileOffset = 13;
		}

		return 0;
	}

	// HTTP Header was already sent

#ifndef SENDFILE
	// Source file is opened so...
	// If memOffset is 0, we must get a new chunk of the buffer size length
	//if (httpSession->chunkOffset == httpSession->chunkSize)
	if (! httpSession->chunkBytesLeft)
		httpSession->chunkOffset = 0;
	if (! httpSession->chunkOffset) {
		bytesRead = httpServer->getContent()->get(httpSession, httpSession->chunkBuffer, httpSession->chunkSize);
		if (bytesRead < 0) {
			return -1;
		}
		if (! bytesRead) {
			return 2;
		}
		httpSession->chunkBytesLeft = bytesRead;
	}
	// Now we have enough data to send, but we send only the loWat value (signalled by kevent)
	// Then sub sendLoWat from memOffset
	if (httpSession->chunkBytesLeft < httpServer->sendLoWat)
		bytesToSent = httpSession->chunkBytesLeft;
	else
		bytesToSent = httpServer->sendLoWat;

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "server -> client: sending chunk of %d bytes", bytesToSent);
#endif
	returnCode = httpServer->sendChunk(httpSession, &httpSession->chunkBuffer[httpSession->chunkOffset], bytesToSent);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] (%d) Connection is forced to close state", httpSession->httpExchange->outputDescriptor, httpSession->httpExchange->inputDescriptor);
		return -1;
	}
	httpSession->chunkOffset += bytesToSent;
	httpSession->chunkBytesLeft -= bytesToSent;
#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "httpSession->chunkOffset is %d", httpSession->chunkOffset);
	systemLog->sysLog(DEBUG, "httpSession->chunkBytesLeft is %d", httpSession->chunkBytesLeft);
#endif
#else
	returnCode = httpServer->sendChunk(httpSession);
	if (returnCode == -2)
		return 3;
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] (%d) Connection is forced to close state", httpSession->httpExchange->outputDescriptor, httpSession->httpExchange->inputDescriptor);
		return -1;
	}
#endif

	return 0;
}

// timeout method from the HttpHandler class
// Called when a timeout occured on a connection
int HttpClientConnection::closeEvent(HttpServer *httpServer, HttpSession *httpSession) {
	char vxferLog[VXFER_LOG_SIZE];
	unsigned int position = httpSession->mp4Position ? httpSession->mp4Position : httpSession->seekPosition;
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

	if (httpSession->multicastData != NULL) {
		switch (httpSession->multicastData->commandType) {
			case 0:
				char *typeId;
				if (httpSession->multicastData->tagId >= 1 && httpSession->multicastData->tagId <= 3)
					typeId = stringType[httpSession->multicastData->tagId];
				else
					typeId = stringType[0];

				snprintf(vxferLog, sizeof(vxferLog), "VXFER %u;%s;%u;%s;%d;%u;%u;%s%s", httpSession->multicastData->itemId, httpSession->multicastData->countryCode, httpSession->multicastData->productId, typeId, (int)httpSession->fileOffset, position, httpSession->multicastData->encodingFormatId, httpType, httpSession->httpRequest);
				break;

			case 1:
				snprintf(vxferLog, sizeof(vxferLog), "VX %u;%u;%u;%d;%u;%u;%s%s", httpSession->multicastData->itemId, httpSession->multicastData->productId, httpSession->multicastData->tagId, (int)httpSession->fileOffset, position, httpSession->multicastData->encodingFormatId, httpType, httpSession->httpRequest);
				break;

			default:
				snprintf(vxferLog, sizeof(vxferLog), "Cannot log multicast data, command type is unknown (%d)", httpSession->multicastData->commandType);
				break;
		}

		if (configuration->noCache == false)
			httpServer->combinedLog(httpSession, vxferLog, LOG_NOTICE);
	}
	else
	if (httpSession->requestArgs.size() > 0) {
		char * vxpos = vxferLog;
		snprintf(vxferLog, sizeof(vxferLog), "VBX ");
		vxpos += 4;
		for (std::list<std::string>::const_iterator it = httpSession->requestArgs.begin(); it != httpSession->requestArgs.end(); ++it) {
			std::size_t pos = std::string::npos;
			if ((pos = (*it).find("=")) != std::string::npos) {
#ifdef DEBUGOUTPUT
				fprintf(stderr, "'%s' : '%s'\n", (*it).substr(0, pos).c_str(), (*it).substr(pos+1, (*it).size() - pos - 2).c_str());
#endif
				assert((vxpos - vxferLog) < VXFER_LOG_SIZE);
				if ((vxpos - vxferLog) < VXFER_LOG_SIZE) {
					snprintf(vxpos, VXFER_LOG_SIZE - (vxpos - vxferLog), "%s;", (*it).c_str());
					vxpos += (*it).size() + 1;
				}
				else
					break;
			}
		}
		assert((vxpos - vxferLog) < VXFER_LOG_SIZE);
		if ((VXFER_LOG_SIZE - (vxpos - vxferLog)) > 0)
			snprintf(vxpos, VXFER_LOG_SIZE - (vxpos - vxferLog), "a=%d;o=%u", (int)httpSession->fileOffset, position);
		if (configuration->noCache == false)
			httpServer->combinedLog(httpSession, vxferLog, LOG_NOTICE);
#ifdef DEBUGOUTPUT
		fprintf(stderr, "No multicast data, print args\n");
		std::copy(httpSession->requestArgs.begin(), httpSession->requestArgs.end(), std::ostream_iterator<std::string>(std::cout, "\n")); 
		fprintf(stderr, "%s", vxferLog);
#endif
	}

	return 0;
}

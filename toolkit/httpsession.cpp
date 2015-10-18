//
// C++ Implementation: httpsession
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "httpsession.h"

HttpSession::HttpSession(void) {
	videoNameFilePath = (char *)malloc(MAXMSGSIZE + 1);
	if (! videoNameFilePath) {
		systemLog->sysLog(CRITICAL, "cannot create videoNameFilePath: %s", strerror(errno));
		return;
	}
	videoNameNfsFilePath = (char *)malloc(MAXMSGSIZE + 1);
	if (! videoNameNfsFilePath) {
		systemLog->sysLog(CRITICAL, "cannot create videoNameNfsFilePath: %s", strerror(errno));
		return;
	}
	httpExchange = NULL;
	mutex = new Mutex();
	if (! mutex)
		systemLog->sysLog(CRITICAL, "cannot create a Mutex object: %s", strerror(errno));

#ifndef SENDFILE
	chunkSize = 1024000;
	chunkBuffer = NULL;
#endif
	preBuffer = NULL;
	multicastData = NULL;
	redirectUrl = NULL;
	initialized = false;

	return;
}

HttpSession::HttpSession(HttpSession *httpSession) {
#ifndef SENDFILE
	// XXX Perhaps we dont need to allocate here because HTTP <-> HTTP
	chunkOffset = httpSession->chunkOffset;
	chunkSize = 1024000;
	chunkBuffer = (char *)malloc(chunkSize);
	if (! chunkBuffer) {
		systemLog->sysLog(CRITICAL, "cannot allocate %d bytes for chunkBuffer: %s", chunkSize, strerror(errno));
		delete httpExchange;
		return;
	}
	memcpy(chunkBuffer, httpSession->chunkBuffer, chunkSize);
#endif
	if (httpSession->preBuffer) {
		preBuffer = (char *)malloc(httpSession->preBufferSize);
		if (! preBuffer) {
			systemLog->sysLog(CRITICAL, "cannot allocate %d bytes for preBuffer: %s", preBufferSize, strerror(errno));
			if (chunkBuffer)
				free(chunkBuffer);
			delete httpExchange;
			return;
		}
		memcpy(preBuffer, httpSession->preBuffer, preBufferSize);
		preBufferOffset = httpSession->preBufferOffset;
		preBufferSent = httpSession->preBufferSent;
	}
	else
		preBuffer = NULL;
	videoNameFilePath = (char *)malloc(strlen(httpSession->videoNameFilePath) + 1);
	if (! videoNameFilePath) {
		systemLog->sysLog(CRITICAL, "cannot create videoNameFilePath: %s", strerror(errno));
		return;
	}
	videoNameNfsFilePath = (char *)malloc(strlen(httpSession->videoNameNfsFilePath) + 1);
	if (! videoNameNfsFilePath) {
		systemLog->sysLog(CRITICAL, "cannot create videoNameNfsFilePath: %s", strerror(errno));
		return;
	}
	mutex = new Mutex();
	if (! mutex)
		systemLog->sysLog(CRITICAL, "cannot create a Mutex object: %s", strerror(errno));

	httpExchange = new HttpExchange(httpSession->httpExchange->outputDescriptor);
	if (! httpExchange) {
		systemLog->sysLog(CRITICAL, "cannot allocate an HttpExchange object during session opening: %s", strerror(errno));
		return;
	}

	httpExchange->inputDescriptor = httpSession->httpExchange->inputDescriptor;
	httpExchange->outputDescriptor = httpSession->httpExchange->outputDescriptor;

	smsg = NULL;
	strcpy(videoName, httpSession->videoName);
	httpCode = httpSession->httpCode;
	strcpy(httpFullRequest, httpSession->httpFullRequest);
	requestSize = httpSession->requestSize;
	httpRequestType = httpSession->httpRequestType;
	strcpy(httpRequest, httpSession->httpRequest);
	strcpy(virtualHost, httpSession->virtualHost);
	strcpy(referer, httpSession->referer);
	strcpy(userAgent, httpSession->userAgent);
	strcpy(ipSource, httpSession->ipSource);
	strcpy(httpHeader, httpSession->httpHeader);
	byteRange.start = httpSession->byteRange.start;
	byteRange.end = httpSession->byteRange.end;
	fileSize = httpSession->fileSize;
	memcpy(&sourceFileStat, &httpSession->sourceFileStat, sizeof(sourceFileStat));
	seekPosition = httpSession->seekPosition;
	seekSeconds = httpSession->seekSeconds;
	mp4Position = httpSession->mp4Position;
	keepAliveConnection = httpSession->keepAliveConnection;
	endOfRequest = httpSession->endOfRequest;
	endOfAnswer = httpSession->endOfAnswer;
	noDataToSend = httpSession->noDataToSend;
	localFileCreated = httpSession->localFileCreated;
	videoChunkSize = httpSession->videoChunkSize;
	fileDescriptor = httpSession->fileDescriptor;
	HTTPHeaderInitialized = httpSession->HTTPHeaderInitialized;
	HTTPHeaderSent = httpSession->HTTPHeaderSent;
	strcpy(videoNameFilePath, httpSession->videoNameFilePath);
	strcpy(videoNameNfsFilePath, httpSession->videoNameNfsFilePath);
	fileOffset = httpSession->fileOffset;
	multicastData = NULL;
	redirectUrl = NULL;
	mustCloseConnection = httpSession->mustCloseConnection;
	burst = httpSession->burst;
	initialized = httpSession->initialized;
	downloadMimeType = httpSession->downloadMimeType;
	shapping = httpSession->shapping;
	shappingTimeout = httpSession->shappingTimeout;
	shappingAuto = httpSession->shappingAuto;
	
	return;
}

HttpSession::~HttpSession() {
	if (videoNameFilePath)
		delete videoNameFilePath;
	if (videoNameNfsFilePath)
		delete videoNameNfsFilePath;
	if (httpExchange)
		delete httpExchange;
	if (mutex)
		delete mutex;
#ifndef SENDFILE
	if (chunkBuffer)
		free(chunkBuffer);
#endif
	if (preBuffer)
		free(preBuffer);

	if (multicastData)
		free(multicastData);

	if (redirectUrl)
		delete redirectUrl;

	return;
}

void HttpSession::reinit() {
	char *stringPtr;

#ifndef SENDFILE
	chunkOffset = 0;
	chunkBuffer = (char *)malloc(chunkSize);
	if (! chunkBuffer) {
		systemLog->sysLog(CRITICAL, "cannot allocate %d bytes for chunkBuffer: %s", chunkSize, strerror(errno));
		delete httpExchange;
		return;
	}
	chunkBuffer[0] = '\0';
#endif
	preBuffer = NULL;
	preBufferSize = 0;
	preBufferOffset = 0;
	preBufferSent = false;
	smsg = NULL;
	videoName[0] = '\0';
	httpCode = -1;
	httpFullRequest[0] = '\0';
	requestSize = 0;
	httpRequestType = 0;
	httpRequest[0] = '\0';
	virtualHost[0] = '\0';
	referer[0] = '\0';
	userAgent[0] = '\0';
	httpHeader[0] = '\0';
	byteRange.start = -1;
	byteRange.end = -1;
	fileSize = -1;
	seekPosition = 0;
	seekSeconds = 0;
	mp4Position = 0;
	keepAliveConnection = false;
	endOfRequest = false;
	endOfAnswer = false;
	noDataToSend = false;
	localFileCreated = true;
	// XXX Correct that !!!!
	videoChunkSize = 262000;
	//mmapOffset = 0;
	fileDescriptor = 0;
	HTTPHeaderInitialized = false;
	HTTPHeaderSent = false;
	videoNameFilePath[0] = '\0';
	videoNameNfsFilePath[0] = '\0';
	fileOffset = 0;
	multicastData = NULL;
	redirectUrl = NULL;
	mustCloseConnection = false;
	initialized = true;
	burst = 0;
	downloadMimeType = false;
	shapping = 0;
	shappingTimeout = 0;
	shappingAuto = false;

	return;
}

void HttpSession::init(int _clientSocket, struct sockaddr_in *_sourceAddress) {
	char *stringPtr;

#ifndef SENDFILE
	chunkOffset = 0;
	chunkBuffer = (char *)malloc(chunkSize);
	if (! chunkBuffer) {
		systemLog->sysLog(CRITICAL, "cannot allocate %d bytes for chunkBuffer: %s", chunkSize, strerror(errno));
		delete httpExchange;
		return;
	}
	chunkBuffer[0] = '\0';
#endif
	preBuffer = NULL;
	preBufferSize = 0;
	preBufferOffset = 0;
	preBufferSent = false;
	httpExchange = new HttpExchange(_clientSocket);
	if (! httpExchange) {
		systemLog->sysLog(CRITICAL, "cannot allocate an HttpExchange object during session opening: %s", strerror(errno));
		return;
	}
        memcpy(&sourceAddress, _sourceAddress, sizeof(sourceAddress));
        stringPtr = inet_ntoa(sourceAddress.sin_addr);
        if (stringPtr) {
                strncpy(ipSource, stringPtr, sizeof(ipSource)-1);
		ipSource[sizeof(ipSource)-1] = '\0';
	}
        else {
                systemLog->sysLog(ERROR, "[%d] Can't extract ip source address: %s", _clientSocket, strerror(errno));
                strncpy(ipSource, "0.0.0.0", sizeof(ipSource)-1);
		ipSource[sizeof(ipSource)-1] = '\0';
        }
	smsg = NULL;
	videoName[0] = '\0';
	httpCode = -1;
	httpFullRequest[0] = '\0';
	requestSize = 0;
	httpRequestType = 0;
	httpRequest[0] = '\0';
	virtualHost[0] = '\0';
	referer[0] = '\0';
	userAgent[0] = '\0';
	httpHeader[0] = '\0';
	byteRange.start = -1;
	byteRange.end = -1;
	fileSize = -1;
	seekPosition = 0;
	seekSeconds = 0;
	mp4Position = 0;
	keepAliveConnection = false;
	endOfRequest = false;
	endOfAnswer = false;
	noDataToSend = false;
	localFileCreated = true;
	// XXX Correct that !!!!
	videoChunkSize = 262000;
	//mmapOffset = 0;
	fileDescriptor = 0;
	HTTPHeaderInitialized = false;
	HTTPHeaderSent = false;
	videoNameFilePath[0] = '\0';
	videoNameNfsFilePath[0] = '\0';
	fileOffset = 0;
	multicastData = NULL;
	redirectUrl = NULL;
	mustCloseConnection = false;
	initialized = true;
	burst = 0;
	downloadMimeType = false;
	shapping = 0;
	shappingTimeout = 0;
	shappingAuto = false;

	return;
}

void HttpSession::init(void) {
#ifndef SENDFILE
	chunkOffset = 0;
	chunkBuffer = (char *)malloc(chunkSize);
	if (! chunkBuffer) {
		systemLog->sysLog(CRITICAL, "cannot allocate %d bytes for chunkBuffer: %s", chunkSize, strerror(errno));
		delete httpExchange;
		return;
	}
	chunkBuffer[0] = '\0';
#endif
	preBuffer = NULL;
	preBufferSize = 0;
	preBufferOffset = 0;
	preBufferSent = false;
	smsg = NULL;
	videoName[0] = '\0';
	httpCode = -1;
	httpFullRequest[0] = '\0';
	requestSize = 0;
	httpRequestType = 0;
	httpRequest[0] = '\0';
	virtualHost[0] = '\0';
	referer[0] = '\0';
	userAgent[0] = '\0';
	httpHeader[0] = '\0';
	byteRange.start = -1;
	byteRange.end = -1;
	fileSize = -1;
	seekPosition = 0;
	seekSeconds = 0;
	mp4Position = 0;
	keepAliveConnection = false;
	endOfRequest = false;
	endOfAnswer = false;
	noDataToSend = false;
	localFileCreated = true;
	// XXX Correct that !!!!
	videoChunkSize = 262000;
	//mmapOffset = 0;
	fileDescriptor = 0;
	HTTPHeaderInitialized = false;
	HTTPHeaderSent = false;
	videoNameFilePath[0] = '\0';
	videoNameNfsFilePath[0] = '\0';
	fileOffset = 0;
	multicastData = NULL;
	redirectUrl = NULL;
	mustCloseConnection = false;
	initialized = true;
	burst = 0;
	downloadMimeType = false;
	shapping = 0;
	shappingTimeout = 0;
	shappingAuto = false;

	return;	
}

void HttpSession::destroy(bool closeSocket) {
	if (closeSocket == true) {
		delete httpExchange;
		httpExchange = NULL;
	}
	else
		close(httpExchange->getInput());
#ifndef SENDFILE
	if (chunkBuffer) {
#ifdef DEBUGOUTPUT
		systemLog->sysLog(DEBUG, "freeing chunkBuffer");
#endif
		free(chunkBuffer);
		chunkBuffer = NULL;
	}
#endif
	if (preBuffer) {
		free(preBuffer);
		preBuffer = NULL;
		preBufferSize = 0;
		preBufferOffset = 0;
		preBufferSent = false;
	}
	if (multicastData) {
		free(multicastData);
		multicastData = NULL;
	}
	
	if (redirectUrl) {
		delete redirectUrl;
		redirectUrl = NULL;
	}

	if (smsg)
		free(smsg);

	initialized = false;

	return;
}

void HttpSession::setBurst(char _burst) {
	burst = _burst;

	return;
}

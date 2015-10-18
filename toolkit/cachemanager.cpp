//
// C++ Implementation: cachemanager
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "cachemanager.h"

#include "../toolkit/thread.h"
#include "../toolkit/httpconnection.h"
#include "../src/disktomemory.h"

CacheManager::CacheManager(Configuration *_configuration) {
	configuration = _configuration;
	//cacheMemory = new CacheMemory();
	cacheDisk = new CacheDisk(configuration);

	return;
}

CacheManager::~CacheManager() {
	//delete cacheMemory;
	delete cacheDisk;

	return;
}

int CacheManager::initialize(HttpSession *httpSession) {
	Thread *httpConnectionCacheThread;
	Thread *httpConnectionProxyThread;
	//Thread *diskToMemoryThread;
	HttpConnection *httpConnectionCache = NULL;
	HttpConnection *httpConnectionProxy = NULL;
	//DiskToMemory *diskToMemory;
	int returnCode;
	HttpSession *httpSessionCopy;

	returnCode = cacheDisk->initialize(httpSession);
	if (httpSession->initialized == false) {
		systemLog->sysLog(CRITICAL, "httpSession is unitialized !!! what is that ?!?");	
	}

#ifdef DEBUGOUTPUT
	printf("returnCode of cachemanager is %d\n", returnCode);
#endif
	if ((returnCode < 0) && (configuration->noCache == false)) {
		// patch SMIL files
		if (strstr(httpSession->httpRequest, ".smil")) {
			fprintf(stderr, httpSession->httpRequest);
			char *smilTag = strstr(httpSession->httpRequest, "smil/");
			char buffer[64];
			char *bufferPtr;
			
			if (smilTag) {
				smilTag += 5;
			
				snprintf(buffer, sizeof(buffer), "%s", smilTag);
			
				bufferPtr = buffer;
				while (*bufferPtr && (*bufferPtr != '.')) bufferPtr++;
			
				*bufferPtr = '\0';
			
				snprintf(httpSession->httpFullRequest, sizeof(httpSession->httpFullRequest), "http://api.example.com/video/getSmil/?shortsig=%s", buffer);
				snprintf(httpSession->httpRequest, sizeof(httpSession->httpRequest), "/video/getSmil/?shortsig=%s", buffer);
			
				returnCode = -2;
			}
		}
		if (returnCode == -1) {
			httpConnectionCache = new HttpConnection(cacheDisk, configuration);
			if (! httpConnectionCache) {
				systemLog->sysLog(ERROR, "cannot create a HttpConnection object: %s", strerror(errno));
				return -1;
			}
			httpConnectionCacheThread = new Thread(httpConnectionCache);
			httpSessionCopy = new HttpSession(httpSession);
			httpSessionCopy->httpExchange->outputDescriptor = 0;
			httpConnectionCacheThread->createThread(httpSessionCopy);
			httpSession->httpExchange->inputDescriptor = 0;
		}
		httpConnectionProxy = new HttpConnection(NULL, configuration);
		if (! httpConnectionProxy) {
			systemLog->sysLog(ERROR, "cannot create a HttpConnection object: %s", strerror(errno));
			return -1;
		}
		httpConnectionProxyThread = new Thread(httpConnectionProxy);
		// XXX modify HttpSession to set the HttpContext inside
		// XXX then we can log VXFER with classic proxyfied HTTP sessions
		httpConnectionProxyThread->createThread(httpSession);

		return 1;
	}
	/*returnCode = cacheMemory->initialize(httpSession);
	if (returnCode < 0) {
		diskToMemory = new DiskToMemory(cacheMemory);
		if (! diskToMemory) {
			systemLog->sysLog(ERROR, "cannot create a DiskToMemory object: %s", strerror(errno));
			return -1;
		}
		diskToMemoryThread = new Thread(diskToMemory);
		diskToMemoryThread->createThread(httpSession);
	}
	else {
		// XXX deinitialize cacheDisk ! not necessary
	}*/
	
	return returnCode;
}

ssize_t CacheManager::get(HttpSession *httpSession, char *buffer, int bufferSize) {
	ssize_t bytesRead = 0;

#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] CacheManager::get()\n");
#endif

	if (httpSession->httpExchange->getMediaType() == 1)
		bytesRead = cacheDisk->get(httpSession, buffer, bufferSize);

	/*if (httpSession->httpExchange->getMediaType() == 2)
		bytesRead = cacheMemory->get(httpSession, buffer, bufferSize); */

#ifdef DEBUGOUTPUT
	printf("returnCode for cacheDisk->get() : %d\n", bytesRead);
	printf("mediaType is %d\n", httpSession->httpExchange->getMediaType());
#endif

	return bytesRead;
}

int CacheManager::remove(char *objectName) {
	int returnCode = -1;

	returnCode = cacheDisk->remove(objectName);
	/*	returnCode = cacheMemory->remove(objectName); */

	return returnCode;
}

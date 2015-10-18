//
// C++ Implementation: cachedisk
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

#include "cachedisk.h"

CacheDisk::CacheDisk(Configuration *_configuration) {
	configuration = _configuration;

	return;
}

CacheDisk::~CacheDisk() {
	return;
}

int CacheDisk::initialize(HttpSession *httpSession) {
	int descriptor = -1;
	int returnCode;
	char videoNameTmpFilePath[2048];
	struct stat tmpFileStat;

#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] initialize diskcache\n");
#endif
	// XXX boundary checking !
	strcpy(httpSession->videoNameFilePath, configuration->cacheDirectory);
	strcat(httpSession->videoNameFilePath, httpSession->videoName);
#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] lstat on %s\n", httpSession->videoNameFilePath);
#endif
	returnCode = lstat(httpSession->videoNameFilePath, &httpSession->sourceFileStat);
#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] lstat return code is: %d\n", returnCode);
#endif
	if (returnCode < 0) {
		systemLog->sysLog(NOTICE, "[%d] File '%s' can't get from cache: %s", httpSession->httpExchange->outputDescriptor, httpSession->videoNameFilePath, strerror(errno));
		// XXX Boundary checking
		strcpy(videoNameTmpFilePath, httpSession->videoNameFilePath);
		strcat(videoNameTmpFilePath, ".tmp");
		returnCode = lstat(videoNameTmpFilePath, &tmpFileStat);
		if (returnCode < 0) {
			descriptor = open(videoNameTmpFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
			if (descriptor < 0) {
				systemLog->sysLog(ERROR, "[%d] (%d) Cannot create file '%s' on disk cache: %s", httpSession->httpExchange->outputDescriptor, descriptor, videoNameTmpFilePath, strerror(errno));
				return -2;
			}
			
			httpSession->httpExchange->inputDescriptor = descriptor;
			return -1;
		}

		return -2;
	}
	if ((httpSession->sourceFileStat.st_mode & S_IFREG) != S_IFREG) {
		systemLog->sysLog(ERROR, "[%d] (%d) File '%s' is not a regular file", httpSession->httpExchange->outputDescriptor, descriptor, httpSession->videoNameFilePath);
		return -2;
	}

	httpSession->fileSize = httpSession->sourceFileStat.st_size;

	descriptor = open(httpSession->videoNameFilePath, O_RDONLY/* | O_DIRECT */, 0);
	if (descriptor < 0) {
		systemLog->sysLog(ERROR, "[%d] (%d) cannot open file '%s': %s", httpSession->httpExchange->outputDescriptor, descriptor, httpSession->videoNameFilePath, strerror(errno));
		return -2;
	}
	
	// XXX Must clean that
	if (httpSession->seekPosition) {
		returnCode = lseek(descriptor, httpSession->seekPosition, SEEK_SET);
		if (returnCode < 0) {
			systemLog->sysLog(ERROR, "cannot seek at %d for the file %s: %s", httpSession->seekPosition, httpSession->videoNameFilePath);
		}
		else
			httpSession->httpExchange->setInputOffset(httpSession->seekPosition);
	}

	httpSession->httpExchange->inputDescriptor = descriptor;
	httpSession->httpExchange->setMediaType(1);

	return 0;
}

ssize_t CacheDisk::get(HttpSession *httpSession, char *buffer, int bufferSize) {
	ssize_t bytesRead;

	if (! httpSession) {
		systemLog->sysLog(ERROR, "httpSession is NULL, cannot request the cache");
		return -1;
	}
	if (! buffer) {
		systemLog->sysLog(ERROR, "buffer is NULL, cannot request the cache");
		return -1;
	}

	bytesRead = read(httpSession->httpExchange->inputDescriptor, buffer, bufferSize);
	if (bytesRead < 0)
		systemLog->sysLog(ERROR, "cannot read the descriptor of the session: %s", strerror(errno));

	httpSession->httpExchange->inputOffset += bytesRead;

	return bytesRead;
}

ssize_t CacheDisk::put(HttpSession *httpSession, char *buffer, int bufferSize) {
	ssize_t bytesWrote;

	bytesWrote = write(httpSession->httpExchange->inputDescriptor, buffer, bufferSize);
	if (bytesWrote < 0) {
		systemLog->sysLog(ERROR, "[%d] (%d) Cannot write in the cache file: %s", httpSession->httpExchange->outputDescriptor, httpSession->httpExchange->inputDescriptor, strerror(errno));
		return -1;
	}

	//httpSession->httpExchange->setInputOffset(httpSession->httpExchange->getInputOffset() + bytesWrote);

	return 0;
}

int CacheDisk::remove(char *relativePath) {
	char *absolutePath;
	int returnCode;
	size_t len;

	len = strlen(configuration->cacheDirectory)+strlen(relativePath)+1;
	absolutePath = new char[len];
	if (! absolutePath) {
		systemLog->sysLog(CRITICAL, "absolutePath object cannot be created: %s", strerror(errno));
		return -1;
	}
	snprintf(absolutePath, len, "%s%s", configuration->cacheDirectory, relativePath);
#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "trying to delete #%s# file from disk", absolutePath);
#endif
	returnCode = unlink(absolutePath);
	delete absolutePath;

	return returnCode;
}

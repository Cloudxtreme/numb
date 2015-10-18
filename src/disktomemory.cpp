//
// C++ Implementation: disktomemory
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

#include "disktomemory.h"

DiskToMemory::DiskToMemory(CacheMemory *_cacheMemory) {
	cacheMemory = _cacheMemory;

	return;
}


DiskToMemory::~DiskToMemory() {
	return;
}

void DiskToMemory::copy(void *arguments) {
	HttpSession *httpSession = (HttpSession *)arguments;
 	int fileDescriptor;
	char buffer[131072];
	ssize_t bytesRead = 1;
	ssize_t totalBytesRead = 0;
	int returnCode;
	CacheMemoryObject *cacheMemoryObject;

	fileDescriptor = open(httpSession->videoNameFilePath, O_RDONLY);
	if (! fileDescriptor) {
		systemLog->sysLog(ERROR, "cannot open file '%s' in read only mode", httpSession->videoNameFilePath);
		cacheMemory->remove(httpSession);
		return;
	}
	while (bytesRead) {
		bytesRead = read(fileDescriptor, buffer, sizeof(buffer));
		if (bytesRead < 0) {
			systemLog->sysLog(ERROR, "cannot read file '%s': %s", httpSession->videoName, strerror(errno));
			close(fileDescriptor);
			return;
		}
		cacheMemory->put(httpSession, buffer, bytesRead);
		totalBytesRead += bytesRead;
	}
	close(fileDescriptor);

	return;
}

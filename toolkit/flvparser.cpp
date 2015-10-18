//
// C++ Implementation: flvparser
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include "flvparser.h"

FlvParser::FlvParser(int _fileDescriptor) {
	fileDescriptor = _fileDescriptor;
	filePosition = 0;
	mmapedFile = (char *)mmap(NULL, 2117191, PROT_READ, MAP_FILE, fileDescriptor, 0);
	if (! mmapedFile) {
		systemLog->sysLog(ERROR, "[%d] cannot mmap file: %s", fileDescriptor, strerror(errno));
		return;
	}
	flvHeader = new FlvHeader(mmapedFile, &filePosition);
	if (! flvHeader) {
		systemLog->sysLog(ERROR, "[%d] cannot initialize a FlvHeader object: %s", fileDescriptor, strerror(errno));
		return;
	}
	flvStream = new FlvStream(mmapedFile, &filePosition);
	if (! flvStream) {
		systemLog->sysLog(ERROR, "[%d] cannot initialize a FlvStream object: %s", fileDescriptor, strerror(errno));
		return;
	}

	return;
}


FlvParser::~FlvParser() {
	if (flvHeader)
		delete flvHeader;
	if (flvStream)
		delete flvStream;
	munmap(mmapedFile, 2117191);

	return;
}

int FlvParser::searchKeyFrameByTime(time_t timestamp) {
	int returnCode;

	returnCode = flvHeader->process();
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] cannot process the FlvHeader", fileDescriptor);
		return -1;
	}
	flvHeader->print();
	for (;;) {
		returnCode = flvStream->process();
		if (returnCode < 0) {
			systemLog->sysLog(ERROR, "[%d] cannot process the FlvStream header", fileDescriptor);
			break;
		}
		//flvStream->print();
		if ((flvStream->timestamp >= timestamp) && (flvStream->frameType == FRAME_KEY)) {
			printf("Frame Key FOUND at %d !!!\n", flvStream->timestamp);
			printf("return filepos = %d\n", filePosition);
			return (filePosition - 11);
		}
		filePosition += flvStream->bodyLength;
		if (filePosition >= 2117191)
			break;
	}

	return 0;
}

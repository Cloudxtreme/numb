//
// C++ Implementation: flvstream
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
#include <netinet/in.h>

#include "log.h"
#include "flvstream.h"

FlvStream::FlvStream(char *_mmapedFile, int *_filePosition) {
	mmapedFile = _mmapedFile;
	filePosition = _filePosition;

	return;
}


FlvStream::~FlvStream() {
	return;
}

int FlvStream::process(void) {
	uint32_t *ptrInt;
	u_char *ptrChar;

	ptrInt = (uint32_t *)&mmapedFile[*filePosition];
	(*filePosition) += sizeof(*ptrInt);
	previousTagSize = *ptrInt;
	// Bid Endian to Little Endian conversion
	previousTagSize = ntohl(previousTagSize);
	ptrChar = (u_char *)&mmapedFile[*filePosition];
	(*filePosition) += sizeof(*ptrChar);
	type = *ptrChar;
	bodyLengthBytes[0] = 0;
	bodyLengthBytes[1] = mmapedFile[*filePosition];
	(*filePosition)++;
	bodyLengthBytes[2] = mmapedFile[*filePosition];
	(*filePosition)++;
	bodyLengthBytes[3] = mmapedFile[*filePosition];
	(*filePosition)++;
	ptrInt = (uint32_t *)bodyLengthBytes;
	bodyLength = *ptrInt;
	// Big Endian to Little Endian conversion
	bodyLength = ntohl(bodyLength);
	timestampBytes[0] = 0;
	timestampBytes[1] = mmapedFile[*filePosition];
	(*filePosition)++;
	timestampBytes[2] = mmapedFile[*filePosition];
	(*filePosition)++;
	timestampBytes[3] = mmapedFile[*filePosition];
	(*filePosition)++;
	ptrInt = (uint32_t *)timestampBytes;
	timestamp = *ptrInt;
	// Big Endian to Little Endian conversion
	timestamp = ntohl(timestamp);
	ptrInt = (uint32_t *)&mmapedFile[*filePosition];
	padding = *ptrInt;
	(*filePosition) += sizeof(padding);

	switch (type) {
		case TYPE_AUDIO:
			break;
		case TYPE_VIDEO:
			codecId = (mmapedFile[*filePosition] & 0x0f) >> 0;
			frameType = (mmapedFile[*filePosition] & 0xf0) >> 4;
			printVideo();
			break;
		case TYPE_META:
			break;
	}

	return 0;
}

void FlvStream::print(void) {
	systemLog->sysLog(INFO, "previousTagSize	: %u", previousTagSize);
	systemLog->sysLog(INFO, "type		: 0x%.2X", type);
	systemLog->sysLog(INFO, "bodyLength	: %u", bodyLength);
	systemLog->sysLog(INFO, "timestamp	: %u", timestamp);
	systemLog->sysLog(INFO, "padding		: 0x%.8X", padding);

	return;
}

void FlvStream::printVideo(void) {
	systemLog->sysLog(INFO, "codecId	: %u", codecId);
	systemLog->sysLog(INFO, "frameType : %u", frameType);

	return;
}

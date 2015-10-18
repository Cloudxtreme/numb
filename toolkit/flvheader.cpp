//
// C++ Implementation: flvheader
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

#include "flvheader.h"

FlvHeader::FlvHeader(char *_mmapedFile, int *_filePosition) {
	mmapedFile = _mmapedFile;
	filePosition = _filePosition;
	bzero(signature, sizeof(signature));
	// XXX check the return of memcpy
}

FlvHeader::~FlvHeader() {
}

int FlvHeader::process(void) {
	char *charPtr;

	printf("filePosition is %d\n", *filePosition);
	charPtr = (char *)&mmapedFile[*filePosition];
	signature[0] = charPtr[0];
	signature[1] = charPtr[1];
	signature[2] = charPtr[2];
	signature[3] = 0;
	(*filePosition) += 3;
	version = mmapedFile[*filePosition];
	(*filePosition)++;
	flags = mmapedFile[*filePosition];
	(*filePosition)++;
	offset = *((unsigned int *)&mmapedFile[*filePosition]);
	(*filePosition) += sizeof(offset);
	// Big Endian to Little Endian conversion
	offset = ntohl(offset);

	return 0;
}

void FlvHeader::print(void) {
	systemLog->sysLog(INFO, "Signature	: %s", signature);
	systemLog->sysLog(INFO, "Version	: %u", version);
	systemLog->sysLog(INFO, "Flags	: %u", flags);
	systemLog->sysLog(INFO, "Offset	: %u", offset);

	return;
}

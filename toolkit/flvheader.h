//
// C++ Interface: flvheader
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef FLVHEADER_H
#define FLVHEADER_H

#include "log.h"

/**
	@author  <spe@>
*/
class FlvHeader{
private:
	char *mmapedFile;
	int *filePosition;
public:
	char header[9];
	unsigned char signature[4];
	unsigned char version;
	unsigned char flags;
	unsigned int offset;

	FlvHeader(char *, int *);
	~FlvHeader();

	int process(void);
	void print(void);
};

#endif

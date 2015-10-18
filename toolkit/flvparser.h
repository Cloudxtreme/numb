//
// C++ Interface: flvparser
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef FLVPARSER_H
#define FLVPARSER_H

#include "log.h"
#include "flvstream.h"
#include "flvheader.h"

/**
	@author  <spe@>
*/

class FlvParser{
private:
	int fileDescriptor;
	FlvHeader *flvHeader;
	FlvStream *flvStream;
	char *mmapedFile;
	int filePosition;
	
public:
	FlvParser(int);
	~FlvParser();

	int searchKeyFrameByTime(time_t);
};

#endif

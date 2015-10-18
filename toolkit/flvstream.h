//
// C++ Interface: flvstream
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef FLVSTREAM_H
#define FLVSTREAM_H

#define TYPE_VIDEO 0x09
#define TYPE_AUDIO 0x08
#define TYPE_META 0x12

#define CODEC_SORENSEN 2
#define CODEC_SCREEN_VIDEO 3
#define CODEC_ON2_VP6 4

#define FRAME_KEY 1
#define FRAME_INTER 2
#define FRAME_DISPOSABLE_INTER 3

/**
	@author  <spe@>
*/
class FlvStream{
private:
	char *mmapedFile;
	int *filePosition;

public:
	uint32_t previousTagSize;
	u_char type;
	char bodyLengthBytes[4];
	uint32_t bodyLength;
	char timestampBytes[4];
	uint32_t timestamp;
	uint32_t padding;

	// Video packet
	u_char codecId;
	u_char frameType;

	FlvStream(char *, int *);
	~FlvStream();

	int process(void);
	void print(void);
	void printVideo(void);
};

#endif

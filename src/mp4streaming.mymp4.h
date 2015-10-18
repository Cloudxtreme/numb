#ifndef MP4STREAMING_H
#define MP4STREAMING_H

#include "../toolkit/httpsession.h"
#include "../toolkit/mp4reader.h"

/**
  *@author spe
  */

class Mp4Streaming {
private:
	HttpSession *httpSession;
	MP4Reader *mp4reader;
	char *mp4Header;
	uint32_t mp4HeaderSize;

public:
	Mp4Streaming(HttpSession *);
	~Mp4Streaming();

	int readMetadata(void);
	uint32_t *getPtrByOffset(uint32_t);
	void copy(char *, char *, char *);
	void shiftTables(void);
	int createMp4Header(void);

//	void writeChar(unsigned char *, int);
//	void writeInt32(unsigned char *, long);
//	void atomWriteHeader(unsigned char *, struct atom_t *);
//	unsigned int atomHeaderSize(unsigned char *);
//	int atomReadHeader(FILE *, struct atom_t *);
//	void atomSkip(FILE *, struct atom_t const *);
//	void atomPrint(struct atom_t const *);
//	int atomIs(struct atom_t const *, const char *);
	int seek(void);
	int sendHeader(void);
};

#endif

#ifndef MP4STREAMING_H
#define MP4STREAMING_H

#include "../toolkit/httpsession.h"
#include "../toolkit/moov.h"

/**
  *@author spe
  */

class Mp4Streaming {
private:
	HttpSession *httpSession;

public:
	Mp4Streaming(HttpSession *);
	~Mp4Streaming();

	void writeChar(unsigned char *, int);
	void writeInt32(unsigned char *, long);
	void atomWriteHeader(unsigned char *, struct atom_t *);
	unsigned int atomHeaderSize(unsigned char *);
	int atomReadHeader(FILE *, struct atom_t *);
	void atomSkip(FILE *, struct atom_t const *);
	void atomPrint(struct atom_t const *);
	int atomIs(struct atom_t const *, const char *);
	int seek(void);
};

#endif

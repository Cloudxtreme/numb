//
// C++ Interface: httpexchange
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HTTPEXCHANGE_H
#define HTTPEXCHANGE_H

/**
	@author  <spe@>
*/
class HttpExchange {
private:
	// Eg diskCache = 1, memoryCache = 2 etc...
	int mediaType;

public:
	int inputDescriptor;
	unsigned int inputOffset;
	char *inputPtr;
	unsigned int inputPtrOffset;

	// Normally a socket
	int outputDescriptor;

	HttpExchange(int);
	~HttpExchange();

	int getInput(void);
	void setInput(int);
	int getOutput(void);
	char *getInputPtr(void);
	void setInputPtr(char *);
	int getInputOffset(void);
	void setInputOffset(int);
	int getInputPtrOffset(void);
	void setInputPtrOffset(int);
	int getMediaType(void);
	void setMediaType(int);
};

#endif

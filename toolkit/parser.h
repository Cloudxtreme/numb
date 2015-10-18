#ifndef _PARSER_H
#define _PARSER_H

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "log.h"

// Externals
extern LogError *systemLog;

#include "../toolkit/list.h"
#include "../toolkit/mystring.h"
#include "../toolkit/memory.h"

class Parser {
private:
	int parserInitialized;
	char delimiter[16];
	int delimiterLenght;
	int minAsciiDelimiter;
	int maxAsciiDelimiter;
public:
	Parser(const char *);
	Parser(int, int);
	~Parser();

	void setDelimiter(const char *_delimiter) { strncpy(delimiter, _delimiter, sizeof(delimiter)); delimiter[15] = '\0'; };
	void upperCase(char *chaine);
	void deleteWhiteSpaces(String *string);
	List<String *> *tokenizeString(char *, int);
	List<Memory<char> *> *tokenizeData(char *, int, ssize_t);
};

#endif

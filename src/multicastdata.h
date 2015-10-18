#ifndef MULTICASTDATA_H
#define MULTICASTDATA_H

#include <stdio.h>

// commandType
#define MULTICASTDATA_CMD_ADDKEY	0
#define MULTICASTDATA_CMD_REMOVEKEY	1

extern char *stringType[];

struct MulticastData {
	unsigned char commandType;
	unsigned int itemId;
	unsigned int productId;
	char countryCode[3];
	unsigned int tagId;
	unsigned char encodingFormatId;
};

#endif

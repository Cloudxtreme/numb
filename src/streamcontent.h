//
// C++ Interface: streamcontent
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef STREAMCONTENT_H
#define STREAMCONTENT_H

#include "../toolkit/httpcontent.h"
#include "../toolkit/cachemanager.h"
#include "../toolkit/hashtable.h"
#include "../src/keyhashtabletimeout.h"
#include "../src/cataloghashtabletimeout.h"
#include "../toolkit/parser.h"
#include "../src/configuration.h"
#include "../toolkit/multicastservercatalog.h"

/**
	@author  <spe@>
*/
class StreamContent : public HttpContent {
private:
	CacheManager *cacheManager;
	HashTable *keyHashtable;
	HashTable *catalogHashtable;
	KeyHashtableTimeout *keyHashtableTimeout;
	CatalogHashtableTimeout *catalogHashtableTimeout;
	Configuration *configuration;
	Parser *parser;
	bool noKeyCheck;
	MulticastServerCatalog *multicastServerCatalog;

public:
	StreamContent(Configuration *, HashTable *, KeyHashtableTimeout *, HashTable *, CatalogHashtableTimeout *, MulticastServerCatalog *, CacheManager *);
	~StreamContent();

	char *urlDecode(char *url, unsigned int urlLength);
	int getBitRate(char *);
	bool isDigitString(char *);
	int extractFileName(HttpSession *);
	CacheManager *getCacheManager(void) { return cacheManager; };
	virtual int initialize(HttpSession *);
	virtual ssize_t get(HttpSession *, char *, int);
};

#endif

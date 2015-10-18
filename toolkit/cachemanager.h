//
// C++ Interface: cachemanager
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include "../toolkit/httpsession.h"
#include "../toolkit/cachedisk.h"
#include "../toolkit/cachememory.h"
#include "../src/configuration.h"

/**
	@author  <spe@>
*/
class CacheManager {
private:
	Configuration *configuration;
	CacheDisk *cacheDisk;
	//CacheMemory *cacheMemory;

public:
	CacheManager(Configuration *);
	~CacheManager();

	int initialize(HttpSession *);
	ssize_t get(HttpSession *, char *, int);
	int remove(char *);
};

#endif

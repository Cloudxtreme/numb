//
// C++ Interface: cachedisk
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CACHEDISK_H
#define CACHEDISK_H

#include "../toolkit/httpsession.h"
#include "../toolkit/cacheobject.h"
#include "../src/configuration.h"

/**
	@author  <spe@>
*/
class CacheDisk : public CacheObject {
private:
	Configuration *configuration;

public:
	CacheDisk(Configuration *);
	~CacheDisk();

	int initialize(HttpSession *);
	ssize_t get(HttpSession *, char *, int);
	ssize_t put(HttpSession *, char *, int);
	int remove(char *);
};

#endif

//
// C++ Interface: cacheobject
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CACHEOBJECT_H
#define CACHEOBJECT_H

#include "../toolkit/httpsession.h"

/**
	@author  <spe@>
*/
class CacheObject {
public:
	CacheObject();
	virtual ~CacheObject();

	virtual int initialize(HttpSession *) { return -1; };
	virtual ssize_t get(HttpSession *, char *, int) { return -1; };
	virtual ssize_t put(HttpSession *, char *, int) { return -1; };
	virtual int remove(HttpSession *, char *) { return -1; };
};

#endif

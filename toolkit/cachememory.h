//
// C++ Interface: cachememory
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CACHEMEMORY_H
#define CACHEMEMORY_H

#include "../toolkit/hashtable.h"
#include "../toolkit/httpsession.h"
#include "../toolkit/cacheobject.h"
#include "../toolkit/cachememorygc.h"
#include "../toolkit/cachememoryobject.h"
#include "../toolkit/thread.h"

/**
	@author  <spe@>
*/

class CacheMemory : public CacheObject {
private:
	HashTable *hashTable;
	CacheMemoryGC *cacheMemoryGC;
	Thread *cacheMemoryGCThread;

public:
	CacheMemory();
	~CacheMemory();

	int initialize(HttpSession *);
	ssize_t get(HttpSession *, char *, int);
	ssize_t put(HttpSession *, char *, int);
	int remove(HttpSession *);
	HashTable *getHashTable(void);
};

#endif

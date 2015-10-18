//
// C++ Interface: cachememorygc
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CACHEMEMORYGC_H
#define CACHEMEMORYGC_H

#include "../toolkit/objectaction.h"
#include "../toolkit/hashtable.h"
#include "../toolkit/cachememoryobject.h"

/**
	@author  <spe@>
*/
class CacheMemoryGC : public ObjectAction {
private:
	HashTable *hashTable;

public:
	CacheMemoryGC(HashTable *);
	~CacheMemoryGC();

	virtual void start(void *arguments) { run(); delete this; return; }
	void run(void);
};

#endif

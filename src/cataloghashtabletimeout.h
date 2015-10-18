//
// C++ Interface: cataloghashtabletimeout
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CATALOGHASHTABLETIMEOUT_H
#define CATALOGHASHTABLETIMEOUT_H

#include "../toolkit/objectaction.h"
#include "../toolkit/hashtable.h"
#include "../toolkit/cachemanager.h"
#include "../toolkit/multicastservercatalog.h"

/**
	@author  <spe@>
*/
class CatalogHashtableTimeout : public ObjectAction {
private:
	HashTable *catalogHashtable;
	int kQueue;
	int timeout;
	CacheManager *cacheManager;
	MulticastServerCatalog *multicastServerCatalog;

public:
	CatalogHashtableTimeout(HashTable *, int, CacheManager *, MulticastServerCatalog *);
	~CatalogHashtableTimeout();

	int add(uint32_t, HashTableElt *);
	int add(uint32_t, HashTableElt *, int);
	int remove(HashTableElt *);
	void run(void *);
	virtual void start(void *arguments) { run(arguments); delete this; return; }
};

#endif

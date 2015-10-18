//
// C++ Interface: keyhashtabletimeout
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KEYHASHTABLETIMEOUT_H
#define KEYHASHTABLETIMEOUT_H

#include "../toolkit/objectaction.h"
#include "../toolkit/hashtable.h"

/**
	@author  <spe@>
*/
class KeyHashtableTimeout : public ObjectAction {
private:
	HashTable *keyHashtable;
	int kQueue;
	int timeout;

public:
	KeyHashtableTimeout(HashTable *, int);
	~KeyHashtableTimeout();

	int add(uint32_t, HashTableElt *);
	int remove(HashTableElt *);
	void run(void *);
	virtual void start(void *arguments) { run(arguments); delete this; return; }
};

#endif

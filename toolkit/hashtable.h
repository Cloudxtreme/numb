/***************************************************************************
                          hashtable.h  -  description
                             -------------------
    begin                : Mer mar 5 2003
    copyright            : (C) 2003 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <sys/types.h>

#include "log.h"

// Externals
extern LogError *systemLog;
	
#include "hashtableelt.h"
#include "hashalgorithm.h"
#include "../toolkit/mutex.h"

/**
  *@author spe
  */

class HashTable {
private:
	bool hashTableInitialized;
	int **hashTable;
	int numberOfElements;
	unsigned int hashTableSize;
	HashAlgorithm *hashAlgorithm;
	unsigned int numberOfCollisions;
	Mutex *mutex;

public:
	HashTable(HashAlgorithm *, int);
	~HashTable();

	HashTableElt *search(char *);
	HashTableElt *search(char *, uint32_t *);
	HashTableElt *add(char *, void *, uint32_t *);
	int remove(char *);
	int remove(char *, HashTableElt **);
	int remove(uint64_t, HashTableElt *);
	void purge(void);
	unsigned int getNumberOfCollisions(void) { return numberOfCollisions; };
	int getNumberOfElements(void) { return numberOfElements; };
	int **getHashtable(void) { return hashTable; };
	unsigned int getSize(void) { return hashTableSize; };
	int lock(void) { return mutex->lockMutex(); };
	int unlock(void) { return mutex->unlockMutex(); };
};

#endif

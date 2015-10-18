/***************************************************************************
                          hashtable.cpp  -  description
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

#include <errno.h>

#include "hashtable.h"

#include <sys/types.h>

HashTable::HashTable(HashAlgorithm *_hashAlgorithm, int _hashTableSize) {
	// Create hashtable

	hashTableInitialized = false;

	hashTable = (int **)calloc(_hashTableSize+1, sizeof(int *));
	if (! hashTable) {
		systemLog->sysLog(CRITICAL, "cannot allocate a hashTable object: %s", strerror(errno));
		return;
	}
	hashAlgorithm = _hashAlgorithm;
	numberOfElements = 0;
	numberOfCollisions = 0;
	hashTableSize = _hashTableSize;
	mutex = new Mutex();
	if (! mutex) {
		systemLog->sysLog(CRITICAL, "cannot create a Mutex object here, can't initialize Hashtable object");
		return;
	}
	hashTableInitialized = true;
	
	return;
}

HashTable::~HashTable() {
	if (hashTable)
		free(hashTable);
	if (mutex)
		delete mutex;

	return;
}


HashTableElt *HashTable::add(char *keyProut, void *data, uint32_t *hashPosition) {
	HashTableElt *hashTableElt;
	HashTableElt *hashTableEltPtr;
	char *keyCopy;
	unsigned int keySize;

	*hashPosition = hashAlgorithm->run(keyProut) & hashTableSize;
	hashTableEltPtr = (HashTableElt *)hashTable[*hashPosition];
	hashTableElt = new HashTableElt();
	if (! hashTableElt) {
		systemLog->sysLog(CRITICAL, "cannot allocate hashTableElt: %s", strerror(errno));
		return NULL;
	}
	hashTableElt->setData(data);
	keySize = strlen(keyProut) + 1;
	keyCopy = (char *)malloc(keySize);
	if (! keyCopy) {
		systemLog->sysLog(CRITICAL, "cannot allocate keyCopy: %s", strerror(errno));
		delete hashTableElt;
		return NULL;
	}
	strncpy(keyCopy, keyProut, keySize);
	keyCopy[keySize] = '\0';
	hashTableElt->setKey(keyCopy);
	numberOfElements++;
	if (! hashTableEltPtr) {
		hashTable[*hashPosition] = (int *)hashTableElt;
		return hashTableElt;
	}
	numberOfCollisions++;
	do {
		if (! strcmp(hashTableEltPtr->getKey(), keyProut)) {
			systemLog->sysLog(ERROR, "cannot store key '%s', key is already exists on hashtable", keyProut);
			delete hashTableElt;
			numberOfElements--;
			numberOfCollisions--;
			return NULL;
		}
		if (! hashTableEltPtr->getNext()) {
			hashTableEltPtr->setNext(hashTableElt);
			return hashTableElt;
		}
	} while ((hashTableEltPtr = hashTableEltPtr->getNext()));

	// Normally never executed
	return hashTableElt;
}

int HashTable::remove(char *key) {
	uint32_t hashPosition;
	HashTableElt *hashTableEltPtr;
	HashTableElt *hashTableEltPtrPrev;

	hashPosition = hashAlgorithm->run(key) & hashTableSize;
	hashTableEltPtr = (HashTableElt *)hashTable[hashPosition];
	if (! hashTableEltPtr) {
		systemLog->sysLog(ERROR, "cannot delete key '%s': key doesn't exist on hashtable", key);
		return -1;
	}
	hashTableEltPtrPrev = hashTableEltPtr;
	do {
		if (! strcmp(key, hashTableEltPtr->getKey())) {
			if (hashTableEltPtr == hashTableEltPtrPrev)
				hashTable[hashPosition] = (int *)hashTableEltPtr->getNext();
			else
				hashTableEltPtrPrev->setNext(hashTableEltPtr->getNext());
			delete hashTableEltPtr;
			numberOfElements--;
			return 0;
		}
		hashTableEltPtrPrev = hashTableEltPtr;
	} while ((hashTableEltPtr = hashTableEltPtr->getNext()));

	systemLog->sysLog(ERROR, "cannot delete key '%s': key doesn't exist on hashtable", key);
	return -1;
}

int HashTable::remove(char *key, HashTableElt **hashTableElt) {
	uint32_t hashPosition;
	HashTableElt *hashTableEltPtr;
	HashTableElt *hashTableEltPtrPrev;

	hashPosition = hashAlgorithm->run(key) & hashTableSize;
	hashTableEltPtr = (HashTableElt *)hashTable[hashPosition];
	if (! hashTableEltPtr) {
		systemLog->sysLog(ERROR, "cannot delete key '%s': key doesn't exist on hashtable", key);
		return -1;
	}
	hashTableEltPtrPrev = hashTableEltPtr;
	do {
		if (! strcmp(key, hashTableEltPtr->getKey())) {
			*hashTableElt = hashTableEltPtr;
			if (hashTableEltPtr == hashTableEltPtrPrev)
				hashTable[hashPosition] = (int *)hashTableEltPtr->getNext();
			else
				hashTableEltPtrPrev->setNext(hashTableEltPtr->getNext());
			delete hashTableEltPtr;
			numberOfElements--;
			return 0;
		}
		hashTableEltPtrPrev = hashTableEltPtr;
	} while ((hashTableEltPtr = hashTableEltPtr->getNext()));

	systemLog->sysLog(ERROR, "cannot delete key '%s': key doesn't exist on hashtable", key);
	return -1;
}

int HashTable::remove(uint64_t hashPosition, HashTableElt *hashtableElt) {
	HashTableElt *hashTableEltPtr;
	HashTableElt *hashTableEltPtrPrev;

	hashTableEltPtr = (HashTableElt *)hashTable[hashPosition];
	if (! hashTableEltPtr) {
		systemLog->sysLog(ERROR, "cannot delete hashPosition %d: key doesn't exist on hashtable", hashPosition);
		return -1;
	}
	hashTableEltPtrPrev = hashTableEltPtr;
	do {
		if (hashTableEltPtr == hashtableElt) {
			if (hashTableEltPtr == hashTableEltPtrPrev)
				hashTable[hashPosition] = (int *)hashTableEltPtr->getNext();
			else
				hashTableEltPtrPrev->setNext(hashTableEltPtr->getNext());
			delete hashTableEltPtr;
			numberOfElements--;
			return 0;
		}
		hashTableEltPtrPrev = hashTableEltPtr;
	} while ((hashTableEltPtr = hashTableEltPtr->getNext()));

	systemLog->sysLog(ERROR, "cannot delete, key doesn't exist on hashtable");
	return -1;
}

HashTableElt *HashTable::search(char *key) {
	uint32_t hashPosition;
	HashTableElt *hashTableEltPtr;
	bool keyFound = false;

	hashPosition = hashAlgorithm->run(key) & hashTableSize;
	hashTableEltPtr = (HashTableElt *)hashTable[hashPosition];
	if (! hashTableEltPtr)
		return NULL;
	do {
		if (! strcmp(key, hashTableEltPtr->getKey())) {
			keyFound = true;
			return hashTableEltPtr;
		}
	} while ((hashTableEltPtr = hashTableEltPtr->getNext()));

	return NULL;
}

HashTableElt *HashTable::search(char *key, uint32_t *hashPosition) {
	HashTableElt *hashTableEltPtr;
	bool keyFound = false;

	*hashPosition = hashAlgorithm->run(key) & hashTableSize;
	hashTableEltPtr = (HashTableElt *)hashTable[*hashPosition];
	if (! hashTableEltPtr)
		return NULL;
	do {
		if (! strcmp(key, hashTableEltPtr->getKey())) {
			keyFound = true;
			return hashTableEltPtr;
		}
	} while ((hashTableEltPtr = hashTableEltPtr->getNext()));

	return NULL;
}

void HashTable::purge(void) {
	uint32_t hashPosition = 0;
	HashTableElt *hashTableEltPtr;
	HashTableElt *hashTableEltPtr2;

	while (hashPosition < hashTableSize) {
		if ((hashTableEltPtr = (HashTableElt *)hashTable[hashPosition])) {
			do {
				hashTableEltPtr2 = hashTableEltPtr;
				delete hashTableEltPtr2;
			} while ((hashTableEltPtr = hashTableEltPtr->getNext()));
			hashTable[hashPosition] = NULL;
		}
		hashPosition++;
	}
	numberOfElements = 0;

	return;
}

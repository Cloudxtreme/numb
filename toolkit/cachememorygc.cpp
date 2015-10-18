//
// C++ Implementation: cachememorygc
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <unistd.h>

#include "cachememorygc.h"

// Memory Garbage Collector for the memory hashtable

CacheMemoryGC::CacheMemoryGC(HashTable *_hashTable) {
	hashTable = _hashTable;

	return;
}

CacheMemoryGC::~CacheMemoryGC() {
	return;
}

// Timeout for garbage collect datas in seconds
static int garbageCollectionTimeout = 30; // Trying with 5 minutes only for the moment

static void CacheMemoryGCCallBack(void *key, void *value, void *userData) {
	HashTable *hashTable = (HashTable *)userData;
	struct CacheMemoryObject *cacheMemoryObject;
	struct timeval actualTime;
	int returnCode;
	
	fprintf(stderr, "[DEBUG] Garbage collecting process engaged\n");

	if ((! key) || (! value) || (! hashTable)) {
		systemLog->sysLog(ERROR, "cannot garbage collect NULL objects\n");
		return;
	}

	hashTable->lock();

	// Check the actual time
	
	returnCode = gettimeofday(&actualTime, NULL);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot gettimeofday for memory garbage collection: %s\n", strerror(errno));
		return;
	}

	// Now we can verify the timestamp and garbage collect if it's necessary
	cacheMemoryObject = (CacheMemoryObject *)value;
	
fprintf(stderr, "[DEBUG] timeVal is %d\n", cacheMemoryObject->timeVal.tv_sec);
	if (cacheMemoryObject->timeVal.tv_sec + garbageCollectionTimeout <= actualTime.tv_sec) {
		fprintf(stderr, "[DEBUG] Garbage collect the object '%X' in memory, timeout occured\n", key); 
		systemLog->sysLog(NOTICE, "garbage collect video 0x%X, timeout of %d occured\n", cacheMemoryObject->object, garbageCollectionTimeout);
		hashTable->deleteKey(key);
		free(key);
		free(cacheMemoryObject->object);
		free(cacheMemoryObject);
	}

	hashTable->unlock();

	return;
}

void CacheMemoryGC::run(void) {
	for (;;) {
		hashTable->parseAllKeys(CacheMemoryGCCallBack, hashTable);
		sleep(30);
	}

	// Never executed
	return;
}

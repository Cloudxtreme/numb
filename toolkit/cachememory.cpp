//
// C++ Implementation: cachememory
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "cachememory.h"

CacheMemory::CacheMemory() {
	hashTable = new HashTable();
	if (! hashTable) {
		systemLog->sysLog(CRITICAL, "cannot create an HashTable object: %s", strerror(errno));
		return;
	}
	cacheMemoryGC = new CacheMemoryGC(hashTable);
	if (! cacheMemoryGC) {
		systemLog->sysLog(CRITICAL, "cannot create a CacheMemoryGC object: %s", strerror(errno));
		return;
	}
	cacheMemoryGCThread = new Thread(cacheMemoryGC);
	if (! cacheMemoryGCThread) {
		systemLog->sysLog(CRITICAL, "cannot create a Thread object: %s", strerror(errno));
		return;
	}
	// Executing the garbage collector
	cacheMemoryGCThread->createThread(NULL);

	return;
}

CacheMemory::~CacheMemory() {
	if (hashTable)
		delete hashTable;

	return;
}

int CacheMemory::initialize(HttpSession *httpSession) {
	struct CacheMemoryObject *cacheMemoryObject;
	int returnCode;
	char *key;

#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] CacheMemory::initialize\n");
#endif
	hashTable->lock();
#ifdef DEBUGOUTPUT
	fprintf(stderr, "search key '%s' in hashtable\n", httpSession->videoName);
#endif
	cacheMemoryObject = (struct CacheMemoryObject *)hashTable->searchKey(httpSession->videoName);
	if (! cacheMemoryObject) {
#ifdef DEBUGOUTPUT
		fprintf(stderr, "[DEBUG] object not found in hashtable\n");
#endif
		cacheMemoryObject = (struct CacheMemoryObject *)malloc(sizeof(struct CacheMemoryObject));
		if (! cacheMemoryObject) {
			systemLog->sysLog(CRITICAL, "[%d] cannot malloc: %s", httpSession->httpExchange->getOutput(), strerror(errno));
			hashTable->unlock();
			return -1;
		}
		returnCode = gettimeofday(&cacheMemoryObject->timeVal, NULL);
		if (returnCode < 0) {
			systemLog->sysLog(ERROR, "[%d] problem during gettimeofday: %s", httpSession->httpExchange->getOutput(), strerror(errno));
			free(cacheMemoryObject);
			hashTable->unlock();
			return -1;
		}
		cacheMemoryObject->object = (char *)malloc(httpSession->sourceFileStat.st_size);
		if (! cacheMemoryObject->object) {
			systemLog->sysLog(CRITICAL, "[%d] cannot malloc: %s", httpSession->httpExchange->getOutput(), strerror(errno));
			hashTable->unlock();
			free(cacheMemoryObject);
			return -1;
		}
		httpSession->httpExchange->setInputPtr(cacheMemoryObject->object);
		key = (char *)malloc(strlen(httpSession->videoName) + 1);
		if (! key) {
			systemLog->sysLog(CRITICAL, "[%d] cannot allocate the key for hashtable insertion: %s", httpSession->httpExchange->getOutput(), strerror(errno));
			free(cacheMemoryObject->object);
			free(cacheMemoryObject);
			return -1;
		}
		strcpy(key, httpSession->videoName);
		hashTable->insertKey(key, cacheMemoryObject);
		hashTable->unlock();

		return -1;
	}
	returnCode = gettimeofday(&cacheMemoryObject->timeVal, NULL);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "[%d] cannot gettimeofday and modify last access time: %s", strerror(errno));
	}
	fprintf(stderr, "[DEBUG] get timeVal is %d\n", cacheMemoryObject->timeVal.tv_sec);
	httpSession->fileSize = cacheMemoryObject->objectSize;
	httpSession->httpExchange->setInputPtr(cacheMemoryObject->object);
	httpSession->httpExchange->setMediaType(2);
#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] inputPtr is %X, and cacheMemoryObject is %X\n", httpSession->httpExchange->getInputPtr(), cacheMemoryObject->object);
#endif
	hashTable->unlock();

	return 0;
}

ssize_t CacheMemory::get(HttpSession *httpSession, char *buffer, int bufferSize) {
	ssize_t bytesToRead;
	size_t copySize;

#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] Pointer offset is %d\n", httpSession->httpExchange->getInputPtrOffset());
	fprintf(stderr, "[DEBUG] FileSize is %d\n", httpSession->fileSize);
#endif

	if (httpSession->fileSize <= 0)
		return 0;

	if (httpSession->fileSize < bufferSize)
		copySize = httpSession->fileSize;
	else
		copySize = bufferSize;

	bcopy(httpSession->httpExchange->getInputPtr() + httpSession->httpExchange->getInputPtrOffset(), buffer, copySize);

	httpSession->httpExchange->setInputPtrOffset(httpSession->httpExchange->getInputPtrOffset() + copySize);

#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] copySize is %d\n", copySize);
#endif

	return copySize;
}

ssize_t CacheMemory::put(HttpSession *httpSession, char *buffer, int bufferSize) {
	// XXX sanity checks here...
	bcopy(buffer, &httpSession->httpExchange->getInputPtr()[httpSession->httpExchange->getInputPtrOffset()], bufferSize);

	httpSession->httpExchange->setInputPtrOffset(httpSession->httpExchange->getInputPtrOffset() + bufferSize);

	return bufferSize;
}

int CacheMemory::remove(HttpSession *httpSession) {
	struct CacheMemoryObject *cacheMemoryObject;

	hashTable->lock();
	cacheMemoryObject = (struct CacheMemoryObject *)hashTable->searchKey(httpSession->videoName);
	if (! cacheMemoryObject) {
		hashTable->unlock();
		return -1;
	}
	hashTable->deleteKey(httpSession->videoName);
#ifdef DEBUGOUTPUT
	fprintf(stderr, "[DEBUG] Removing object '%s' from memory\n", httpSession->videoName);
#endif
	if (cacheMemoryObject->object)
		free(cacheMemoryObject->object);
	free(cacheMemoryObject);
	hashTable->unlock();

	return 0;
}

HashTable *CacheMemory::getHashTable(void) {
	return hashTable;
}

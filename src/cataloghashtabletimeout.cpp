//
// C++ Implementation: cataloghashtabletimeout
//
// Description: 
//
//
// Author:  <spebsd@gmail.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "cataloghashtabletimeout.h"
#include "../src/multicastpacketcatalog.h"

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

CatalogHashtableTimeout::CatalogHashtableTimeout(HashTable *_catalogHashtable, int _timeout, CacheManager *_cacheManager, MulticastServerCatalog *_multicastServerCatalog) {
	if (! _catalogHashtable) {
		systemLog->sysLog(CRITICAL, "keyHashtable is NULL, something is terribly wrong");
		return;
	}
	kQueue = kqueue();
	if (kQueue < 0) {
		systemLog->sysLog(CRITICAL, "cannot initialize kQueue for KeyHashtableTimeout object: %s", strerror(errno));
		return;
	}
	timeout = _timeout;
	catalogHashtable = _catalogHashtable;
	cacheManager = _cacheManager;
	multicastServerCatalog = _multicastServerCatalog;

	return;
}


CatalogHashtableTimeout::~CatalogHashtableTimeout() {
	if (kQueue)
		close(kQueue);

	return;
}

int CatalogHashtableTimeout::add(uint32_t hashPosition, HashTableElt *hashtableElt) {
	struct kevent kChange;
	int returnCode;

	EV_SET(&kChange, (uintptr_t)hashtableElt, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, timeout, (void *)hashPosition);
	returnCode = kevent(kQueue, &kChange, 1, NULL, 0, NULL);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot add a timeout event: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int CatalogHashtableTimeout::add(uint32_t hashPosition, HashTableElt *hashtableElt, int objectTimeout) {
	struct kevent kChange;
	int returnCode;

	EV_SET(&kChange, (uintptr_t)hashtableElt, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, objectTimeout, (void *)hashPosition);
	returnCode = kevent(kQueue, &kChange, 1, NULL, 0, NULL);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot add a timeout event: %s", strerror(errno));
		return -1;
	}

	return 0;
}

/*int CatalogHashtableTimeout::renew() {

}*/

int CatalogHashtableTimeout::remove(HashTableElt *hashtableElt) {
	struct kevent kChange;
	int returnCode;

	EV_SET(&kChange, (uintptr_t)hashtableElt, EVFILT_TIMER, EV_DELETE | EV_CLEAR, 0, 0, 0);
	returnCode = kevent(kQueue, &kChange, 1, NULL, 0, NULL);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot delete a timeout event: %s", strerror(errno));
		return -1;
	}

	return 0;
}

void CatalogHashtableTimeout::run(void *arguments) {
	int numberOfEvents;
	struct kevent kEvents;
	int returnCode;
	HashTableElt *hashtableElt;
	char *key;
	size_t bufferLength;
	char *buffer;

	if (kQueue < 0)
		return;
	for (;;) {
		numberOfEvents = kevent(kQueue, NULL, 0, &kEvents, 1, NULL);
		if (numberOfEvents < 0) {
			systemLog->sysLog(ERROR, "error while receiving an event from kqueue: %s", strerror(errno));
			continue;
			// Remplacer ca par l'extraction de l'evenement en erreur et continuer ensuite...
		}
		switch (kEvents.filter) {
			case EVFILT_TIMER:
				catalogHashtable->lock();

				hashtableElt = (HashTableElt *)kEvents.ident;
#ifdef DEBUGOUTPUT
				fprintf(stderr, "[DEBUG] Removing object name %s from cache, timeout occured !\n", hashtableElt->getKey());
#endif				
				if (hashtableElt->getData())
					free(hashtableElt->getData());

				key = strdup(hashtableElt->getKey());
				if (! key) {
					systemLog->sysLog(ERROR, "cannot get key for erasing cache file: %s", strerror(errno));
					systemLog->sysLog(ERROR, "file will not be delete from disk/memory, something is terribly wrong !");
				}

				// Sending CATALOGDEL command to the cluster for removing key
				bufferLength = 2+strlen(key);
				buffer = new char[bufferLength+1];
				if (! buffer) {
					systemLog->sysLog(CRITICAL, "cannot allocate buffer object with %d bytes: %s", bufferLength+1, strerror(errno));
					free(key);
					catalogHashtable->unlock();
					continue;
				}
				snprintf(buffer, bufferLength+1, "%d\n%s", CATALOGDEL, key);
#ifdef DEBUGCATALOG
				systemLog->sysLog(DEBUG, "sending multicast packet on network: #%s#", buffer);
#endif
				multicastServerCatalog->sendPacket(buffer, bufferLength);
				delete buffer;

				systemLog->sysLog(NOTICE, "removing object name %s from catalog hashtable", key);
				returnCode = catalogHashtable->remove((uint64_t)kEvents.udata, hashtableElt);
				if (returnCode < 0) {
					systemLog->sysLog(ERROR, "cannot remove a key from the hashtable :(");
					free(key);
					catalogHashtable->unlock();
					continue;
				}
				catalogHashtable->unlock();
				if (key) {
					returnCode = cacheManager->remove(key);
					if (returnCode)
						systemLog->sysLog(ERROR, "cannot remove object name %s from cache: %s", key, strerror(errno));
					free(key);
				}

				break;
			default:
				systemLog->sysLog(ERROR, "filter %d is unknown !", kEvents.filter);
				break;
		}
	}
}

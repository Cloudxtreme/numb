//
// C++ Implementation: keyhashtabletimeout
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "keyhashtabletimeout.h"

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

KeyHashtableTimeout::KeyHashtableTimeout(HashTable *_keyHashtable, int _timeout) {
	if (! _keyHashtable) {
		systemLog->sysLog(CRITICAL, "keyHashtable is NULL, something is terribly wrong");
		return;
	}
	kQueue = kqueue();
	if (kQueue < 0) {
		systemLog->sysLog(CRITICAL, "cannot initialize kQueue for KeyHashtableTimeout object: %s", strerror(errno));
		return;
	}
	timeout = _timeout;
	keyHashtable = _keyHashtable;

	return;
}


KeyHashtableTimeout::~KeyHashtableTimeout() {
	if (kQueue)
		close(kQueue);

	return;
}

int KeyHashtableTimeout::add(uint32_t hashPosition, HashTableElt *hashtableElt) {
	struct kevent kChange;
	int returnCode;

	EV_SET(&kChange, (uintptr_t)hashtableElt, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, timeout, (void *)hashPosition);
	returnCode = kevent(kQueue, &kChange, 1, NULL, 0, NULL);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot add a timeout event: %s", strerror(errno));
		// XXX patch
		exit(-1);
		return -1;
	}

	return 0;
}

int KeyHashtableTimeout::remove(HashTableElt *hashtableElt) {
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

void KeyHashtableTimeout::run(void *arguments) {
	int numberOfEvents;
	struct kevent kEvents;
	int returnCode;
	HashTableElt *hashtableElt;

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
				hashtableElt = (HashTableElt *)kEvents.ident;
#ifdef DEBUGOUTPUT
				fprintf(stderr, "[DEBUG] Removing key %s, timeout occured !\n", hashtableElt->getKey());
#endif
				if (hashtableElt->getData())
					free(hashtableElt->getData());
				keyHashtable->lock();
				returnCode = keyHashtable->remove((uint64_t)kEvents.udata, hashtableElt);
				keyHashtable->unlock();
				if (returnCode < 0) {
					systemLog->sysLog(ERROR, "cannot remove a key from the hashtable :(");
					continue;
				}
				break;
			default:
				systemLog->sysLog(ERROR, "filter %d is unknown !", kEvents.filter);
				break;
		}
	}
}

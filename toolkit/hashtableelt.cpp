//
// C++ Implementation: hashtableelt
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <stdio.h>
#include <stdlib.h>

#include "hashtableelt.h"

HashTableElt::HashTableElt() {
	next = NULL;
	key = NULL;
	data = NULL;
	
	return;
}


HashTableElt::~HashTableElt() {
	if (key)
		free(key);

	return;
}

void HashTableElt::setData(void *_data) {
	data = _data;
}

void *HashTableElt::getData(void) {
	return data;
}

void HashTableElt::setKey(char *_key) {
	key = _key;
}

char *HashTableElt::getKey(void) {
	return key;
}

void HashTableElt::setNext(HashTableElt *_hashTableElt) {
	next = _hashTableElt;
}

HashTableElt *HashTableElt::getNext(void) {
	return next;
}

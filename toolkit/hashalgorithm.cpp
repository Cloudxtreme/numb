//
// C++ Implementation: hashalgorithm
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "hashalgorithm.h"

HashAlgorithm::HashAlgorithm(unsigned int _algorithmType) {
	hashAlgorithmInitialized = false;
	if (_algorithmType > ALGO_END) {
		systemLog->sysLog(ERROR, "algorithm type (%d) is unknown", _algorithmType);
		systemLog->sysLog(ERROR, "cannot initialize HashAlgorithm object");
		return;
	}
	algorithmType = _algorithmType;
	hashAlgorithmInitialized = true;

	return;
}


HashAlgorithm::~HashAlgorithm() {
}

uint32_t HashAlgorithm::run(const char *key) {
	if (hashAlgorithmInitialized == false) {
		systemLog->sysLog(ERROR, "HashAlgorithm object is not initialized properly");
		return 0;
	}
	switch (algorithmType) {
		case ALGO_PAULHSIEH:
			return SuperFastHash(key);
			break;
	}

	// Never executed normally
	return 0;
}

uint32_t HashAlgorithm::SuperFastHash (const char *data) {
	uint32_t hash, tmp, len;
	int rem;

	if (! data) {
		systemLog->sysLog(ERROR, "data is NULL. Cannot hash a NULL char * pointer");
		return 0;
	}

	len = strlen(data);
	hash = len;

	rem = len & 3;
	len >>= 2;

	for (;len > 0; len--) {
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (uint16_t);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3:
			hash += get16bits (data);
			hash ^= hash << 16;
			hash ^= data[sizeof (uint16_t)] << 18;
			hash += hash >> 11;
			break;
		case 2:
			hash += get16bits (data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
		case 1:
			hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
			break;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}


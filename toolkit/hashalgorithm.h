//
// C++ Interface: hashalgorithm
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HASHALGORITHM_H
#define HASHALGORITHM_H

#include <sys/types.h>

#include "log.h"

#define get16bits(d) (*((const uint16_t *) (d)))
#define ALGO_PAULHSIEH 1
#define ALGO_END 2

// Externals
extern LogError *systemLog;

/**
	@author  <spe@>
*/
class HashAlgorithm {
private:
	bool hashAlgorithmInitialized;
	uint32_t SuperFastHash (const char *data);
	unsigned int algorithmType;

public:
	HashAlgorithm(unsigned int);
	~HashAlgorithm();

	uint32_t run(const char *);
};

#endif

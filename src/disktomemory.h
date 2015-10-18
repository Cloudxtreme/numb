//
// C++ Interface: disktomemory
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef DISKTOMEMORY_H
#define DISKTOMEMORY_H

#include "../toolkit/objectaction.h"
#include "../toolkit/cachememory.h"

/**
	@author  <spe@>
*/
class DiskToMemory : public ObjectAction {
private:
	CacheMemory *cacheMemory;

public:
	DiskToMemory(CacheMemory *);
	virtual ~DiskToMemory();

	void copy(void *);
	virtual void start(void *arguments) { copy(arguments); delete(this); return; };
};

#endif

/***************************************************************************
                          cachefile.h  -  description
                             -------------------
 ***************************************************************************/

#ifndef CACHEFILE_H
#define CACHEFILE_H

#include "../toolkit/log.h"
#include "../toolkit/thread.h"
#include "../toolkit/objectaction.h"
#include "cachefile.h"

/**
  *@author spe
  */

class CacheFile : public ObjectAction {
public: 
	CacheFile();
	~CacheFile();

	int cpFile(int, int, unsigned int *);
	void copyFile(void *arguments);
	virtual void start(void *arguments) { copyFile(arguments); delete (int *)arguments; pthread_exit(NULL); };
};

#endif

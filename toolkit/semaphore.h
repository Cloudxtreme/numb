/***************************************************************************
                          semaphore.h  -  description
                             -------------------
    begin                : Sam nov 9 2002
    copyright            : (C) 2002 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <time.h>
#ifdef PSEM
#include <semaphore.h>
#else
#include <sys/sem.h>
#endif

#include "log.h"
extern LogError *systemLog;

#ifndef PSEM
#define KEY (6478)
#endif

/**
  *@author spe
  */

class Semaphore {
private:
#ifdef PSEM
	sem_t semaphore;
#else
	int semaphore;
	union semun {
		int value;
		struct semid_ds *buffer;
		unsigned short *array;
	} argument;
#endif
	int semaphoreInitialized;
public: 
	Semaphore();
	Semaphore(int);
	~Semaphore();

	int semaphoreWait(void);
	int semaphoreTryWait(void);
	int semaphorePost(void);
	int semaphoreValue(void);
};

#endif

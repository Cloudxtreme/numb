/***************************************************************************
                          thread.h  -  description
                             -------------------
    begin                : Jeu nov 7 2002
    copyright            : (C) 2002 by Sebastien Petit
    email                : spe@selectbourse.net
 ***************************************************************************/

#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include "objectaction.h"
#include "semaphore.h"

class Thread
{
private:
  ObjectAction *objectAction;
  pthread_t pthreadId;
  pthread_attr_t pthreadAttributes;
  bool pthreadInitialized;
  Semaphore *threadSemaphore;
  int number;
public:
  Thread(ObjectAction *);
  Thread(ObjectAction *, int);
  ~Thread(void);
  int createThread(void *);
  int getNumber(void) { return number; };
};

#endif

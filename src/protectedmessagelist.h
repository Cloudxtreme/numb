#ifndef PROTECTEDMESSAGELIST_H
#define PROTECTEDMESSAGELIST_H

#include "../toolkit/log.h"
#include "../toolkit/list.h"
#include "../toolkit/mutex.h"
#include "../toolkit/semaphore.h"

/**
  *@author spe
  */

class ProtectedMessageList {
private:
public:
  int maxListSize;
  Mutex *messageListMutex;
  Semaphore *messageListSemaphore;
  List<int *> *messageList;
  int numberOfMessages;

  Mutex *messageFullMutex;
  time_t lastMessageDate;
  int nNumberSkipMessage;

  ProtectedMessageList(int);
  ~ProtectedMessageList();
  int getMaxListSize(void) { return maxListSize; };
  int addElement(int *);
  int removeFirst(void);
  int *getFirstElement(void);
  bool isListFull(void);
};

#endif

#include "protectedmessagelist.h"

extern LogError *systemLog;

ProtectedMessageList::ProtectedMessageList(int _maxListSize){
  maxListSize = _maxListSize;
  messageList = new List<int *>();
  messageList->setDestroyData(2);
  messageListMutex = new Mutex();
  messageListSemaphore = new Semaphore();
  numberOfMessages = 0;

  messageFullMutex = new Mutex();
  lastMessageDate = 0;
  nNumberSkipMessage = 0;

  return;
}

ProtectedMessageList::~ProtectedMessageList() {
  if (messageListMutex)
    delete messageListMutex;
  messageListMutex = NULL;
  if (messageListSemaphore)
    delete messageListSemaphore;
  messageListSemaphore = NULL;
  if (messageFullMutex)
    delete messageFullMutex;
  messageFullMutex = NULL;
  numberOfMessages = 0;

  maxListSize = 0;
  lastMessageDate = 0;
  nNumberSkipMessage = 0;

  return;
}

int ProtectedMessageList::addElement(int *socketDescriptor) {
  int returnCode;
  void *returnPtr;

  try {
    returnCode = messageListMutex->lockMutex();
    if (returnCode < 0)
      throw(-1);
    returnPtr = messageList->addElement(socketDescriptor);
    if (! returnPtr)
      throw(-1);
    returnCode = messageListSemaphore->semaphorePost();
    if (returnCode < 0)
      throw(-1);
    returnCode = messageListMutex->unlockMutex();
    if (returnCode < 0)
      throw(-1);
  }
  catch (...) {
    systemLog->sysLog(ERROR, "can't add an socketDescriptor(int) object on the ProtectedMessageList");
    return -1;
  }
  numberOfMessages++;

  return 0;
}

int ProtectedMessageList::removeFirst(void) {
  int returnCode;

  try {
    returnCode = messageListMutex->lockMutex();
    if (returnCode < 0)
      throw(-1);
    returnCode = messageList->removeFirst();
    if (returnCode < 0)
      throw(-1);
    returnCode = messageListMutex->unlockMutex();
    if (returnCode < 0)
      throw(-1);
  }
  catch (...) {
    systemLog->sysLog(ERROR, "can't remove the first element object ont the ProtectedMessageList");
    return -1;
  }
  numberOfMessages--;

  return 0;
}

int *ProtectedMessageList::getFirstElement(void) {
	int returnCode;
	int *element;

  try {
    returnCode = messageListMutex->lockMutex();
    if (returnCode < 0)
      throw(-1);
    element = messageList->getFirstElement();
    if (element == NULL)
      throw(-1);
    returnCode = messageListMutex->unlockMutex();
    if (returnCode < 0)
      throw(-1);
  }
  catch (...) {
    systemLog->sysLog(ERROR, "can't remove the first element object ont the ProtectedMessageList");
    return NULL;
  }

  return element;
}

bool ProtectedMessageList::isListFull(void) {
  if (numberOfMessages == maxListSize)
    return true;
  else
    return false;
}

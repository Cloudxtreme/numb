#include <unistd.h>
#include <errno.h>
#include "thread.h"
#include "log.h"

Thread::Thread(ObjectAction *_objectAction) {
	int returnCode;

	pthreadInitialized = false;
	if (! _objectAction) {
		systemLog->sysLog(LOG_ERR, "cannot create thread object, objectAction is NULL\n");
		return;
	}
	objectAction = _objectAction;
	returnCode = pthread_attr_init(&pthreadAttributes);
	if (returnCode) {
		switch (returnCode) {
			case ENOMEM:
				systemLog->sysLog(LOG_ERR, "cannot initialize pthreadAttributes, out of memory\n");
				break;
			default:
				systemLog->sysLog(LOG_ERR, "the error code is unknown\n");
				break;
		}
		return;
	}
	returnCode = pthread_attr_setdetachstate(&pthreadAttributes, PTHREAD_CREATE_DETACHED);
	if (returnCode == EINVAL) {
		systemLog->sysLog(LOG_ERR, "the detached state type is invalid\n");
		return;
	}
	threadSemaphore = new Semaphore();
	if (! threadSemaphore) {
		systemLog->sysLog(LOG_ERR, "threadSemaphore is NULL, cannot initialize thread\n");
		return;
	}
	pthreadInitialized = true;

	return;
}

Thread::Thread(ObjectAction *_objectAction, int _number) {
	int returnCode;

	pthreadInitialized = false;
	if (! _objectAction) {
		systemLog->sysLog(LOG_ERR, "cannot create thread object, objectAction is NULL\n");
		return;
	}
	objectAction = _objectAction;
	returnCode = pthread_attr_init(&pthreadAttributes);
	if (returnCode) {
		switch (returnCode) {
			case ENOMEM:
				systemLog->sysLog(LOG_ERR, "cannot initialize pthreadAttributes, out of memory\n");
				break;
			default:
				systemLog->sysLog(LOG_ERR, "the error code is unknown\n");
				break;
		}
		return;
	}
	returnCode = pthread_attr_setdetachstate(&pthreadAttributes, PTHREAD_CREATE_DETACHED);
	if (returnCode == EINVAL) {
		systemLog->sysLog(LOG_ERR, "the detached state type is invalid\n");
		return;
	}
	threadSemaphore = new Semaphore();
	if (! threadSemaphore) {
		systemLog->sysLog(LOG_ERR, "threadSemaphore is NULL, cannot initialize thread\n");
		return;
	}
	number = _number;
	pthreadInitialized = true;

	return;
}

Thread::~Thread(void) {
	if (pthreadInitialized)
		pthread_attr_destroy(&pthreadAttributes);

	if (threadSemaphore) {
		delete threadSemaphore;
	}
	pthreadInitialized = false;
	objectAction = NULL;

	return;
}

// Wrapping function for calling pthread_create()
static void *threadFunction(void *threadArguments) {
	Semaphore *threadSemaphore = (Semaphore *)((int **)threadArguments)[0];
	ObjectAction *objectAction = (ObjectAction *)((int **)threadArguments)[1];
	void *arguments = ((int **)threadArguments)[2];

	threadSemaphore->semaphorePost();

	objectAction->start(arguments);

	return NULL;
}

int Thread::createThread(void *arguments) {
	int returnCode;
	int *threadArguments[3];

	if (! pthreadInitialized) {
		systemLog->sysLog(LOG_ERR, "the posix thread is not initialized correctly, cannot use it");
		return EINVAL;
	}

	threadArguments[0] = (int *)threadSemaphore;
	threadArguments[1] = (int *)objectAction;
	threadArguments[2] = (int *)arguments;

	returnCode = pthread_create(&pthreadId, &pthreadAttributes, threadFunction, threadArguments);
	
	if (returnCode) {
		switch (returnCode) {
		case EAGAIN:
			systemLog->sysLog(LOG_ERR, "the system lacked the necessary resources to create another thread, limit exceeded ?\n");
			break;
		case EINVAL:
			systemLog->sysLog(LOG_ERR, "the pthreadAttributes argument is invalid\n");
			break;
		default:
			systemLog->sysLog(LOG_ERR, "the error code is unknown: %s\n", strerror(errno));
			break;
		}
		return -1;
	}

	// Wait the real creation of the thread for synchronization
	threadSemaphore->semaphoreWait();

	return returnCode;
}

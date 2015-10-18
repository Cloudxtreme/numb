/***************************************************************************
                          semaphore.cpp  -  description
                             -------------------
    begin                : Sam nov 9 2002
    copyright            : (C) 2002 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

#include <errno.h>
#include "../toolkit/semaphore.h"

Semaphore::Semaphore() {
	int returnCode = 0;

	semaphoreInitialized = 0;
#ifdef PSEM
	returnCode = sem_init(&semaphore, 0, 0);
	if (returnCode < 0) {
		systemLog->sysLog(CRITICAL, "cannot initialize semaphore: %s\n", strerror(errno));
		return;
	}
#else
	argument.value = 0;
	returnCode = semget(IPC_PRIVATE, 1, 0600);
	if (returnCode < 0) {
		systemLog->sysLog(CRITICAL, "cannot initialize semaphore: %s\n", strerror(errno));
		return;
	}
#endif
	semaphoreInitialized = 1;

	return;
}

Semaphore::Semaphore(int semaphoreValue) {
	int returnCode;

	semaphoreInitialized = 0;
#ifdef PSEM
	returnCode = sem_init(&semaphore, 0, semaphoreValue);
	if (returnCode < 0) {
		systemLog->sysLog(CRITICAL, "cannot initialize semaphore: %s\n", strerror(errno));
		return;
	}
#else
	argument.value = 0;
	returnCode = semget(KEY, semaphoreValue, 0600 | IPC_CREAT);
	if (returnCode < 0) {
		systemLog->sysLog(CRITICAL, "cannot initialize semaphore: %s\n", strerror(errno));
		return;
	}
#endif
	semaphoreInitialized = 1;

	return;
}

Semaphore::~Semaphore(){
#ifdef PSEM
	if (semaphoreInitialized)
		sem_destroy(&semaphore);
#endif
	semaphoreInitialized = 0;

	return;
}

int Semaphore::semaphoreWait(void) {
	int returnCode;
#ifndef PSEM
	struct sembuf operation;
#endif

	if (! semaphoreInitialized) {
		systemLog->sysLog(ERROR, "the semaphore is not initialized correctly, cannot use it\n");
		return EINVAL;
	}
#ifdef PSEM
	returnCode = sem_wait(&semaphore);
#else
	operation.sem_num = 0;
	operation.sem_op = -1;
	operation.sem_flg = 0;
	returnCode = semop(semaphore, &operation, 1);
#endif
	if (returnCode)
		systemLog->sysLog(ERROR, "cannot wait on semaphore: %s\n", strerror(errno));

	return returnCode;
}

/*
 * This method try to wait on the semaphore
 * zero is returned if waiting on semaphore is successfull
 * EAGAIN is returned if the semaphore valued is 0
 * EINVAL is returned if the semaphore is not initialized correctly.
 * otherwise an error code is returned
 */
int Semaphore::semaphoreTryWait(void) {
	int returnCode;
#ifndef PSEM
	struct sembuf operation;
#endif

	if (! semaphoreInitialized) {
		systemLog->sysLog(ERROR, "the semaphore is not initialized correctly, cannot try to use it");
		return EINVAL;
	}
#ifdef PSEM
	returnCode = sem_trywait(&semaphore);
#else
	operation.sem_num = 0;
	operation.sem_op = -1;
	operation.sem_flg = IPC_NOWAIT;
	returnCode = semop(semaphore, &operation, 1);
#endif
	if (returnCode)
		systemLog->sysLog(ERROR, "cannot try to wait on semaphore: %s", strerror(errno));

	return returnCode;
}

int Semaphore::semaphorePost(void) {
	int returnCode;
#ifndef PSEM
	struct sembuf operation;
#endif

	if (! semaphoreInitialized) {
		systemLog->sysLog(ERROR, "the semaphore is not initialized correctly, cannot post on it");
		return EINVAL;
	}
#ifdef PSEM
	returnCode = sem_post(&semaphore);
#else
	operation.sem_num = 0;
	operation.sem_op = 1;
	operation.sem_flg = 0;
	returnCode = semop(semaphore, &operation, 1);
#endif
	if (returnCode < 0)
		systemLog->sysLog(ERROR, "cannot post on semaphore: %s", strerror(errno));

	return returnCode;
}

int Semaphore::semaphoreValue(void) {
	int returnCode;
	int returnValue;

	if (! semaphoreInitialized) {
		systemLog->sysLog(ERROR, "the semaphore is not initialized correctly, cannot post on it");
		return EINVAL;
	}

	returnCode = sem_getvalue(&semaphore, &returnValue);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot get value of the semaphore: %s", strerror(errno));
		return -1;
	}

	return returnValue;
}

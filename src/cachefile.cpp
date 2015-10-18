/***************************************************************************
                          cachefile.cpp  -  description
                             -------------------
    begin                : Tue Jun 24 2006
    copyright            : (C) 2006 by spe
    email                : spebsd@gmail.com
 ***************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "cachefile.h"

extern LogError *systemLog;

CacheFile::CacheFile(void) {
	return;
}

CacheFile::~CacheFile() {
	return;
}

int CacheFile::cpFile(int source, int destination, unsigned int *fileSize) {
	ssize_t sizeRead = 1;
	ssize_t sizeWrite;
	char buffer[132768];

	*fileSize = 0;
	while (sizeRead) {
		sizeRead = read(source, buffer, sizeof(buffer));
 		sizeWrite = write(destination, buffer, sizeRead);
		if ((sizeWrite < 0) || (sizeWrite != sizeRead))
			return -1;
 		(*fileSize) += sizeWrite; 
	}

	return 0;

}

void CacheFile::copyFile(void *arguments) {
	int fileDescriptor;
	int nfsFileDescriptor;
	int returnCode;
	unsigned int fileSize;
	char *videoNameFilePath = (char *)((int *)arguments)[0];
	char *videoNameNfsFilePath = (char *)((int *)arguments)[1];
	int *numberOfCopyThread = (int *)((int *)arguments)[2];
	int videoNameFilePathLength = strlen(videoNameFilePath);
	char *videoNameTmpFilePath;

	videoNameTmpFilePath = (char *)malloc(videoNameFilePathLength+5);
	if (! videoNameTmpFilePath) {
		systemLog->sysLog(ERROR, "error, cannot allocate memory: %s", strerror(errno));
		if (videoNameFilePath)
			free(videoNameFilePath);
		if (videoNameNfsFilePath)
			free(videoNameNfsFilePath);
		return;
	}
	snprintf(videoNameTmpFilePath, videoNameFilePathLength + 5, "%s.tmp", videoNameFilePath);
#ifdef DEBUG
	systemLog->sysLog(INFO, "%s doesn't exist on the cache directory, caching file from NFS", videoNameFilePath);
#endif
	fileDescriptor = open(videoNameTmpFilePath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fileDescriptor < 0) {
		systemLog->sysLog(CRITICAL, "cannot open %s in create mode: %s", videoNameTmpFilePath, strerror(errno));
		if (videoNameTmpFilePath)
			free(videoNameTmpFilePath);
		if (videoNameFilePath)
			free(videoNameFilePath);
		if (videoNameNfsFilePath)
			free(videoNameNfsFilePath);
		(*numberOfCopyThread)--;
		return;	
	}
	nfsFileDescriptor = open(videoNameNfsFilePath, O_RDONLY, 0);
	if (nfsFileDescriptor < 0) {
		systemLog->sysLog(CRITICAL, "cannot access to nfs file %s: %s", videoNameNfsFilePath, strerror(errno));
		if (videoNameTmpFilePath)
			free(videoNameTmpFilePath);
		if (videoNameFilePath)
			free(videoNameFilePath);
		if (videoNameNfsFilePath)
			free(videoNameNfsFilePath);
		close(fileDescriptor);
		unlink(videoNameFilePath);
		(*numberOfCopyThread)--;
		return;
	}
	returnCode = cpFile(nfsFileDescriptor, fileDescriptor, &fileSize);
	if (returnCode < 0) {
		systemLog->sysLog(CRITICAL, "cannot copy %s to %s: %s", videoNameNfsFilePath, videoNameTmpFilePath, strerror(errno));
		if (videoNameTmpFilePath)
			free(videoNameTmpFilePath);
		if (videoNameFilePath)
			free(videoNameFilePath);
		if (videoNameNfsFilePath)
			free(videoNameNfsFilePath);
		close(nfsFileDescriptor);
		close(fileDescriptor);
		(*numberOfCopyThread)--;
	}
	returnCode = rename(videoNameTmpFilePath, videoNameFilePath);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot rename %s to %s: %s", videoNameTmpFilePath, videoNameFilePath, strerror(errno));
	}
	if (videoNameTmpFilePath)
		free(videoNameTmpFilePath);
	if (videoNameFilePath)
		free(videoNameFilePath);
	if (videoNameNfsFilePath)
		free(videoNameNfsFilePath);
	close(nfsFileDescriptor);
	close(fileDescriptor);
	(*numberOfCopyThread)--;

	return;
}

/*void CacheFile::copyHttpFile(void *arguments) {
	char *videoNameFilePath = (char *)((int *)arguments)[0];
	int *numberOfCopyThread = (int *)((int *)arguments)[1];
	int videoNameFilePathLength = strlen(videoNameFilePath);
	Curl *curl = (Curl *)((int *)arguments)[2];
	CurlSession *curlSession;
	FILE *streamFile;
	int kQueue;

	videoNameTmpFilePath = (char *)malloc(videoNameFilePathLength+5);
	if (! videoNameTmpFilePath) {
		systemLog->sysLog(ERROR, "error, cannot allocate memory: %s", strerror(errno));
		if (videoNameFilePath)
			free(videoNameFilePath);
		if (videoNameNfsFilePath)
			free(videoNameNfsFilePath);
		return;
	}
	snprintf(videoNameTmpFilePath, videoNameFilePathLength + 5, "%s.tmp", videoNameFilePath);
#ifdef DEBUG
	systemLog->sysLog(INFO, "%s doesn't exist on the cache directory, caching file from NFS", videoNameFilePath);
#endif
	kQueue = kqueue();

	curlSession = curl->createSession();
	
	curl->deleteSession(curlSession);
} */

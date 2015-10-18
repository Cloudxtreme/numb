/***************************************************************************
                          file.cpp  -  description
                             -------------------
    begin                : Dim nov 17 2002
    copyright            : (C) 2002 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "file.h"

File::File() {
  streamFileInitialized = 0;
  fileInitialized = 0;
  maxReadBuffer = 2048;

  return;
}

File::File(String *_fileName, int openMode, int checkFile) {
  streamFileInitialized = 0;
  fileInitialized = 0;
  maxReadBuffer = 2048;
  if (! _fileName) {
    systemLog->sysLog(ERROR, "fileName must be not NULL. Cannot open a NULL file\n");
    return;
  }
  fileName = NEW String(strlen(_fileName->getBloc())+1);
  if (! fileName) {
    systemLog->sysLog(CRITICAL, "cannot open the file %s\n", _fileName->getBloc());
    return;
  }
  strncpy(fileName->getBloc(), _fileName->getBloc(), fileName->getBlocSize()-1);
  fileName->getBloc()[fileName->getBlocSize()-1] = '\0';
  if (! openFile(fileName, openMode, checkFile))
    fileInitialized = 1;

  return;
}

File::File(char *_fileName, int openMode, int checkFile) {
  streamFileInitialized = 0;
  fileInitialized = 0;
  maxReadBuffer = 2048;
  if (! _fileName) {
    systemLog->sysLog(ERROR, "fileName must be not NULL. Cannot open a NULL file\n");
    return;
  }
  fileName = NEW String(strlen(_fileName)+1);
  if (! fileName) {
    systemLog->sysLog(CRITICAL, "cannot open the file %s\n", _fileName);
    return;
  }
  strncpy(fileName->getBloc(), _fileName, strlen(_fileName));
  fileName->getBloc()[fileName->getBlocSize()-1] = '\0';
  if (! openFile(fileName, openMode, checkFile))
    fileInitialized = 1;

  return;
}

File::File(String *_fileName, const char *openMode, int checkFile) {
  streamFileInitialized = 0;
  fileInitialized = 0;
  maxReadBuffer = 2048;
  if (! _fileName) {
    systemLog->sysLog(ERROR, "fileName must be not NULL. Cannot open a NULL file\n");
    return;
  }
  fileName = NEW String(strlen(_fileName->getBloc())+1);
  if (! fileName) {
    systemLog->sysLog(CRITICAL, "cannot open the file %s\n", _fileName->getBloc());
    return;
  }
  strncpy(fileName->getBloc(), _fileName->getBloc(), fileName->getBlocSize()-1);
  fileName->getBloc()[fileName->getBlocSize()-1] = '\0';
  if (! openStreamFile(openMode, checkFile))
    streamFileInitialized = 1;

  return;
}

File::File(char *_fileName, const char *openMode, int checkFile) {
  streamFileInitialized = 0;
  fileInitialized = 0;
  maxReadBuffer = 2048;
  if (! _fileName) {
    systemLog->sysLog(ERROR, "fileName must be not NULL. Cannot open a NULL file\n");
    return;
  }
  fileName = NEW String(strlen(_fileName)+1);
  if (! fileName) {
    systemLog->sysLog(CRITICAL, "cannot open the file %s\n", _fileName);
    return;
  }
  strncpy(fileName->getBloc(), _fileName, strlen(_fileName));
  fileName->getBloc()[strlen(_fileName)] = '\0';
  if (! openStreamFile(openMode, checkFile))
    streamFileInitialized = 1;

  return;
}

File::~File() {
  if (fileInitialized)
    closeFile();
  if (streamFileInitialized)
    closeStreamFile();
  streamFileInitialized = 0;
  fileInitialized = 0;
  maxReadBuffer = 0;
  if (fileName)
    delete fileName;
  fileName = NULL;

  return;
}

/*
 * openMode:
 * O_RDONLY        open for reading only
 * O_WRONLY        open for writing only
 * O_RDWR          open for reading and writing
 * O_NONBLOCK      do not block on open
 * O_APPEND        append on each write
 * O_CREAT         create file if it does not exist
 * O_TRUNC         truncate size to 0
 * O_EXCL          error if create and file exists
 * O_SHLOCK        atomically obtain a shared lock
 * O_EXLOCK        atomically obtain an exclusive lock
 * O_DIRECT        eliminate or reduce cache effects
 * O_FSYNC         synchronous writes
 * O_NOFOLLOW      do not follow symlinks
 */
int File::openFile(String *fileName, int openMode, int checkFile) {
  if (checkFile)
    checkRegularFile(1);
  fileDescriptor = open(fileName->getBloc(), openMode);
  if (fileDescriptor == -1) {
    systemLog->sysLog(ERROR, "cannot open file %s: %s\n", fileName->getBloc(), strerror(errno));
    return errno;
  }
  fileInitialized = 1;

  return 0;
}

int File::openStreamFile(const char *openMode, int checkFile) {
  if (checkFile)
    checkRegularFile(1);
  streamDescriptor = fopen(fileName->getBloc(), openMode);
  if (! streamDescriptor) {
    systemLog->sysLog(ERROR, "cannot open the stream file: %s\n", strerror(errno));
    return errno;
  }
  streamFileInitialized = 1;

  return 0;
}

int File::closeFile(void) {
  if (! fileInitialized)
    return EINVAL;
  if (close(fileDescriptor) == -1) {
    systemLog->sysLog(ERROR, "cannot close the file: %s\n", strerror(errno));
    return errno;
  }
  fileInitialized = 0;

  return 0;
}

int File::closeStreamFile(void) {
  if (! streamFileInitialized)
    return EINVAL;
  if (fclose(streamDescriptor) == EOF) {
    systemLog->sysLog(ERROR, "cannot close the stream file: %s\n", strerror(errno));
    return errno;
  }
  streamFileInitialized = 0;

  return 0;
}

String *File::readStreamFile(size_t *bytesRead) {
  String *readBuffer;
  
  if (! streamFileInitialized) {
    systemLog->sysLog(ERROR, "the stream file is not opened. you must call openStreamFile method before trying to close the file\n");
    return NULL;
  }
  if (! bytesRead) {
    systemLog->sysLog(ERROR, "the bytesRead argument is NULL, cannot read the stream file\n");
    return NULL;
  }
  readBuffer = NEW String(maxReadBuffer);
  *bytesRead = fread(readBuffer->getBloc(), 1, readBuffer->getBlocSize(), streamDescriptor);

  return readBuffer;
}

String *File::readStreamFile(size_t *bytesRead, int bytesToRead) {
  String *readBuffer;

  if (! streamFileInitialized) {
    systemLog->sysLog(ERROR, "the stream file is not opened. you must call openStreamFile method before trying to close the file\n");
    return NULL;
  }
  if (! bytesRead) {
    systemLog->sysLog(ERROR, "the bytesRead argument is NULL, cannot read the stream file\n");
    return NULL;
  }
  readBuffer = NEW String(bytesToRead);
  *bytesRead = fread(readBuffer->getBloc(), 1, readBuffer->getBlocSize(), streamDescriptor);

  return readBuffer;
}

size_t File::readStreamFile(char *writeBuffer, size_t bytesToRead) {
	size_t bytesRead;

  if (! streamFileInitialized) {
    systemLog->sysLog(ERROR, "the stream file is not opened. you must call openStreamFile method before trying to close the file\n");
    return NULL;
  }
  if (! bytesToRead) {
    systemLog->sysLog(ERROR, "the bytesToRead argument is 0, cannot read the stream file\n");
    return NULL;
  }
  bytesRead = fread(writeBuffer, 1, bytesToRead, streamDescriptor);

  return bytesRead;
}


String *File::getStringStreamFile(void) {
  String *readBuffer;

  if (! streamFileInitialized) {
    systemLog->sysLog(ERROR, "the stream file is not opened. you must call openStreamFile method before trying to close the file\n");
    return NULL;
  }
  readBuffer = NEW String(maxReadBuffer);
  fgets(readBuffer->getBloc(), readBuffer->getBlocSize(), streamDescriptor);

  return readBuffer;
}

int File::writeStreamFile(String *writeBuffer) {
  int bytesWrited;

  if (! streamFileInitialized) {
    systemLog->sysLog(ERROR, "the stream file is not opened. you must call openStreamFile method before trying to close the file\n");
    return EINVAL;
  }
  bytesWrited = fwrite(writeBuffer->getBloc(), writeBuffer->getBlocSize(), 1, streamDescriptor);

  return bytesWrited;
}

int File::writeStreamFile(char *writeBuffer, size_t bufferLength) {
  int bytesWrited;

  if (! streamFileInitialized) {
    systemLog->sysLog(ERROR, "the stream file is not opened. you must call openStreamFile method before trying to close the file\n");
    return EINVAL;
  }
  bytesWrited = fwrite(writeBuffer, 1, bufferLength, streamDescriptor);

  return bytesWrited;
}

int File::feofStreamFile(void) {
  if (! streamFileInitialized) {
    systemLog->sysLog(ERROR, "the stream file is not opened. you must call openStreamFile method first\n");
    return -1;
  }
  return feof(streamDescriptor);
}

int File::checkRegularFile(int paranoiacMode)
{
	if (! fileName) {
		systemLog->sysLog(ERROR, "fileName argument is NULL. cannot check the file\n");
		return EINVAL;
	}
	if (! lstat(fileName->getBloc(), &sb)) {
		if (sb.st_mode & S_IFREG)
			return 1;
		else {
			if (paranoiacMode) {
        systemLog->sysLog(ERROR, "%s is not a regular file. exiting", fileName->getBloc());
				exit(EXIT_FAILURE);
			}
			return 0;
		}
	}
	if (paranoiacMode) {
		systemLog->sysLog(ERROR, "problem to do lstat on file %s: %s", fileName->getBloc(), strerror(errno));
		exit(EXIT_FAILURE);
	}

	return -1;
}

ssize_t File::readFile(char *writeBuffer, size_t bufferLength) {
	if (! streamFileInitialized) {
		systemLog->sysLog(ERROR, "the stream file is not opened. you must call openStreamFile method before trying to close the file\n");
		return EINVAL;
	}

	return read(fileDescriptor, writeBuffer, bufferLength);
}

int File::seekStreamFile(long offset, int whence) {
	if (! streamFileInitialized) {
		systemLog->sysLog(ERROR, "the stream file is not opened. you must call openStreamFile method before trying to close the file\n");
		return EINVAL;
	}

	return fseek(streamDescriptor, offset, whence);
}

long File::getFilePosition(void) {
	return ftell(streamDescriptor);
}

/***************************************************************************
                          file.h  -  description
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

#ifndef FILE_H
#define FILE_H

#include "log.h"
#include "mystring.h"
#include "debugtk.h"

#include <sys/types.h>
#include <sys/stat.h>

extern LogError *systemLog;

/**
  *@author spe
  */

class File {
private:
  String *fileName;
  char buffer[32768];
  int fileDescriptor;
  FILE *streamDescriptor;
  int maxReadBuffer;
  int fileInitialized;
  int streamFileInitialized;
  struct stat sb;

public:
  File();
  File(String *, int, int);
  File(char *, int, int);
  File(String *, const char *, int);
  File(char *, const char *, int);
  ~File();
  void setMaxReadBuffer(size_t _maxReadBuffer) { maxReadBuffer = _maxReadBuffer; };
  int getMaxReadBuffer(void) { return maxReadBuffer; };
  int getFileInitialized(void) { return fileInitialized; };
  int getStreamFileInitialized(void) { return streamFileInitialized; };
  int openFile(String *, int, int);
  int openStreamFile(const char *, int);
  int closeFile(void);
  int closeStreamFile(void);
  int checkRegularFile(void);
  String *readStreamFile(size_t *);
  String *readStreamFile(size_t *, int);
  size_t readStreamFile(char *, size_t);
  String *getStringStreamFile(void);
  int writeStreamFile(String *);
  int writeStreamFile(char *, size_t);
  int feofStreamFile(void);
  int checkRegularFile(int);
  FILE *getStreamDescriptor(void) { return streamDescriptor; };
  ssize_t readFile(char *, size_t);
  int seekStreamFile(long, int);
  struct stat *getStat(void) { return &sb; };
  long getFilePosition(void);
};

#endif

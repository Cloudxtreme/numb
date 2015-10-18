/***************************************************************************
                            log.h  -  description
                             -------------------
    begin                : Jeu nov 7 2002
    copyright            : (C) 2002 by Sebastien Petit
    email                : spe@selectbourse.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Includes C
 */
#ifndef LOG_H
#define LOG_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <pthread.h>
#include "debugtk.h"

#define LOG_FILE  -1

#define SYSLOG		0x01
#define LOCALFILE	0x02
#define STDOUT		0x04

#define LOGBUFFER 2048

class LogError {
private:
  int localFile;
  char mode;
  int logOpt;
  char ident[64];
  char buf2[LOGBUFFER];
  pthread_rwlock_t locksyslog;
  pthread_rwlock_t lockmode;
  pthread_rwlock_t lockidentity;
  pthread_rwlock_t lockfacility;
  pthread_rwlock_t lockopen;
  pthread_rwlock_t lockclose;
public:
  LogError(void);
  LogError(const char *, int);
  ~LogError();
  void setOutput(char);
  char getOutput(void);
  void setIdentity(const char *);
  void setLocalFile(int _localFile) { localFile = _localFile; };
  int getLocalFile(void) { return localFile; };
  void setMode(char _mode) { mode = _mode; };
  char getMode(void) { return mode; };
  void setLogOpt(int _logOpt) { logOpt = _logOpt; };
  int  getLogOpt(void) { return logOpt; };
  void setIdent(const char *_ident) { bzero(ident, sizeof(ident)); strncpy(ident, _ident, sizeof(ident)); };
  char *getIdent(void) { return ident; };
  void initMutexes(void);
  void sysLog(int, const char *, ...);
  void sysLog(const char *, int, const char *, ...);
  void sysLog(const char *, const char *, unsigned int, int, const char *, ...);
  void openFile(const char *);
  void closeFile(void);
};

extern LogError *systemLog;

#define ERROR __FILE__, __FUNCTION__, __LINE__, LOG_ERR
#define CRITICAL __FILE__, __FUNCTION__, __LINE__, LOG_CRIT
#define WARNING __FILE__, __FUNCTION__, __LINE__, LOG_WARNING
#define INFO __FILE__, __FUNCTION__, __LINE__, LOG_INFO
#define NOTICE __FILE__, __FUNCTION__, __LINE__, LOG_NOTICE
#define DEBUG __FILE__, __FUNCTION__, __LINE__, LOG_DEBUG

#endif

/***************************************************************************
                          httpsession.cpp  -  description
                             -------------------
    begin                : Tue Jun 24 2006
    copyright            : (C) 2006 by spe
 ***************************************************************************/

#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "httpsession.h"
#include "cachefile.h"
#include "../toolkit/flvparser.h"



StreamHttpSession::StreamHttpSession(Server *_server, int _clientSocket, int *_numberOfCopy, Thread *_cacheFileThread){

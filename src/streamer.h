//
// C++ Interface: streamer
//
// Description: 
//
//
// Author: Sebastien Petit <spebsd@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef STREAMER_H
#define STREAMER_H

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include "../toolkit/server.h"
#include "../toolkit/httpserver.h"
#include "../toolkit/multicastserver.h"
#include "../toolkit/multicastservercatalog.h"
#include "../src/streamcontent.h"
#include "../toolkit/thread.h"
#include "../src/configuration.h"
#include "../toolkit/list.h"
#include "../src/monitoredhost.h"
#include "../src/cataloghashtabletimeout.h"
#include "../toolkit/cachemanager.h"

/**
	@author Sebastien Petit <spebsd@gmail.com>
*/
class Streamer{
private:
	HttpServer *httpServer;
	MulticastServer *multicastServer;
	MulticastServerCatalog *multicastServerCatalog;
	Thread *cacheFileThread;
	HashTable *catalogHashtable;
	List<MonitoredHost *> *serverList;
	Configuration *configuration;
	CatalogHashtableTimeout *catalogHashtableTimeout;
	CacheManager *cacheManager;
	Thread *catalogHashtableTimeoutThread;

public:
	Streamer();
	~Streamer();

	void errorConfigurationFileAndOptions(void);
	void usage(char *);
	int parseAndLoadCatalog(char *);
	int listenAndGetCatalog(void);
	int loadExistingDiskCache(char *);
	int main(int, char **);
};

#endif

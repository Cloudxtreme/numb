//
// C++ Interface: multicastserver
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MULTICASTSERVERCATALOG_H
#define MULTICASTSERVERCATALOG_H

#include "../toolkit/server.h"
#include "../toolkit/hashtable.h"
#include "../toolkit/parser.h"
#include "../src/multicastdata.h"
#include "../src/configuration.h"
#include "../src/monitoredhost.h"

#include <stdlib.h>
#include <netinet/in.h>

/**
	@author  <spe@>
*/
class MulticastServerCatalog : public Server {
private:
	bool multicastServerInitialized;
	HashTable *catalogHashtable;
	List<MonitoredHost *> *serverList;
	Parser *parser;
	Configuration *configuration;

protected:
	int decodePacket(char *);

public:
	MulticastServerCatalog(Configuration *, char *, unsigned short, HashTable *, List<MonitoredHost *> *);
	~MulticastServerCatalog();

	int joinGroup(char *, struct in_addr *);
	int setTTL(unsigned char);
	int setInterface(struct in_addr *);
	int enableLoopback(void);
	int disableLoopback(void);
	int sendPacket(char *, size_t);
	int purgeCatalogFromHost(uint32_t);
	int verifyTimeouts(void);
	int sendPingPacket(void);
	void run(void);
	virtual void start(void *arguments) { run(); delete this; return; };
};

#endif

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
#ifndef MULTICASTSERVER_H
#define MULTICASTSERVER_H

#include "../toolkit/server.h"
#include "../toolkit/hashtable.h"
#include "../toolkit/parser.h"
#include "../src/multicastdata.h"
#include "../src/keyhashtabletimeout.h"

/**
	@author  <spe@>
*/
class MulticastServer : public Server {
private:
	bool multicastServerInitialized;
	KeyHashtableTimeout *keyHashtableTimeout;
	HashTable *keyHashtable;
	Parser *parser;

protected:
	int decodePacket(char *);

public:
	MulticastServer(char *, in_addr_t, unsigned short, HashTable *, KeyHashtableTimeout *);
	~MulticastServer();

	int joinGroup(char *, struct in_addr *);
	int setTTL(unsigned char);
	int setInterface(struct in_addr *);
	int enableLoopback(void);
	int disableLoopback(void);
	void run(void);
	virtual void start(void *arguments) { run(); delete this; return; };
};

#endif

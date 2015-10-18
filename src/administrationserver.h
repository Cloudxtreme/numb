/***************************************************************************
                          administrationserver.h  -  description
                             -------------------
    begin                : Tue Jun 24 2003
    copyright            : (C) 2003 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

#ifndef ADMINISTRATIONSERVER_H
#define ADMINISTRATIONSERVER_H

#include "../toolkit/log.h"
#include "../toolkit/server.h"
#include "../toolkit/thread.h"
#include "../toolkit/objectaction.h"
#include "../src/configuration.h"
#include "../toolkit/hashtable.h"

/**
  *@author spe
  */

class AdministrationServer : public ObjectAction {
private:
	bool administrationServerInitialized;
	bool administrationServerDebug;
	int administrationServerPort;
	int administrationServerMaxQueue;
	Configuration *configuration;
	HashTable *catalogHashtable;

public: 
	Server *server;
	AdministrationServer(Configuration *, HashTable *);
	~AdministrationServer();

	void listen(void);
	virtual void start(void *arguments) { listen(); delete this; return; };
};

#endif

/***************************************************************************
                          administrationserverconnection.h  -  description
                             -------------------
    begin                : Tue Jun 24 2003
    copyright            : (C) 2003 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

#ifndef ADMINISTRATIONSERVERCONNECTION_H
#define ADMINISTRATIONSERVERCONNECTION_H

#include "../toolkit/log.h"
#include "../toolkit/thread.h"
#include "../toolkit/objectaction.h"
#include "../toolkit/server.h"
#include "../toolkit/mystring.h"
#include "../src/configuration.h"
#include "../toolkit/hashtable.h"

#include <sys/types.h>
#include <netinet/in.h>

/**
  *@author spe
  */

extern "C" {
	extern struct conf_type *conf;
}

class AdministrationServerConnection : public ObjectAction {
private:
	bool administrationServerConnectionDebug;
	bool administrationServerConnectionInitialized;
	Server *server;
	const char **commandsList;
	uint64_t clientSocket;
	String *serverAnswer;
	Configuration *configuration;
	struct sockaddr_in *sourceAddress;
	bool sessionAuthenticated;
	HashTable *catalogHashtable;

public: 
	AdministrationServerConnection(Server *, Configuration *, struct sockaddr_in *, HashTable *);
	~AdministrationServerConnection();

	void newSession(void *);
	bool checkAuthentication(void);
	bool executeCommand(socketMsg *smsg);
	int presentServer(void);
	void serverMessage(unsigned short);
	virtual void start(void *arguments) { newSession(arguments); delete this; return; };
};

#endif

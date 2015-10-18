/***************************************************************************
                          administrationserver.cpp  -  description
                             -------------------
    begin                : Tue Jun 24 2003
    copyright            : (C) 2003 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

#include "administrationserver.h"
#include "administrationserverconnection.h"

AdministrationServer::AdministrationServer(Configuration *_configuration, HashTable *_catalogHashtable) {
	//in_addr_t bindAddr = inet_addr("127.0.0.1");

	administrationServerDebug = false;
	administrationServerMaxQueue = 64;
	server = new Server(administrationServerPort, administrationServerMaxQueue);
	if (! server) {
		systemLog->sysLog(ERROR, "in AdministrationServer::AdministrationServer : cannot allocate a new object Server : %s", strerror(errno));
		administrationServerInitialized = false;
	}
	else {
		server->listenSocket();
		administrationServerInitialized = true;
	}
	if (! _configuration) {
		systemLog->sysLog(ERROR, "Configuration object is NULL. cannot initialize server");
		administrationServerInitialized = false;
	}
	else
		configuration = _configuration;
	catalogHashtable = _catalogHashtable;
	administrationServerPort = configuration->administrationServerPort;

	return; 
}

AdministrationServer::~AdministrationServer() {
	administrationServerDebug = false;
	administrationServerInitialized = false;
	if (server)
		delete server;

	return;
}

void AdministrationServer::listen(void) {
	int clientSocket;
	Thread *administrationServerThread;
	AdministrationServerConnection *administrationServerConnection;
	struct sockaddr_in *saddr;

	if (administrationServerInitialized == false) {
		systemLog->sysLog(ERROR, "cannot continue because server is not initialized correctly");
		return;
	}

	// Now we must do an infinite loop on acceptSocket for accepting client connections
	for (;;) {
		saddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
		clientSocket = server->acceptSocket(saddr);
		// If clientSocket is non negative, then the socket is open
		if (clientSocket >= 0) {
			administrationServerConnection = new AdministrationServerConnection(server, configuration, saddr, catalogHashtable);
			administrationServerThread = new Thread(administrationServerConnection);
			administrationServerThread->createThread((void *)clientSocket);
			delete administrationServerThread;
		}
	}

	// Never executed
	return;
}

/***************************************************************************
                          server.h  -  description
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
 
#ifndef _SERVER_H
#define _SERVER_H

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <arpa/inet.h>
#include <errno.h>

#include "log.h"
#include "list.h"
#include "../toolkit/objectaction.h"

#define MAXMSGSIZE 2048

extern LogError *systemLog;

typedef struct socketmessage
{
	struct sockaddr_in saddr;
	ssize_t len;
	ssize_t brecv;
	ssize_t bsent;
	char recvmsg[MAXMSGSIZE+1];
	char *sendmsg;
} socketMsg;


class Server : public ObjectAction {
private:
	int maxqueue;
	int numberOfConnections;

protected:
	int sd;

	void setNumberOfConnections(int);
	int getNumberOfConnections(void);

public:
	int sendLoWat;

	Server(int, int, int, int);
	Server(int, int);
	Server(int, int, int);
	Server(int);
	~Server(void);
	int listenSocket(void);
	int acceptSocket(struct sockaddr_in *);
	int waitMessage(int, int);
	int getCptThread();
	int closeSocket(int);
	int openSocket(int, int, int, int);
	int keepaliveSocket(void);
	socketMsg *recvMessage(int sdc);
	int sendMessage(int sdc, socketMsg *msg);
	void libereSocketMessage(socketMsg *);
	int setNonBlocking();
	int setSendBuffer(int);
	int getSendBuffer();
	int setRecvBuffer(int);
	int setAcceptFilterHttp(void);
	int setMinimumToSend(int);
	int setMinimumToSend(int, int);
	int getSocketDescriptor() { return sd; }
	void addAConnection(void);
	void deleteAConnection(void);
};

#endif

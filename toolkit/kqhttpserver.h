//
// C++ Interface: kqhttpserver
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KQHTTPSERVER_H
#define KQHTTPSERVER_H

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "../toolkit/httpserver.h"
#include "../toolkit/kqhttpsession.h"
#include "../toolkit/httpprotocol.h"

/**
	@author  <spe@>
*/
class KqHttpServer : public HttpServer {
private:

	HttpProtocol *httpProtocol;

public:
	KqHttpServer(int);
	~KqHttpServer();

	int KqHttpServer::acceptConnection(void);
	int KqHttpServer::endConnection(KqHttpSession *);
	int KqHttpServer::writeEvent(KqHttpSession *);
	int KqHttpServer::readEvent(KqHttpSession *);
	int KqHttpServer::run();
};

#endif

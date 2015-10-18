//
// C++ Interface: streamhttpprotocol
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HTTPCLIENTCONNECTION_H
#define HTTPCLIENTCONNECTION_H

#include "../toolkit/httpsession.h"
#include "../toolkit/httpserver.h"
#include "../toolkit/httphandler.h"
#include "../src/configuration.h"

/**
	@author  <spe@>
*/
class HttpClientConnection : public HttpHandler {
private:
	char *buffer;
	int bufferSize;
	Configuration *configuration;
	
public:
	HttpClientConnection(Configuration *);
	~HttpClientConnection();

	int handleConnection(HttpServer *, HttpSession *);
	void logDataDownloaded(HttpSession *);
	virtual int handle(HttpServer *httpServer, HttpSession *httpSession) { return handleConnection(httpServer, httpSession); }
	virtual int closeEvent(HttpServer *, HttpSession *);
};

#endif

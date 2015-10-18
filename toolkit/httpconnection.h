//
// C++ Interface: httpconnection
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <sys/types.h>
#include <sys/socket.h>

#include "../toolkit/objectaction.h"
#include "../toolkit/httpsession.h"
#include "../toolkit/cachedisk.h"
#include "../toolkit/cachememory.h"
#include "../toolkit/curl.h"
#include "../src/configuration.h"

/**
	@author  <spe@>
*/
class HttpConnection : public ObjectAction {
private:
	int socketDescriptor;
	struct sockaddr_in saddr;
	Curl *curl;
	Configuration *configuration;

public:
	bool cantSendMore;
	CacheObject *cacheObject;

	HttpConnection(CacheObject *, Configuration *);
	~HttpConnection();

	void proxyize(HttpSession *);
	void cache(HttpSession *);
	void combinedLog(HttpSession *, char *, int, int);
	virtual void start(void *arguments) { if (! cacheObject) proxyize((HttpSession *)arguments); else cache((HttpSession *)arguments); delete(this); return; };
};

#endif

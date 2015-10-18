//
// C++ Interface: httpcontext
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#include "../toolkit/httphandler.h"

/**
	@author  <spe@>
*/
class HttpContext {
private:
	char *virtualHost;
	HttpHandler *httpHandler;
public:
	HttpContext(const char *, HttpHandler *);
	~HttpContext();

	char *getVirtualHost(void);
	HttpHandler *getHandler(void);
	void setHandler(HttpHandler *);
	
};

#endif

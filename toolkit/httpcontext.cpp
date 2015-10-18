//
// C++ Implementation: httpcontext
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "httpcontext.h"

HttpContext::HttpContext(const char *_virtualHost, HttpHandler *_httpHandler) {
	virtualHost = strdup(_virtualHost);
	httpHandler = _httpHandler;

	return;
}


HttpContext::~HttpContext() {
	if (virtualHost)
		free(virtualHost);

	return;
}

char *HttpContext::getVirtualHost(void) {
	return virtualHost;
}

HttpHandler *HttpContext::getHandler(void) {
	return httpHandler;
}

void HttpContext::setHandler(HttpHandler *_httpHandler) {
	httpHandler = _httpHandler;

	return;
}

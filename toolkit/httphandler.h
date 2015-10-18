//
// C++ Interface: httphandler
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include "../toolkit/httpsession.h"

class HttpServer;

/**
	@author  <spe@>
*/
class HttpHandler {
public:
	HttpHandler();
	virtual ~HttpHandler();

	virtual int handle(HttpServer *, HttpSession *) { return 0; };
	virtual int closeEvent(HttpServer *, HttpSession *) { return 0; };
};

#endif

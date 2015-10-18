//
// C++ Implementation: kqhttpserver
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "kqhttpserver.h"

Executor::Executor(int maxConnectionsAuthorized) : HttpServer(maxConnectionsAuthorized) {

	return;
}

Executor::~Executor() {

	return;
}



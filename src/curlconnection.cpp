//
// C++ Implementation: curlconnection
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "curlconnection.h"

#include "../toolkit/curlsession.h"

CurlConnection::CurlConnection() {
	curl = new Curl();
	if (! curl)
		systemLog->sysLog(ERROR, "cannot create a Curl object: %s", strerror(errno));

	return;
}

CurlConnection::~CurlConnection() {
	if (curl)
		delete curl;

	return;
}

void CurlConnection::fetch(char *localFileName, char *completeUrl) {
	CurlSession *curlSession;

	if (! completeUrl) {
		systemLog->sysLog(ERROR, "cannot fetch Url because completeUrl is NULL");
		return;
	}
	curlSession = curl->createSession();
	curl->fetchHttpUrl(curlSession, localFileName, completeUrl);

	return;
}

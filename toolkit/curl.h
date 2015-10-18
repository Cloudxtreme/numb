//
// C++ Interface: curl
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CURL_H
#define CURL_H

#include <curl/curl.h>
#include "list.h"
#include "log.h"
#include "file.h"
#include "curlsession.h"
#include "../toolkit/mutex.h"

/**
	@author  <spe@>
*/
class Curl{
private:
	bool curlInitialized;
	List<CurlSession *> *curlSessionList;
	int curlSessionNumber;
	Mutex *curlMutex;
public:
	Curl();
	~Curl();

	CurlSession *createSession(void);
	int deleteSession(CurlSession *);
	int setHeaderOutput(CurlSession *);
	int fetchHttpUrl(CurlSession *, File *, char *);
	int fetchHttpUrl(CurlSession *, char *, char *);
	int fetchHttpUrlWithCallback(CurlSession *, void *, void *, void *, void *, char *);
	int getHttpCode(CurlSession *);
	int getDownloadSize(CurlSession *);
	int getContentLength(CurlSession *);
	int setRequestHeader(CurlSession *, struct curl_slist *);
	long getLastModified(CurlSession *);
};

#endif

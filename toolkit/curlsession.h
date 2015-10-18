//
// C++ Interface: curlsession
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CURLSESSION_H
#define CURLSESSION_H

#include <curl/curl.h>

/**
	@author  <spe@>
*/
class CurlSession {
public:
	CURL *session;

	CurlSession();
	~CurlSession();
};

#endif

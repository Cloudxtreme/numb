//
// C++ Interface: curlconnection
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CURLCONNECTION_H
#define CURLCONNECTION_H

#include "../toolkit/objectaction.h"
#include "../toolkit/curl.h"

/**
	@author  <spe@>
*/
class CurlConnection : public ObjectAction {
private:
	Curl *curl;

public:
	CurlConnection();
	~CurlConnection();
	
	virtual void start(void *arguments) { char *completeUrl = (char *)arguments; 
};

#endif

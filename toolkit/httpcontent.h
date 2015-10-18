//
// C++ Interface: httpcontent
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HTTPCONTENT_H
#define HTTPCONTENT_H

#include "../toolkit/httpsession.h"

/**
	@author  <spe@>
*/
class HttpContent {
public:
	HttpContent();
	virtual ~HttpContent();

	virtual int initialize(HttpSession *) { return -1; }
	virtual ssize_t get(HttpSession *, char *, int) { return -1; }
	virtual int put(HttpSession *, char *, int) { return -1; }
};

#endif

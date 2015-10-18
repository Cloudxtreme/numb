//
// C++ Interface: httpprotocol
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HTTPPROTOCOL_H
#define HTTPPROTOCOL_H

/**
	@author  <spe@>
*/
class HttpProtocol {
public:
	HttpProtocol();
	~HttpProtocol();

	virtual int getMethod(void) { };
	virtual int postMethod(void) { };
	virtual int putMethod(void) { };
	virtual int headMethod(void) { };
};

#endif

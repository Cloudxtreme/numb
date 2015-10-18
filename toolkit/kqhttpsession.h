//
// C++ Interface: kqhttpsession
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KQHTTPSESSION_H
#define KQHTTPSESSION_H

#include "../toolkit/httpsession.h"

/**
	@author  <spe@>
*/
class KqHttpSession : public HttpSession {
private:

public:
	KqHttpSession(int);
	~KqHttpSession();

	void KqHttpSession::init(void);
};

#endif

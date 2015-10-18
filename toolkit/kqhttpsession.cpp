//
// C++ Implementation: kqhttpsession
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "kqhttpsession.h"

KqHttpSession::KqHttpSession(int _clientSocket) : HttpSession(_clientSocket) {
	return;
}

KqHttpSession::~KqHttpSession() : ~HttpSession() {
	return;
}

void KqHttpSession::init(void) {
	sessionInit();

	return;
}



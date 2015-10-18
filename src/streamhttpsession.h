/***************************************************************************
                          streamhttpsession.h  -  description
                             -------------------
 ***************************************************************************/

#ifndef KEWEGOCLIENTCONNECTION_H
#define KEWEGOCLIENTCONNECTION_H

#include <sys/stat.h>
#include "../toolkit/log.h"
#include "../toolkit/httpsession.h"

/**
  *@author spe
  */


class StreamHttpSession : public HttpSession {
private:
	
public:
	int fileDescriptor;
	int eventPosition;
	
	StreamHttpSession(Server *, int, int *, Thread *);
	~StreamHttpSession();

	int create(void);
	void destroy(bool);
};

#endif

//
// C++ Interface: multicastpacket
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MULTICASTPACKETCATALOG_H
#define MULTICASTPACKETCATALOG_H

/**
	@author  <spe@>
*/

#define CATALOGPING		0
#define CATALOGGET		1
#define CATALOGADD		2
#define CATALOGDEL		3

#include <netinet/in.h>

// MulticastPacketCatalog class
class MPCatalog {
public:
	char messageType;
	char *relativePath;
	in_addr_t ipAddress;

	MPCatalog();
	~MPCatalog();
};

#endif

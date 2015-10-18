//
// C++ Implementation: multicastpacket
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "multicastpacketcatalog.h"

#include <stdlib.h>

MPCatalog::MPCatalog() {
	relativePath = NULL;

	return;
}


MPCatalog::~MPCatalog() {
	if (relativePath)
		free(relativePath);

	return;
}



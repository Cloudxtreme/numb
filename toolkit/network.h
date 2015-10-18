/***************************************************************************
                          network.h  -  description
                             -------------------
    begin                : Mar nov 12 2002
    copyright            : (C) 2002 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NETWORK_H
#define NETWORK_H

#include "debugtk.h"

/**
  *@author spe
  */

class Network {
private:
  int socketDescriptor;
  int bytesSend;
  int bytesReceived;
public: 
	Network();
	~Network();
  int Network::tcpConnection(Host *destinationHost);
  int Network::udpConnection(Host *destinationHost);
  int Network::closeConnection(
};

#endif

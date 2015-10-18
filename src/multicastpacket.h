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
#ifndef MULTICASTPACKET_H
#define MULTICASTPACKET_H

/**
	@author  <spe@>
*/

#define TYPE_INTERNAL_VIEW 1
#define TYPE_EXTERNAL_VIEW 2
#define TYPE_INTERNAL_VBVIEW 3
#define TYPE_STANDALONE_VIEW 4

class MulticastPacket{
public:
	char key[17];
	unsigned int itemId;
	unsigned int partnerId;
	unsigned short type;
	char countryCode[3];

	MulticastPacket();
	~MulticastPacket();
};

#endif

/***************************************************************************
                          messagereceived.h  -  description
                             -------------------
    begin                : Jeu nov 7 2002
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

#ifndef MESSAGERECEIVED_H
#define MESSAGERECEIVED_H

#include <time.h>
#include "memory.h"
#include "string.h"

/**This is a baseclass for saving raw comstock messages
  *@author spe
  */

class MessageReceived {
public:
  String *message;
  time_t messageTimestamp;

	MessageReceived();
	~MessageReceived();
};

#endif

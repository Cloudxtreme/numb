/***************************************************************************
                          messagereceived.cpp  -  description
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

#include "messagereceived.h"

MessageReceived::MessageReceived() {
  message = NULL;
  messageTimestamp = 0;
}

MessageReceived::~MessageReceived() {
  if (message)
    DELETE message;
  message = NULL;
  messageTimestamp = 0;
}

/***************************************************************************
                          ports.cpp  -  description
                             -------------------
    begin                : Dim déc 15 2002
    copyright            : (C) 2002 by root
    email                : root@vaio.intra.selectbourse.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ports.h"

Ports::Ports() {
  portType = 0;
  singlePort = 0;
  rangePortBegin = 0;
  rangePortEnd = 0;

  return;
}

Ports::~Ports() {
  portType = 0;
  singlePort = 0;
  rangePortBegin = 0;
  rangePortEnd = 0;

  return;
}

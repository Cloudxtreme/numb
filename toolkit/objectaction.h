/***************************************************************************
                          objectaction.h  -  description
                             -------------------
    begin                : Mer déc 4 2002
    copyright            : (C) 2002 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

#ifndef OBJECTACTION_H
#define OBJECTACTION_H

#include <stdio.h>

// Memory debugging
#include "debugtk.h"

/**
  *@author spe
  */

class ObjectAction {
public:
  ObjectAction();
  ObjectAction(int);
  virtual ~ObjectAction();
  virtual void start(void *) { };
  virtual void stop(void) { };
  virtual void wait(void) { };
  virtual void kill(void) { };
};

#endif

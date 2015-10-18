/***************************************************************************
                          string.h  -  description
                             -------------------
    begin                : Ven nov 8 2002
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

#ifndef STRING_H
#define STRING_H

#include "../toolkit/memory.h"
#include "../toolkit/mystring.h"

/**
  *@author spe
  */

class String : public Memory<char> {
public:
	String(int);
	String(const char *);
	~String();

	void initialize(void);
	int stringCopy(String *);
	int stringCopy(const char *);
	int stringNCopy(const char *, int);
	int stringNCopy(String *, int);
	int snPrintf(const char *, ...);
	int stringSearch(const char *);
	void deleteWhiteSpaces(void);
	void deleteCrLn(void);
	int isNumeric(void);
	int hexToBinary(char **, int *);
	int hexToDec(int);
};

#endif

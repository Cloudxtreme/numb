/***************************************************************************
                          string.cpp  -  description
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

#include <ctype.h>
#include "mystring.h"

extern LogError *systemLog;

String::String(int stringLenght) : Memory<char>(stringLenght) {
}

String::String(const char *str) : Memory<char>(strlen(str) + 1) {
	stringCopy(str);

	return;
}

String::~String() {
}

void String::initialize(void) {
	bzero(bloc, blocSize);

	return;
}

int String::stringCopy(String *sourceString) {
	if ((! sourceString) || (! bloc)) {
		systemLog->sysLog(ERROR, "cannot copy the source string because bloc is NULL or sourceString is NULL");
		return EINVAL;
	}
	(void)strncpy(bloc, sourceString->bloc, blocSize-1);
	bloc[blocSize-1] = '\0';

	return 0;
}

int String::stringCopy(const char *sourceString) {
	if ((! sourceString) || (! bloc)) {
		systemLog->sysLog(ERROR, "cannot copy the source string because bloc is NULL or sourceString is NULL");
		return EINVAL;
	}
	(void)strncpy(bloc, sourceString, blocSize-1);
	bloc[blocSize-1] = '\0';

	return 0;
}

int String::stringNCopy(String *sourceString, int charactersNumber) {
	if ((! sourceString) || (! bloc)) {
		systemLog->sysLog(LOG_ERR, "cannot copy the source string because bloc is NULL or sourceString is NULL");
		return EINVAL;
	}
	(void)strncpy(bloc, sourceString->bloc, charactersNumber-1);
	bloc[charactersNumber-1] = '\0';

	return 0;
}

int String::stringNCopy(const char *sourceString, int charactersNumber) {
	if ((! sourceString) || (! bloc)) {
		systemLog->sysLog(ERROR, "cannot copy the source string because bloc is NULL or sourceString is NULL");
		return EINVAL;
	}
	(void)strncpy(bloc, sourceString, charactersNumber-1);
	bloc[charactersNumber-1] = '\0';

	return 0;
}

int String::snPrintf(const char *sourceString, ...) {
	va_list ap;

	if ((! sourceString) || (! bloc)) {
		systemLog->sysLog(LOG_ERR, "cannot copy the source string because bloc is NULL or sourceString is NULL\n");
		return EINVAL;
	}
	va_start(ap, sourceString);
	(void)vsnprintf(bloc, blocSize, sourceString, ap);
	va_end(ap);

	return 0;
}

int String::stringSearch(const char *sourceString) {
  if ((! sourceString) || (! bloc)) {
    systemLog->sysLog(LOG_ERR, "cannot search the source string because bloc is NULL or sourceString is NULL");
    return -1;
  }
  if (strstr(bloc, sourceString))
    return 1;
  else
    return 0;
}

void String::deleteWhiteSpaces(void) {
  int counter = 0;
  int rotateCounter;

  if (! bloc) {
    systemLog->sysLog(LOG_ERR, "cannot deleting white spaces because the string is NULL\n");
    return;
  }
  while (bloc[counter] && (counter < blocSize)) {
    if ((bloc[counter] == ' ') || (bloc[counter] == '\t')) {
      rotateCounter = counter;
      while ((rotateCounter+1 < blocSize) && (bloc[rotateCounter+1])) {
        bloc[rotateCounter] = bloc[rotateCounter+1];
        rotateCounter++;
      }
      bloc[rotateCounter] = 0;
    }
    counter++;
  }

  return;
}

void String::deleteCrLn(void) {
  int counter = 0;
  int rotateCounter;

  if (! bloc) {
    systemLog->sysLog(LOG_ERR, "cannot deleting white spaces because the string is NULL\n");
    return;
  }
  while (bloc[counter] && (counter < blocSize)) {
    if ((bloc[counter] == '\n') || (bloc[counter] == '\r')) {
      rotateCounter = counter;
      while ((rotateCounter+1 < blocSize) && (bloc[rotateCounter+1])) {
        bloc[rotateCounter] = bloc[rotateCounter+1];
        rotateCounter++;
      }
      bloc[rotateCounter] = 0;
    }
    counter++;
  }

  return;
}

int String::isNumeric(void) {
  int counter = 0;

  while (bloc[counter]) {
    if (! isdigit(bloc[counter]))
      return 0;
    counter++;
  }
  return 1;
}

int String::hexToDec(int hex) {
	int dec;

	if ((hex >= 0x30) && (hex <= 0x39))
		dec = hex - 0x30;
	else
		if ((hex >= 0x61) && (hex <= 0x66))
			dec = hex - 0x61 + 10;
		else
			return -1;

	return dec;
}

int String::hexToBinary(char **binary, int *length) {
	int counter = 0;
	int binCounter = 0;
	char lo, hi;
	int len;
	int decimal;

	if ((! binary) || (! length)) {
		systemLog->sysLog(ERROR, "invalid pointer arguments: char **binary = %p, length = %p\n", binary, length);
		return -1;
	}
	len = strlen(bloc);
	if ((len % 2) == 1) {
		systemLog->sysLog(ERROR, "cannot convert a string to hex that is not a multiple of 2");
		return -1;
	}

	*length = len / 2;
	*binary = (char *)malloc(*length);

	while (bloc[counter]) {
		lo = tolower(bloc[counter]);
		hi = tolower(bloc[counter+1]);
		decimal = hexToDec(lo);
		if (decimal == -1) {
			systemLog->sysLog(ERROR, "invalid hexadecimal value: 0x%c\n", bloc[counter]);
			return -1;
		}
		(*binary)[binCounter] = decimal;
		(*binary)[binCounter] = ((*binary)[binCounter] << 4) & 0xf0;
		decimal = hexToDec(hi);
		if (decimal == -1) {
			systemLog->sysLog(ERROR, "invalid hexadecimal value: 0x%c\n", bloc[counter]);
			return -1;
		}
		(*binary)[binCounter] += decimal;
		printf("%.2X ", decimal);
		counter += 2;
		binCounter++;
	}

	return 0;
}

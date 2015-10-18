//
// C++ Interface: hashtableelt
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HASHTABLEELT_H
#define HASHTABLEELT_H

/**
	@author  <spe@>
*/
class HashTableElt {
private:
	char *key;
	void *data;
	HashTableElt *next;
	HashTableElt *previous;

public:
	HashTableElt();
	~HashTableElt();

	void setData(void *);
	void *getData(void);
	void setKey(char *);
	char *getKey(void);
	void setNext(HashTableElt *);
	HashTableElt *getNext(void);
};

#endif

/***************************************************************************
                          listelt.h  -  description
                             -------------------
    begin                : Jeu nov 7 2002
    copyright            : (C) 2002 by Sebastien Petit
    email                : spe@selectbourse.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include "debugtk.h"
#include "log.h"

#define DATA_DESTROY_WITH_DELETE 2

template<class T> class ListElt {
private:
  T data;
  ListElt<T> *next;
  ListElt<T> *prev;
public:
  T getData(void); 
  void setData(T _data);
  ListElt<T> *getNext(void);
  void setNext(ListElt<T> *_next);
  ListElt<T> *getPrev(void);
  void setPrev(ListElt<T> *_prev); 
};

template<class T> T ListElt<T>::getData(void) {
	return data;
}

template<class T> void ListElt<T>::setData(T _data) {
	data = _data;
}

template<class T> ListElt<T> *ListElt<T>::getNext(void) {
	 return next;
}

template<class T> void ListElt<T>::setNext(ListElt<T> *_next) {
	next = _next;
}

template<class T> ListElt<T> *ListElt<T>::getPrev(void) {
	return prev;
}

template<class T> void ListElt<T>::setPrev(ListElt<T> *_prev) {
	prev = _prev;
}

template<class Item> class List {
private:
  ListElt<Item> *maListe;
  int destroyData;
public:
  int listSize;
  List();
  ~List();

  ListElt<Item> *getMaListe(void);
  void setMaListe(ListElt<Item> *_maListe);
  // destroyData == 0 pas de liberation
  // destroyData == 1 liberation par free
  // destroyData == 2 liberation par delete
  void setDestroyData(int _destroyData);
  int removeFirst(void);
  int removeElt(ListElt<Item> *elt, int destroyData);
  ListElt<Item> *addElement(Item data);
  void purgeAllElements(void);
  Item getFirstElement(void);
  Item getLastElement(void);
  Item getElement(int);
  int getListSize(void) { return listSize; }
};

template<class Item> List<Item>::List() {
	ListElt<Item> *dernierElement;

	dernierElement = NEW ListElt<Item>;
	destroyData = 1;
	maListe = NEW ListElt<Item>;
	maListe->setNext(dernierElement);
	maListe->setPrev(dernierElement);
	maListe->setData(NULL);
	dernierElement->setNext(maListe);
	dernierElement->setPrev(maListe);
	dernierElement->setData(NULL);
	listSize = 0;
}

template<class Item> List<Item>::~List()
{
	purgeAllElements();
	listSize = 0;
	delete(maListe->getPrev());
	delete(maListe);
}

template<class Item> ListElt<Item> *List<Item>::getMaListe(void) { 
	return maListe; 
}

template<class Item> void List<Item>::setMaListe(ListElt<Item> *_maListe) { 
	maListe = _maListe; 
}

template<class Item> void List<Item>::setDestroyData(int _destroyData) { 
	destroyData = _destroyData; 
}

template<class Item> int List<Item>::removeFirst(void) {
	ListElt<Item> *elt;

	if (! maListe) {
		systemLog->sysLog(LOG_ERR, "cannot remove elements when list is NULL");
		return -1;
	}
	if (maListe->getNext() == maListe->getPrev()) {
		systemLog->sysLog(LOG_ERR, "cannot remove first when the liste is empty\n");
		return -1;
	}
	if(maListe->getNext() ==  NULL) {
		return -1;
	}
	elt = maListe->getNext();
	elt->getNext()->setPrev(maListe);
	maListe->setNext(elt->getNext());
	if (destroyData == 1)
		if(elt->getData() != NULL)
			free(elt->getData());
	if (destroyData == 2)
		if(elt->getData() != NULL)
			delete(elt->getData());

	delete(elt);
  listSize--;
  
	return 0;
}

template<class Item> int List<Item>::removeElt(ListElt<Item> *elt, int destroyData) {
	if (! elt) {
		systemLog->sysLog(LOG_ERR, "cannot add elements when poselt is NULL");
		return -1;
	}
	if (elt->getNext() == elt->getPrev()) {
		systemLog->sysLog(LOG_ERR, "cannot remove first and last elements of sentinel");
		return -1;
	}
	elt->getPrev()->setNext(elt->getNext());
	elt->getNext()->setPrev(elt->getPrev());
	if (destroyData == 1) {
		free(elt->getData());
	}
	if (destroyData == 2) {
		delete(elt->getData());
	}

	delete(elt);
  listSize--;
  
  return 0;
}

template<class Item> ListElt<Item> *List<Item>::addElement(Item data) {
	ListElt<Item> *elt = NEW ListElt<Item>;

	if (! maListe) {
		systemLog->sysLog(LOG_ERR, "cannot add elements when List::maListe is NULL");
		return NULL;
	}
	if (! elt || ! data)
		return NULL;
	elt->setPrev(maListe->getPrev()->getPrev());
	elt->setNext(maListe->getPrev());
	elt->setData(data);
	maListe->getPrev()->getPrev()->setNext(elt);
	maListe->getPrev()->setPrev(elt);
  listSize++;

	return elt;
}

template<class Item> void List<Item>::purgeAllElements(void) {
	ListElt<Item> *elt;

	if (! maListe) {
		systemLog->sysLog(LOG_ERR, "cannot remove all elements when list is NULL");
		return;
	}
	elt = maListe->getPrev()->getPrev();
	while (elt != maListe) {
		elt = elt->getPrev();
		removeElt(elt->getNext(), destroyData);
	}
	listSize = 0;

	return;
}

template<class Item> Item List<Item>::getFirstElement(void) {
	return maListe->getNext()->getData();
}

template<class Item> Item List<Item>::getElement(int elementPosition) {
  ListElt<Item> *pointerList = maListe->getNext();

  if (elementPosition < 1) {
    systemLog->sysLog(LOG_ERR, "cannot return a list element position < 1\n");
    return NULL;
  }
  while (--elementPosition)
    pointerList = pointerList->getNext();

  return pointerList->getData();
}

template<class Item> Item List<Item>::getLastElement(void) {
	return maListe->getPrev()->getPrev()->getData();
}

#endif

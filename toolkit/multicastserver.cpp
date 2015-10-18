//
// C++ Implementation: multicastserver
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "multicastserver.h"

#include "../src/multicastpacket.h"

MulticastServer::MulticastServer(char *multicastGroup, in_addr_t sourceMulticastIp, unsigned short port, HashTable *_keyHashtable, KeyHashtableTimeout *_keyHashtableTimeout) : Server(SOCK_DGRAM, IPPROTO_IP, port) {
	int returnCode;
	struct in_addr interfaceIp;
	char delimiter[2];

	interfaceIp.s_addr = sourceMulticastIp;
	multicastServerInitialized = false;
		
	returnCode = joinGroup(multicastGroup, &interfaceIp);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot initialized MulticastServer correctly");
		return;
	}
	returnCode = setTTL(255);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot initialized MulticastServer correctly");
		return;
	}
	delimiter[0] = '\n';
	delimiter[1] = '\0';
	parser = new Parser(delimiter);
	if (! parser) {
		systemLog->sysLog(CRITICAL, "cannot create a Parser object: %s", strerror(errno));
		return;
	}
	keyHashtable = _keyHashtable;
	keyHashtableTimeout = _keyHashtableTimeout;

	/* construct a multicast address structure */
	struct sockaddr_in mc_addr;
	memset(&mc_addr, 0, sizeof(mc_addr));
	mc_addr.sin_family = AF_INET;
	mc_addr.sin_addr.s_addr = inet_addr(multicastGroup);
	mc_addr.sin_port = htons(port);

	if (bind(sd, (struct sockaddr*) &mc_addr, sizeof(mc_addr)) == -1) {
	  systemLog->sysLog(ERROR, "cannot bind multicast adress: %s\n", strerror(errno));
	  return;
	}

	multicastServerInitialized = true;
	
	return;
}

MulticastServer::~MulticastServer() {
	if (parser)
		delete parser;
	multicastServerInitialized = false;

	return;
}

/* join multicast group with ip address */
int MulticastServer::joinGroup(char *multicastIp, struct in_addr *interfaceIp)
{
	struct ip_mreq  imr;
	int reuse = 1;

	bzero(&imr, sizeof(imr));
	imr.imr_multiaddr.s_addr = inet_addr(multicastIp);
	imr.imr_interface.s_addr = interfaceIp->s_addr;
	if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(imr)) == -1) {
		systemLog->sysLog(ERROR, "cannot join multicast group (IP_ADD_MEMBERSHIP): %s", strerror(errno));
		return -1;
	}
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1) {
	  systemLog->sysLog(ERROR, "cannot set reuse port on multicast address: %s", strerror(errno));
	  return -1;
	}
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
	  systemLog->sysLog(ERROR, "cannot set reuse addr on multicast address: %s", strerror(errno));
	  return -1;
	}

	return 0;
}

/* Set multicast ttl IP */
int MulticastServer::setTTL(unsigned char ttl)
{
	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1) {
		systemLog->sysLog(ERROR, "cannot set IP_MULTICAST_TTL: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int MulticastServer::setInterface(struct in_addr *address)
{
	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, address, sizeof(*address)) == -1) {
		systemLog->sysLog(ERROR, "cannot setsockopt IP_MULTICAST_IF on primary address: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int MulticastServer::enableLoopback(void) {
	int flag = 1;

	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, sizeof(flag)) == -1) {
		perror("cannot setsockopt IP_MULTICAST_LOOP");
		return -1;
	}

	return 0;
}

int MulticastServer::disableLoopback(void) {
	int flag = 0;

	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, sizeof(flag)) == -1) {
		systemLog->sysLog(ERROR, "cannot setsockopt IP_MULTICAST_LOOP: %s", strerror(errno));
		return -1;
	}

	return 0;	
}

int MulticastServer::decodePacket(char *message) {
	char key[17];
	List<String *> *stringList;
	String *string;
	struct MulticastData *multicastData;
	uint32_t hashPosition;
	HashTableElt *hashTableElt;
	int returnCode;

	// Parse the messsage and tokenize on '\n'
	stringList = parser->tokenizeString(message, 128);
	if (! stringList)
		return -1;

	stringList->setDestroyData(2);

	if (stringList->listSize < 5) {
		systemLog->sysLog(ERROR, "invalid packet, must have 6 arguments, but we have %d arguments.\n--- Message ---\n%s", stringList->listSize, message);
		delete stringList;
		return -1;
	}

	multicastData = (struct MulticastData *)malloc(sizeof(*multicastData));
	if (! multicastData) {
		systemLog->sysLog(CRITICAL, "cannot create a struct MulticastData: %s", strerror(errno));
		delete stringList;
		return -1;
	}
	
	// Now comnplete the multicastPacket fields
	string = stringList->getElement(1);
	multicastData->commandType = atoi(string->bloc);

	switch (multicastData->commandType) {
		case 0:
			string = stringList->getElement(2);
			strncpy(key, string->bloc, sizeof(key)-1);
			key[sizeof(key)-1] = '\0';
			string = stringList->getElement(3);
			multicastData->itemId = atoi(string->bloc);
			string = stringList->getElement(4);
			multicastData->productId = atoi(string->bloc);
			string = stringList->getElement(5);
			snprintf(multicastData->countryCode, sizeof(multicastData->countryCode), "%s", string->bloc);
			string = stringList->getElement(6);
			multicastData->tagId = atoi(string->bloc);
			if (stringList->listSize > 6) {
				string = stringList->getElement(7);
				multicastData->encodingFormatId = atoi(string->bloc);
			}
			break;

		case 1:
			string = stringList->getElement(2);
			strncpy(key, string->bloc, sizeof(key)-1);
			key[sizeof(key)-1] = '\0';
			string = stringList->getElement(3);
			multicastData->itemId = atoi(string->bloc);
			string = stringList->getElement(4);
			multicastData->productId = atoi(string->bloc);
			string = stringList->getElement(5);
			multicastData->tagId = atoi(string->bloc);
			if (stringList->listSize > 5) {
				string = stringList->getElement(6);
				multicastData->encodingFormatId = atoi(string->bloc);
			}
			break;

		default:
			// format unknown, log error here XXX
			break;
	}

	// And destruct the arguments List
	delete stringList;

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "[=== message decoded ===]");
	systemLog->sysLog(DEBUG, "	key is #%s#", key);
	systemLog->sysLog(DEBUG, "	commandType is %d", multicastData->commandType);
	systemLog->sysLog(DEBUG, "	itemId is %d", multicastData->itemId);
	systemLog->sysLog(DEBUG, "	productId is %d", multicastData->productId);
	systemLog->sysLog(DEBUG, "	tagId is %d", multicastData->tagId);
	systemLog->sysLog(DEBUG, "	encoding format id %d", multicastData->encodingFormatId);
#endif

	keyHashtable->lock();
	hashTableElt = keyHashtable->add(key, multicastData, &hashPosition);
	if (! hashTableElt) {
		free(multicastData);
		keyHashtable->unlock();
		return -1;
	}
	returnCode = keyHashtableTimeout->add(hashPosition, hashTableElt);
	if (returnCode < 0) {
		keyHashtable->remove(key);
		free(multicastData);
		keyHashtable->unlock();
		return -1;
	}
	keyHashtable->unlock();

#ifdef DEBUGOUTPUT
	if ((keyHashtable->getNumberOfElements() % 1000) == 0) {
		systemLog->sysLog(DEBUG, "numberOfCollisions is %d", keyHashtable->getNumberOfCollisions());
		systemLog->sysLog(DEBUG, "numberOfElements   is %d", keyHashtable->getNumberOfElements());
	}
#endif

	return 0;
}

void MulticastServer::run(void) {
	socketMsg *socketMessage;
	int returnCode;

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "Multicast server is running and waiting for packets");
#endif
	for (;;) {
		socketMessage = recvMessage(sd);
#ifdef DEBUGOUTPUT
		systemLog->sysLog(DEBUG, "multicast packet size is %d", socketMessage->brecv);
#endif
		if (socketMessage) {
			returnCode = decodePacket(socketMessage->recvmsg);
			free(socketMessage);
			socketMessage = NULL;
		}
	}
}

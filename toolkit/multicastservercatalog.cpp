//
// C++ Implementation: multicastservercatalog
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "multicastservercatalog.h"
#include "../src/multicastpacketcatalog.h"
#include "../src/catalogdata.h"

MulticastServerCatalog::MulticastServerCatalog(Configuration *_configuration, char *multicastGroup, unsigned short port, HashTable *_catalogHashtable, List<MonitoredHost *> *_serverList) : Server(SOCK_DGRAM, IPPROTO_IP, port) {
	int returnCode;
	struct in_addr interfaceIp;
	char delimiter[2];

	interfaceIp.s_addr = INADDR_ANY;
	multicastServerInitialized = false;
	returnCode = joinGroup(multicastGroup, &interfaceIp);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot initialized MulticastServerCatalog correctly");
		return;
	}
	returnCode = setTTL(255);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot initialized MulticastServerCatalog correctly");
		return;
	}
	returnCode = disableLoopback();
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot initialized MulticastServerCatalog correctly");
	}
	delimiter[0] = '\n';
	delimiter[1] = '\0';
	parser = new Parser(delimiter);
	if (! parser) {
		systemLog->sysLog(CRITICAL, "cannot create a Parser object: %s", strerror(errno));
		return;
	}
	catalogHashtable = _catalogHashtable;
	serverList = _serverList;
	configuration = _configuration;

	multicastServerInitialized = true;
	
	return;
}

MulticastServerCatalog::~MulticastServerCatalog() {
	if (parser)
		delete parser;
	multicastServerInitialized = false;


	return;
}

/* join multicast group with ip address */
int MulticastServerCatalog::joinGroup(char *multicastIp, struct in_addr *interfaceIp)
{
	struct ip_mreq  imr;

	bzero(&imr, sizeof(imr));
	imr.imr_multiaddr.s_addr = inet_addr(multicastIp);
	imr.imr_interface.s_addr = interfaceIp->s_addr;
	if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(imr)) == -1) {
		systemLog->sysLog(ERROR, "cannot join multicast group (IP_ADD_MEMBERSHIP): %s", strerror(errno));
		return -1;
	}

	return 0;
}

/* Set multicast ttl IP */
int MulticastServerCatalog::setTTL(unsigned char ttl)
{
	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1) {
		systemLog->sysLog(ERROR, "cannot set IP_MULTICAST_TTL: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int MulticastServerCatalog::setInterface(struct in_addr *address)
{
	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, address, sizeof(*address)) == -1) {
		systemLog->sysLog(ERROR, "cannot setsockopt IP_MULTICAST_IF on primary address: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int MulticastServerCatalog::enableLoopback(void) {
	int flag = 1;

	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, sizeof(flag)) == -1) {
		perror("cannot setsockopt IP_MULTICAST_LOOP");
		return -1;
	}

	return 0;
}

int MulticastServerCatalog::disableLoopback(void) {
	int flag = 0;

	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &flag, sizeof(flag)) == -1) {
		systemLog->sysLog(ERROR, "cannot setsockopt IP_MULTICAST_LOOP: %s", strerror(errno));
		return -1;
	}

	return 0;	
}

int MulticastServerCatalog::decodePacket(char *message) {
	List<String *> *stringList;
	String *string;
	struct CatalogData *catalogData;
	uint32_t hashPosition;
	HashTableElt *hashTableElt;
	int returnCode;
	int i;
	MonitoredHost *monitoredHost;
	size_t len;
	char messageType;
	char *relativePath;
	char *buffer;

	// Parse the messsage and tokenize on '\n'
	stringList = parser->tokenizeString(message, 128);
	if (! stringList)
		return -1;

	stringList->setDestroyData(2);

	if ((stringList->listSize <= 0) || (stringList->listSize > 3)) {
		systemLog->sysLog(ERROR, "invalid packet, must have arguments > 0 and arguments <= 3, but we have %d arguments.\n--- Message ---\n%s", stringList->listSize, message);
		delete stringList;
		return -1;
	}

	catalogData = (struct CatalogData *)malloc(sizeof(struct CatalogData));
	if (! catalogData) {
		systemLog->sysLog(CRITICAL, "cannot create a MPCatalog object: %s", strerror(errno));
		delete stringList;
		return -1;
	}
	
	// Now comnplete the multicastPacket fields
	string = stringList->getElement(1);
	messageType = atoi(string->bloc);

	switch (messageType) {
		case CATALOGPING:
			string = stringList->getElement(2);
			catalogData->host = inet_addr(string->bloc);
#ifdef DEBUGCATALOG
			systemLog->sysLog(DEBUG, "ping from #%s#", string->bloc);
#endif
			for (i = 1; i <= serverList->getListSize(); i++) {
				if (serverList->getElement(i)->ipAddress == catalogData->host) {
					serverList->getElement(i)->timeout = time(NULL) + 10;
#ifdef DEBUGCATALOG
					systemLog->sysLog(DEBUG, "found server %s on the list, setting timeout to %d", inet_ntoa(*((struct in_addr *)&catalogData->host)), serverList->getElement(i)->timeout);
#endif
					free(catalogData);
					delete stringList;
					return 0;
				}
			}
			monitoredHost = new MonitoredHost();
			if (! monitoredHost) {
				systemLog->sysLog(CRITICAL, "cannot allocate ipAddressPtr object: %s", strerror(errno));
				free(catalogData);
				delete stringList;
				return -1;
			}
			monitoredHost->ipAddress = catalogData->host;
			monitoredHost->timeout = time(NULL) + 10;
			serverList->addElement(monitoredHost);

			buffer = new char[4096];
			if (! buffer) {
				systemLog->sysLog(CRITICAL, "cannot allocate a buffer object: %s", strerror(errno));
				free(catalogData);
				delete stringList;
				return -1;
			}
			buffer[0] = '\0';
			systemLog->sysLog(INFO, "a new host %s has join the cluster !", inet_ntoa(*((struct in_addr *)&catalogData->host)));
			for (i = 1; i <= serverList->getListSize(); i++)
				snprintf(buffer, 4096, "%s %s ", buffer, inet_ntoa(*((struct in_addr *)&serverList->getElement(i)->ipAddress)));
			
 			systemLog->sysLog(INFO, "cluster members are: %s%s", inet_ntoa(configuration->proxyIp), buffer);

			delete buffer;

			free(catalogData);
			delete stringList;

			return 0;
			
			break;
		case CATALOGADD:
			char *key;

			if (stringList->getListSize() != 3) {
				systemLog->sysLog(ERROR, "CATALOGADD command must have exactly 2 parameters");
				free(catalogData);
				delete stringList;
				return -1;
			}
			string = stringList->getElement(2);
			catalogData->host = inet_addr(string->bloc);
			catalogData->counter = 0;
			string = stringList->getElement(3);
			len = strlen(string->bloc);
			key = new char[len + 1];
			if (! key) {
				systemLog->sysLog(CRITICAL, "cannot allocate relativePath object: %s", strerror(errno));
				free(catalogData);
				delete stringList;
				return -1;
			}
			strncpy(key, string->bloc, len);
			key[len] = '\0';

#ifdef DEBUGCATALOG
			systemLog->sysLog(DEBUG, "new entry in catalog:");
			systemLog->sysLog(DEBUG, "	key = #%s#", key);
			systemLog->sysLog(DEBUG, "	catalogData->host = #%s#", inet_ntoa(*((struct in_addr *)&catalogData->host)));
#endif

			catalogHashtable->lock();
			hashTableElt = catalogHashtable->add(key, catalogData, &hashPosition);
			if (! hashTableElt) {
				systemLog->sysLog(CRITICAL, "cannot add key to catalog hashtable: %s", strerror(errno));
				free(catalogData);
				delete stringList;
				free(key);
				catalogHashtable->unlock();
				return -1;
			}
			catalogHashtable->unlock();

#ifdef DEBUGCATALOG
			if ((catalogHashtable->getNumberOfElements() % 1000) == 0) {
				systemLog->sysLog(DEBUG, "numberOfCollisions is %d", catalogHashtable->getNumberOfCollisions());
				systemLog->sysLog(DEBUG, "numberOfElements   is %d", catalogHashtable->getNumberOfElements());
			}
#endif
			delete stringList;
			return 0;
			
			break;
		case CATALOGDEL:
			if (stringList->getListSize() != 2) {
				systemLog->sysLog(ERROR, "CATALOGDEL command must have exactly 1 parameters");
				free(catalogData);
				delete stringList;
				return -1;
			}
			string = stringList->getElement(2);
			len = strlen(string->bloc);
			relativePath = new char[len + 1];
			if (! relativePath) {
				systemLog->sysLog(CRITICAL, "cannot allocate relativePath object: %s", strerror(errno));
				free(catalogData);
				delete stringList;
				return -1;
			}
			strncpy(relativePath, string->bloc, len);
			relativePath[len] = '\0';

			catalogHashtable->lock();
			returnCode = catalogHashtable->remove(relativePath, &hashTableElt);
			if (returnCode) {
				systemLog->sysLog(CRITICAL, "cannot delete key to catalog hashtable: %s", strerror(errno));
				delete catalogData;
				delete stringList;
				return -1;
			}

			//catalogData = (CatalogData *)hashTableElt->getData();
			catalogHashtable->unlock();

			free(catalogData);
			free(relativePath);
			delete hashTableElt;
			delete stringList;

			return 0;
			
			break;
		default:
			systemLog->sysLog(ERROR, "unrecognized catalog message type %d", messageType);
			free(catalogData);
			delete stringList;
			return -1;
			break;
	} 

	// Never executed
	return -1;
}

int MulticastServerCatalog::sendPacket(char *buffer, size_t len) {
	int returnCode;
	socketMsg socketMessage;

	bzero(&socketMessage, sizeof(socketMessage));
	socketMessage.saddr.sin_family = AF_INET;
	socketMessage.saddr.sin_port = htons(4447);
	socketMessage.saddr.sin_addr.s_addr = inet_addr("224.0.0.3");
	socketMessage.len = len;
	socketMessage.sendmsg = buffer;
	returnCode = sendMessage(sd, &socketMessage);
	if (returnCode) {
		systemLog->sysLog(ERROR, "cannot send multicast packet: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int MulticastServerCatalog::sendPingPacket(void) {
	char *buffer;
	char *ip;
	int len;
	int returnCode;

	ip = strdup(inet_ntoa(*((struct in_addr *)&configuration->proxyIp)));
	if (! ip) {
		systemLog->sysLog(CRITICAL, "cannot strdup on char *ip: %s", strerror(errno));
		return -1;
	}
	len = 2 + strlen(ip);
	buffer = (char *)malloc(len+1);
	snprintf(buffer, len+1, "%d\n%s", CATALOGPING, ip);
	free(ip);
	returnCode = sendPacket(buffer, len);
	free(buffer);

	return returnCode;
}

int MulticastServerCatalog::purgeCatalogFromHost(uint32_t ipAddress) {
	int i = 0;
	int **hashtablePtr;
	int hashtableSize;
	HashTableElt *hashtableElt;
	struct CatalogData *catalogData;

	catalogHashtable->lock();
	hashtablePtr = catalogHashtable->getHashtable();
	hashtableSize = catalogHashtable->getSize();
#ifdef DEBUGCATALOG
	systemLog->sysLog(DEBUG, "ipAddress is %s hashtableSize is %d", inet_ntoa(*((struct in_addr *)&ipAddress)), hashtableSize);
#endif
	while (i < hashtableSize) {
		if (hashtablePtr[i]) {
#ifdef DEBUGCATALOG
			systemLog->sysLog(DEBUG, "found data at %d in catalog hashtable", i);
#endif
			hashtableElt = (HashTableElt *)hashtablePtr[i];
			while (hashtableElt) {
				catalogData = (struct CatalogData *)hashtableElt->getData();
#ifdef DEBUGCATALOG
				systemLog->sysLog(DEBUG, "catalogData->host is %s and ipAddress is %s", inet_ntoa(*((struct in_addr *)&catalogData->host)), inet_ntoa(*((struct in_addr *)&ipAddress)));
#endif
				if (catalogData->host == ipAddress) {
					catalogHashtable->remove(i, hashtableElt);
					free(catalogData);
				}
				hashtableElt = hashtableElt->getNext();
			}
		}
		i++;
	}
	catalogHashtable->unlock();

	return 0;
}

int MulticastServerCatalog::verifyTimeouts(void) {
	int i;
	MonitoredHost *monitoredHost;
	time_t actualTime;
	int returnCode;

	actualTime = time(NULL);
	for (i = 1; i <= serverList->getListSize(); i++) {
		monitoredHost = serverList->getElement(i);
		if (monitoredHost->timeout < actualTime) {
			systemLog->sysLog(NOTICE, "server %s is not alive, server is disabled for redirection", inet_ntoa(*((struct in_addr *)&monitoredHost->ipAddress)));
			returnCode = purgeCatalogFromHost(monitoredHost->ipAddress);
			if (returnCode < 0)
				systemLog->sysLog(ERROR, "cannot purge catalog entries from host %s", inet_ntoa(*((struct in_addr *)&monitoredHost->ipAddress)));
			serverList->removeElement(i);
			return 0;
		}
	}
	
	return 0;
}

void MulticastServerCatalog::run(void) {
	socketMsg *socketMessage;
	int returnCode;
	struct pollfd fds;
	int timeout;
	time_t pollStart;
	time_t pollEnd;
	bool timeoutExpired = false;

	systemLog->sysLog(NOTICE, "Multicast Catalog server is running and waiting for packets");
	timeout = 5000;
	fds.fd = sd;
	fds.events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;
	for (;;) {
		pollStart = time(NULL);
#ifdef DEBUGCATALOG
		systemLog->sysLog(DEBUG, "starting poll with %d ms of timeout", timeout);
#endif
		returnCode = poll(&fds, 1, timeout);
		if (returnCode < 0) {
			systemLog->sysLog(ERROR, "cannot poll on socket descriptor %d", sd);
			continue;
		}
		if (! returnCode)
			timeoutExpired = true;
		pollEnd = time(NULL);
		timeout -= ((pollEnd - pollStart) * 1000);
		if ((timeout <= 0) || (timeoutExpired == true)) {
			timeout = 5000;
			returnCode = sendPingPacket();
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "cannot send ping packet: %s", strerror(errno));
				continue;
			}
			returnCode = verifyTimeouts();
			if (returnCode < 0)
				systemLog->sysLog(ERROR, (const char *)"cannot verify that servers are alive");
			if (timeoutExpired == true) {
				timeoutExpired = false;
				continue;
			}
		}
		socketMessage = recvMessage(sd);
#ifdef DEBUGCATALOG
		systemLog->sysLog(DEBUG, "multicast packet size is %d", socketMessage->brecv);
#endif
		if (socketMessage) {
			returnCode = decodePacket(socketMessage->recvmsg);
			free(socketMessage);
			socketMessage = NULL;
		}
	}
}

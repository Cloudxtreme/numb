/***************************************************************************
                          administrationserverconnection.cpp  -  description
                             -------------------
    begin                : Tue Jun 24 2003
    copyright            : (C) 2003 by spe
    email                : spe@artik.intra.selectbourse.net
 ***************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "administrationserverconnection.h"
#include "../toolkit/parser.h"
#include "../toolkit/list.h"
#include "../toolkit/mystring.h"
#include "../src/catalogdata.h"

const char *cmdAuth = "AUTH";
const char *cmdReload = "RELOAD";
const char *cmdStats = "STATS";
const char *cmdPing = "PING";
const char *cmdVersion = "VERSION";
const char *cmdQuit = "QUIT";
const char *cmdSet = "SET";
const char *cmdGetCatalog = "GETCATALOG";

AdministrationServerConnection::AdministrationServerConnection(Server *_server, Configuration *_configuration, struct sockaddr_in *_sourceAddress, HashTable *_catalogHashtable) {
	administrationServerConnectionDebug = false;
	administrationServerConnectionInitialized = false;
	server = _server;
	commandsList = new const char *[9];
	commandsList[0] = cmdAuth;
	commandsList[1] = cmdReload;
	commandsList[2] = cmdStats;
	commandsList[3] = cmdPing;
	commandsList[4] = cmdVersion;
	commandsList[5] = cmdQuit;
	commandsList[6] = cmdSet;
	commandsList[7] = cmdGetCatalog;
	commandsList[8] = NULL;
	serverAnswer = new String(1024);
	if (! serverAnswer) {
		systemLog->sysLog(ERROR, "cannot allocate memory for serverAnswer. Error is %s", strerror(errno));
		return;
	}
	if (! _configuration) {
		systemLog->sysLog(ERROR, "Configuration object is NULL. cannot initialize server connection");
		return;
	}
	else
		configuration = _configuration;
	sourceAddress = _sourceAddress;
	sessionAuthenticated = false;
	catalogHashtable = _catalogHashtable;
	administrationServerConnectionInitialized = true;

	return;
}

AdministrationServerConnection::~AdministrationServerConnection() {
	administrationServerConnectionDebug = false;
	administrationServerConnectionInitialized = false;
	if (serverAnswer)
		delete serverAnswer;
	if (sourceAddress)
		free(sourceAddress);

	return;
}

void AdministrationServerConnection::serverMessage(unsigned short errorCode) {
	socketMsg smsg;

	switch (errorCode) {
		case 101:
			serverAnswer->stringNCopy("101 GoodBye !\n", serverAnswer->getBlocSize());
			break;
		case 200:
			serverAnswer->stringNCopy("200 DAEMONRELOAD OK!\n", serverAnswer->getBlocSize());
			break;
		case 201:
			serverAnswer->snPrintf("%d AUTH successfull\n", errorCode);
			break;
		case 202:
			serverAnswer->stringNCopy("202 PING OK!\n", serverAnswer->getBlocSize());
			break;
		case 203:
			serverAnswer->snPrintf("203 VERSION: %s\n", configuration->revision);
			break;
		case 204:
			serverAnswer->snPrintf("%d SET successfull\n", errorCode);
			break;
		case 205:
			serverAnswer->snPrintf("%d GETCATALOG successfull\n", errorCode);
			break;
		case 300:
			serverAnswer->stringNCopy("300 AUTH syntax is <user> <pass>\n", serverAnswer->getBlocSize());
			break;
		case 301:
			serverAnswer->stringNCopy("301 SET syntax is <keyword> <value>\n", serverAnswer->getBlocSize());
			break;
		case 302:
			serverAnswer->snPrintf("%d SET <keyword> or <value> is invalid\n", errorCode);
			break;
		case 303:
			serverAnswer->stringNCopy("303 RELOAD failure during reload process\n", serverAnswer->getBlocSize());
			break;
		case 304:
			serverAnswer->stringNCopy("304 STATS unexpected error in stats delivery\n", serverAnswer->getBlocSize());
			break;
		case 305:
			serverAnswer->stringNCopy("305 PING unexpected error in stats delivery\n", serverAnswer->getBlocSize());
			break;
		case 306:
			serverAnswer->snPrintf("%d AUTH failed, sorry :(\n", errorCode);
			break;
		case 400:
			serverAnswer->snPrintf("%d you must be authenticated, please use AUTH <user> <pass>\n", errorCode);
			break;
		case 401:
			serverAnswer->snPrintf("%d catalog hashtable is empty\n", errorCode);
			break;
		case 501:
			serverAnswer->stringNCopy("501 Arguments cannot be parsed correctly\n", serverAnswer->getBlocSize());
			break;
		case 502:
			serverAnswer->snPrintf("%d internal server error\n", errorCode);
			break;
		default:
			serverAnswer->stringNCopy("500 Command unknown\n", serverAnswer->getBlocSize());
			break;
	}
	smsg.len = strlen(serverAnswer->bloc);
	smsg.sendmsg = serverAnswer->bloc;
	server->sendMessage(clientSocket, &smsg);

	return;
}

bool AdministrationServerConnection::checkAuthentication(void) {
	if (sessionAuthenticated == false) {
		serverMessage(400);
		return false;
	}

	return true;
}

bool AdministrationServerConnection::executeCommand(socketMsg *smsg) {
	Parser *parseCommand;
	Parser *parseArgument;
	List<Memory<char> *> *tokensCommand;
	List<Memory<char> *> *tokensArgument;
	int **hashtablePtr;
	int hashtableSize;
	HashTableElt *hashtableElt;
	struct CatalogData *catalogData;
	socketMsg smsgSend;
	size_t sizeSent;
	char specialDelimiter[2];
	char timeBuffer[64];
	int counter = 0;
	bool wantToQuit = false;
	char *argument;
	int i;
	size_t catalogBufferSize;
	char *catalogBuffer;
	int returnCode;

	specialDelimiter[0] = ';';
	specialDelimiter[1] = 0x00;
	parseCommand = new Parser(" ");
	parseArgument = new Parser(specialDelimiter);
	tokensCommand = parseCommand->tokenizeData(smsg->recvmsg, 1, smsg->brecv);
	while (commandsList[counter]) {
		if (! strncmp(tokensCommand->getFirstElement()->bloc, commandsList[counter], strlen(commandsList[counter])))
			break;
		counter++;
	}
	if (! commandsList[counter]) {
		systemLog->sysLog(ERROR, "( %s:%d ) unrecognized administration command : %s\n", inet_ntoa(sourceAddress->sin_addr), ntohs(sourceAddress->sin_port), tokensCommand->getFirstElement()->bloc);
		counter = -1;
	}
	tokensCommand->removeFirst();
  
	// Local Initializations
	serverAnswer->initialize();
	bzero(timeBuffer, sizeof(timeBuffer));

	switch (counter) {
	case 0:
		/* AUTH <user> <pass> - authentication */
		tokensArgument = parseArgument->tokenizeData(tokensCommand->getFirstElement()->bloc, 2, tokensCommand->getFirstElement()->getBlocSize());
		if ((! tokensArgument) || (tokensArgument->getListSize() != 2)) {
			serverMessage(300);
			break;
		}
		if (! strcmp(tokensArgument->getElement(1)->getBloc(), "admin")) {
			if (! strncmp(tokensArgument->getElement(2)->getBloc(), "a02sg32", 7)) {
				serverMessage(201);
				sessionAuthenticated = true;
			}
			else
				serverMessage(306);
		}
		else
			serverMessage(306);
		delete tokensArgument;
		break;
	case 1:
		/* DAEMONRELOAD - reload configuration */
		if (checkAuthentication() == true) {
			configuration->parseConfigurationFile();
			serverMessage(200);
		}
		break;
	case 2:
		/* DAEMONSTATS - get the stats */
		if (checkAuthentication() == true)
			serverMessage(201);
		break;
	case 3:
		/* DAEMONPING - check if daemon is ok */
		if (checkAuthentication() == true)
			serverMessage(305);
		break;
	case 4:
		/* DAEMONVERSION - return the daemon revision */
		if (checkAuthentication() == true)
			serverMessage(203);
		break;
	case 5:
		/* QUIT - quit the administration interface */
		serverMessage(101);
		wantToQuit = true;
		break;
	case 6:
		/* SET - set some startup parameters */
		if (checkAuthentication() == false)
			break;
		tokensArgument = parseArgument->tokenizeData(tokensCommand->getFirstElement()->bloc, 2, tokensCommand->getFirstElement()->getBlocSize());
		if ((! tokensArgument) || (tokensArgument->getListSize() != 2)) {
			serverMessage(301);
			break;
		}
		i = 0;
		argument = tokensArgument->getElement(2)->bloc;
		while (argument[i]) {
			if ((argument[i] == '\n') || (argument[i] == '\r'))
				argument[i] = '\0';
			i++;
		}
		if (! strcmp(tokensArgument->getElement(1)->bloc, "nokeycheck")) {
			if (! strcmp(tokensArgument->getElement(2)->bloc, "true")) {
				configuration->noKeyCheck = true;
				serverMessage(204);
			}
			else
				if (! strcmp(tokensArgument->getElement(2)->bloc, "false")) {
					configuration->noKeyCheck = false;
					serverMessage(204);
				}
				else
					serverMessage(302);
			delete tokensArgument;
			break;
		}
		if (! strcmp(tokensArgument->getElement(1)->bloc, "keytimeout")) {
			int keyTimeout;

			keyTimeout = atoi(tokensArgument->getElement(2)->bloc);
			if (keyTimeout) {
				configuration->keyTimeout = keyTimeout;
				serverMessage(204);
			}
			else
				serverMessage(302);
			delete tokensArgument;
			break;
		}
		if (! strcmp(tokensArgument->getElement(1)->bloc, "readtimeout")) {
			int readTimeout;

			readTimeout = atoi(tokensArgument->getElement(2)->bloc);
			if (readTimeout) {
				configuration->readTimeout = readTimeout;
				serverMessage(204);
			}
			else
				serverMessage(302);
			delete tokensArgument;
			break;
		}
		if (! strcmp(tokensArgument->getElement(1)->bloc, "writetimeout")) {
			int writeTimeout;

			writeTimeout = atoi(tokensArgument->getElement(2)->bloc);
			if (writeTimeout) {
				configuration->writeTimeout = writeTimeout;
				serverMessage(204);
			}
			else
				serverMessage(302);
			delete tokensArgument;
			break;
		}
		if (! strcmp(tokensArgument->getElement(1)->bloc, "shappingtimeout")) {
			int shappingTimeout;

			shappingTimeout = atoi(tokensArgument->getElement(2)->bloc);
			if (shappingTimeout) {
				configuration->shappingTimeout = shappingTimeout;
				serverMessage(204);
			}
			else
				serverMessage(302);
			delete tokensArgument;
			break;
		}
		
		serverMessage(302);
		delete tokensArgument;
		break;
	case 7:
		catalogHashtable->lock();
		hashtablePtr = catalogHashtable->getHashtable();
		hashtableSize = catalogHashtable->getSize();
		i = 0;
		if (! catalogHashtable->getNumberOfElements()) {
			serverMessage(401);
			catalogHashtable->unlock();
			break;
		}
		catalogBufferSize = 0;
		catalogBuffer = (char *)malloc(1);
		catalogBuffer[0] = '\0';
		while (i < hashtableSize) {
			if (hashtablePtr[i]) {
				hashtableElt = (HashTableElt *)hashtablePtr[i];
				while (hashtableElt) {
					catalogData = (struct CatalogData *)hashtableElt->getData();
					serverAnswer->snPrintf("%s|%s|%d\n", hashtableElt->getKey(), inet_ntoa(*((struct in_addr *)&catalogData->host)), catalogData->counter);
					catalogBufferSize += strlen(serverAnswer->bloc);
					catalogBuffer = (char *)realloc(catalogBuffer, catalogBufferSize+1);
					if (! catalogBuffer) {
						systemLog->sysLog(CRITICAL, "cannot reallocate catalogBuffer: %s", strerror(errno));
						serverMessage(502);
						catalogHashtable->unlock();
						break;
					}
					strcat(catalogBuffer, serverAnswer->bloc);
					hashtableElt = hashtableElt->getNext();
				}
			}
			i++;
		}
		catalogHashtable->unlock();
		serverMessage(205);
		usleep(500);
		serverAnswer->snPrintf("%d\n", catalogBufferSize);
		smsgSend.len = strlen(serverAnswer->bloc);
		smsgSend.sendmsg = serverAnswer->bloc;
		server->sendMessage(clientSocket, &smsgSend);
		usleep(500);
		sizeSent = 0;
		while (sizeSent != catalogBufferSize) {
			smsgSend.len = catalogBufferSize - sizeSent;
			smsgSend.sendmsg = &catalogBuffer[sizeSent];
			returnCode = server->sendMessage(clientSocket, &smsgSend);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "cannot send catalog buffer to client: %s", strerror(errno));
				free(catalogBuffer);
				return -1;
			}
			sizeSent += smsgSend.bsent;
		}
		if (catalogBuffer)
			free(catalogBuffer);

		break;
	default:
		serverMessage(500);
		break;
	}

	delete tokensCommand;
	delete parseCommand;
	delete parseArgument;

	return wantToQuit;
}

int AdministrationServerConnection::presentServer(void) {
	socketMsg smsg;
	char banner[2048];

	snprintf(banner, sizeof(banner), "100 Welcome to %s\n", configuration->revision);
	if (administrationServerConnectionInitialized == false) {
		systemLog->sysLog(ERROR, "cannot continue because AdministrationServerConnection object is not initialized correctly");
		return -1;
	}
	smsg.len = strlen(banner);
	smsg.sendmsg = banner;

	return server->sendMessage(clientSocket, &smsg);  
}

void AdministrationServerConnection::newSession(void *arguments) {
	socketMsg *smsg;
	bool wantToQuit = false;

	// setting client socket descriptor in the class
	clientSocket = (uint64_t)arguments;

	// server presentation banner
	if (presentServer() < 0) {
		close(clientSocket);
		return;
	}

	// now we can wait commands on the socket
	// ending when client close the socket or socket error
	while (! wantToQuit) {
		smsg = server->recvMessage(clientSocket);
		if (! smsg) {
			close(clientSocket);
			return;
		}
		wantToQuit = executeCommand(smsg);

		// Free allocated message (in recvMessage)
		if (smsg)
			free(smsg);
	}

	close(clientSocket);

	return;
}

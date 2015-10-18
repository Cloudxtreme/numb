/***************************************************************************
                          configuration.cpp  -  description
                             -------------------
    begin                : Dim dec 27 2007
    copyright            : (C) 2007 by spe
    email                : spebsd@gmail.com
 ***************************************************************************/

#include "configuration.h"
#include "../toolkit/parser.h"
#include "../toolkit/mystring.h"

Configuration::Configuration(String *_configurationFileName) {
	configurationInitialized = 0;
	configurationFileName = new String(_configurationFileName->bloc);
	openFile();
	strncpy(multicastIp, "224.0.0.3", sizeof(multicastIp)-1);
	multicastIp[sizeof(multicastIp) - 1] = '\0';
	multicastPort = 4446;
	multicastCatalogPort = 4447;
	sourceMulticastIp = INADDR_ANY;
	noCache = false;
	noKeyCheck = false;
	noDaemon = false;
	originServerUrl[0][0] = '\0';
	originServerUrl[1][0] = '\0';
	originServerUrl[2][0] = '\0';
	originServerUrl[3][0] = '\0';
	originServerUrl[4][0] = '\0';
	documentRoot[0] = '\0';
	cacheDirectory[0] = '\0';
	sendBuffer = 131072;
	receiveBuffer = 131072;
	strncpy(logFile, "/var/log/numb.log", sizeof(logFile)-1);
	logFile[sizeof(logFile)-1] = '\0';
	logMode = SYSLOG;
	readTimeout = 10000;
	writeTimeout = 30000;
	shappingTimeout = 0;
	keyTimeout = 180000;
	listenQueue = 20000;
	strncpy(revision, "numb revision 2.0 / Sebastien Petit <spebsd@gmail.com>", sizeof(revision)-1);
	revision[sizeof(revision)-1] = '\0';
	administrationServerEnable = false;
	proxyName = NULL;
	shareCatalog = false;
	workerNumber = 2;
	cacheTimeout = 43200000;
	burst = 0;
	aesKey = NULL;
	noByteRange[0] = '\0';
	listeningPort = 80;
	administrationServerPort = 9321;
	
	configurationFileNameOpened = true;
	configurationInitialized = 1;

	return;
}

Configuration::Configuration() {
	configurationInitialized = 0;
	strncpy(multicastIp, "224.0.0.3", sizeof(multicastIp)-1);
	multicastIp[sizeof(multicastIp)-1] = '\0';
	multicastPort = 4446;
	multicastCatalogPort = 4447;
	sourceMulticastIp = INADDR_ANY;
	configurationFileName = new String("/usr/local/etc/numb.conf");
	configurationFileNameOpened = false;
	noCache = false;
	noKeyCheck = false;
	noDaemon = false;
	originServerUrl[0][0] = '\0';
	originServerUrl[1][0] = '\0';
	originServerUrl[2][0] = '\0';
	originServerUrl[3][0] = '\0';
	originServerUrl[4][0] = '\0';
	documentRoot[0] = '\0';
	cacheDirectory[0] = '\0';
	sendBuffer = 131072;
	receiveBuffer = 131072;
	strncpy(logFile, "/var/log/numb.log", sizeof(logFile)-1);
	logFile[sizeof(logFile)-1] = '\0';
	logMode = SYSLOG;
	readTimeout = 10000;
	writeTimeout = 30000;
	shappingTimeout = 0;
	keyTimeout = 180000;
	listenQueue = 20000;
	strncpy(revision, "numb revision 2.0 / Sebastien Petit <spebsd@gmail.com>", sizeof(revision)-1);
	revision[sizeof(revision)-1] = '\0';
	administrationServerEnable = false;
	proxyName = NULL;
	shareCatalog = false;
	workerNumber = 2;
	cacheTimeout = 86400000;
	burst = 0;
	aesKey = NULL;
	aesVHost[0] = '\0';
	noByteRange[0] = '\0';
	listeningPort = 80;
	administrationServerPort = 9321;

	configurationInitialized = 1;

	return;
}

Configuration::~Configuration() {
	if (configurationInitialized) {
		if (configurationFile->closeStreamFile())
			systemLog->sysLog(ERROR, "configuration file is not closed correctly, a file descriptor may be lose\n");
	}
	if (configurationFile)
		delete configurationFile;
	configurationFile = NULL;
	if (proxyName)
		delete proxyName;
	proxyName = NULL;
	if (aesKey)
		delete aesKey;
	aesKey = NULL;
	configurationInitialized = 0;

	return;
}

int Configuration::openFile(void) {
	configurationFile = new File(configurationFileName, "r", 1);
	if (! configurationFile->getStreamFileInitialized()) {
		systemLog->sysLog(ERROR, "configuration file is not opened, cannot initialize Configuration Object correctly\n");
		return -1;
	}
	configurationFileNameOpened = true;

	return 0;
}

void Configuration::configurationLineError(String *configurationLine) {
	systemLog->sysLog(ERROR, "syntax error in configuration line: %s", configurationLine->getBloc());
	return;
}

int Configuration::parseConfigurationFile(void) {
	Parser parseCommands("=");
	List<String *> *tokenCommand = NULL;
	String *configurationLine;

	if (! configurationInitialized) {
		systemLog->sysLog(ERROR, "configuration file is not initialized. cannot load values");
		return EINVAL;
	}
  
	while (! configurationFile->feofStreamFile()) {
		configurationLine = configurationFile->getStringStreamFile();
		tokenCommand = parseCommands.tokenizeString(configurationLine->getBloc(), 255);

		if (tokenCommand->getListSize() < 2) {
			systemLog->sysLog(ERROR, "error in configuration file (skipping): %s", configurationLine->getBloc());
			delete tokenCommand;
			delete configurationLine;
			continue;
		}
		// We must destroy char * pointer with a free call
		tokenCommand->setDestroyData(2);

		// Deleting Spaces and Tabulations from tokenCommand
		tokenCommand->getFirstElement()->deleteWhiteSpaces();
		tokenCommand->getElement(2)->deleteWhiteSpaces();

		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "multicastip")) {
			tokenCommand->removeFirst();
			strncpy(multicastIp, tokenCommand->getFirstElement()->bloc, sizeof(multicastIp)-1);
			multicastIp[sizeof(multicastIp)-1] = '\0';
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "multicastport")) {
			tokenCommand->removeFirst();
			multicastPort = atoi(tokenCommand->getFirstElement()->bloc);
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "multicastcatalogport")) {
			tokenCommand->removeFirst();
			multicastCatalogPort = atoi(tokenCommand->getFirstElement()->bloc);
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "originserverurl")) {
			tokenCommand->removeFirst();
			/*strncpy(originServerUrl, tokenCommand->getFirstElement()->bloc, sizeof(originServerUrl)-1);
			originServerUrl[sizeof(originServerUrl)-1] = '\0';*/
			/* XXX a faire */
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "listeningport")) {
			tokenCommand->removeFirst();
			listeningPort = atoi(tokenCommand->getFirstElement()->bloc);
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "nodaemon")) {
			tokenCommand->removeFirst();
			if (! strcmp(tokenCommand->getFirstElement()->bloc, "yes"))
				noDaemon = true;
			else
				noDaemon = false;
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "nokeycheck")) {
			tokenCommand->removeFirst();
			if (! strcmp(tokenCommand->getFirstElement()->bloc, "yes"))
				noKeyCheck = true;
			else
				noKeyCheck = false;
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "userid")) {
			tokenCommand->removeFirst();
			userId = atoi(tokenCommand->getFirstElement()->bloc);
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "groupid")) {
			tokenCommand->removeFirst();
			groupId = atoi(tokenCommand->getFirstElement()->bloc);
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "documentroot")) {
			tokenCommand->removeFirst();
			strncpy(documentRoot, tokenCommand->getFirstElement()->bloc, sizeof(documentRoot)-1);
			documentRoot[sizeof(documentRoot)-1] = '\0';
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "nocache")) {
			tokenCommand->removeFirst();
			if (! strcmp(tokenCommand->getFirstElement()->bloc, "yes"))
				noCache = true;
			else
				noCache = false;
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "cachedir")) {
			tokenCommand->removeFirst();
			strncpy(cacheDirectory, tokenCommand->getFirstElement()->bloc, sizeof(cacheDirectory)-1);
			cacheDirectory[sizeof(cacheDirectory)-1] = '\0';
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "sendbuffer")) {
			tokenCommand->removeFirst();
			sendBuffer = atoi(tokenCommand->getFirstElement()->getBloc());
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "recvbuffer")) {
			tokenCommand->removeFirst();
			receiveBuffer = atoi(tokenCommand->getFirstElement()->getBloc());
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "log")) {
			tokenCommand->removeFirst();
			if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "syslog"))
				logMode = SYSLOG;
			if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "localfile"))
				logMode = LOCALFILE;
			if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "stdout"))
				logMode = STDOUT;
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "logfile")) {
			tokenCommand->removeFirst();
			strncpy(logFile, tokenCommand->getFirstElement()->getBloc(), sizeof(logFile)-1);
			logFile[sizeof(logFile)-1] = '\0';
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "readtimeout")) {
			tokenCommand->removeFirst();
			readTimeout = atoi(tokenCommand->getFirstElement()->getBloc());
			readTimeout *= 1000;
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "writetimeout")) {
			tokenCommand->removeFirst();
			writeTimeout = atoi(tokenCommand->getFirstElement()->getBloc());
			writeTimeout *= 1000;
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "shappingtimeout")) {
			tokenCommand->removeFirst();
			shappingTimeout = atoi(tokenCommand->getFirstElement()->getBloc());
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "keytimeout")) {
			tokenCommand->removeFirst();
			keyTimeout = atoi(tokenCommand->getFirstElement()->getBloc());
			keyTimeout *= 1000;
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "sharecatalog")) {
			tokenCommand->removeFirst();
			if (! strcasecmp(tokenCommand->getFirstElement()->getBloc(), "yes"))
				shareCatalog = true;
			else
				shareCatalog = false;
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "workerthreads")) {
			tokenCommand->removeFirst();
			workerNumber = atoi(tokenCommand->getFirstElement()->getBloc());
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "cachetimeout")) {
			tokenCommand->removeFirst();
			cacheTimeout = atoi(tokenCommand->getFirstElement()->getBloc());
			cacheTimeout *= 1000;
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "burst")) {
			tokenCommand->removeFirst();
			burst = atoi(tokenCommand->getFirstElement()->getBloc());
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "aeskey")) {
			tokenCommand->removeFirst();
			
			String hexKey(tokenCommand->getFirstElement()->getBloc());
			char *binary;
			int binaryLength;
			int returnCode;

			returnCode = hexKey.hexToBinary(&binary, &binaryLength);
			if ((returnCode == -1) || (binaryLength != 32)) {
				systemLog->sysLog(ERROR, "invalid value '%s' for option 'aeskey'\n", hexKey.bloc);
				exit(EXIT_FAILURE);
			}

			aesKeySize = binaryLength;
			aesKey = (char *)malloc(binaryLength);
			memcpy(aesKey, binary, binaryLength);
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(), "aesvhost")) {
			tokenCommand->removeFirst();
			strncpy(aesVHost, tokenCommand->getFirstElement()->getBloc(), sizeof(aesVHost)-1);
			logFile[sizeof(aesVHost)-1] = '\0';
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(),"nobyterange")) {
			tokenCommand->removeFirst();
			strncpy(noByteRange, tokenCommand->getFirstElement()->getBloc(), sizeof(noByteRange)-1);
			logFile[sizeof(noByteRange)-1] = '\0';
		}
		if (! strcmp(tokenCommand->getFirstElement()->getBloc(),"administrationserverport")) {
			tokenCommand->removeFirst();
			administrationServerPort = atoi(tokenCommand->getFirstElement()->getBloc());
		}

		if (tokenCommand)
			delete tokenCommand;

		delete configurationLine;
  	}

	return 0;
}

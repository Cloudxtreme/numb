/***************************************************************************
                          configuration.h  -  description
                             -------------------
    begin                : Dim dec 27 2007
    copyright            : (C) 2007 by spe
    email                : spebsd@gmail.com
 ***************************************************************************/

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

#include "../toolkit/mystring.h"
#include "../toolkit/file.h"

/**
  *@author spe
  */

class Configuration {
private:
	bool configurationFileNameOpened;
	char configurationInitialized;
	File *configurationFile;
public:
	char multicastIp[16];
	unsigned short multicastPort;
	unsigned short multicastCatalogPort;
	in_addr_t sourceMulticastIp;
	unsigned short listeningPort;
	char originServerUrl[16][256];
	char documentRoot[1024];
	char cacheDirectory[1024];
	String *configurationFileName;
	bool noDaemon;
	bool noKeyCheck;
	bool noCache;
	uid_t userId;
	gid_t groupId;
	int sendBuffer;
	int receiveBuffer;
	char logFile[1024];
	char logMode;
	int readTimeout;
	int writeTimeout;
	int shappingTimeout;
	int keyTimeout;
	int listenQueue;
	char revision[256];
	bool administrationServerEnable;
	int administrationServerPort;
	char *proxyName;
	struct in_addr proxyIp;
	bool shareCatalog;
	unsigned char workerNumber;
	unsigned int cacheTimeout;
	char burst;
	char *aesKey;
	int aesKeySize;
	char aesVHost[128];
	char noByteRange[1024];

	Configuration(String *);
	Configuration();
	~Configuration();
	int openFile(void);
	void configurationLineError(String *);
	int parseConfigurationFile(void);
	void setConfigurationFileName(String *_configurationFileName) { if (configurationFileName) delete configurationFileName; configurationFileName = new String(_configurationFileName->bloc); return; }
};

#endif

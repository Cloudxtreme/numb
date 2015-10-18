//
// C++ Implementation: streamer
//
// Description: 
//
//
// Author: Sebastien Petit <spebsd@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "streamer.h"
#include "../toolkit/thread.h"
#include "../toolkit/hashalgorithm.h"
#include "../src/configuration.h"
#include "httpclientconnection.h"
#include "../src/keyhashtabletimeout.h"
#include "../src/administrationserver.h"
#include "../src/monitoredhost.h"
#include "../src/catalogdata.h"
#include "../toolkit/hashtable.h"
#include "../src/multicastpacketcatalog.h"
#include "../toolkit/mystring.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/event.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <dirent.h>

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

char *stringType[] = { (char *)"", (char *)"PINT", (char *)"EXT", (char *)"VBINT", (char *)NULL };

Streamer::Streamer() {
	catalogHashtableTimeout = NULL;

	return;
}

Streamer::~Streamer() {
	return;
}

void Streamer::errorConfigurationFileAndOptions(void) {
	systemLog->sysLog(ERROR, "you can't specify a configuration file name with command line options");
	systemLog->sysLog(ERROR, "--config/-c <config_file> is mutually exclusive with other options");
	fprintf(stderr, "cannot use configuration file name with command line options setted\n");
	fprintf(stderr, "--config/-c <config_file> is mutually exclusive with other options\n");
	
	return;
}

void Streamer::usage(char *processName) {
	fprintf(stderr, "Numb - A streaming daemon using HTTP protocol / Kqueue / FreeBSD\n");
	fprintf(stderr, "2006-2007 Sebastien Petit\n");
	fprintf(stderr, "Usage: %s [ -c <config_file> | -p <port_listening> -M <multicast_ip> -P <multicast_port> -u <user_id> -g <group_id> -d -k ] < -o <origin_server_url> -C <cache_directory_path> | -n > -r <document_root_path>\n", processName);
	fprintf(stderr, "	--config/-c		Give the configuration file path (default: /usr/local/etc/numb.conf)\n");
	fprintf(stderr, "	--listeningport/-p	Set the listening port (default: 80)\n");
	fprintf(stderr, "	--maxconnections/-m	Set the maximum number of simultaneous connections allowed (default: 20000)\n");
	fprintf(stderr, "	--multicasip/-M		Set the multicast IP adress (default: 224.0.0.3)\n");
	fprintf(stderr, "	--multicastport/-P	Set the multicast UDP destination port (default: 4446)\n");
	fprintf(stderr, "	--nodaemon/-d		Don't go to background\n");
	fprintf(stderr, "	--nokeycheck/-k		Don't test the key validity for returning files (http://.../?key=...)\n");
	fprintf(stderr, "	--userid/-u		Set the effective user id for running process\n");
	fprintf(stderr, "	--groupid/-g		Set the effective group id for running process\n");
	fprintf(stderr, "	--documentroot/-r	Set the default document root for the HTTP server\n");
	fprintf(stderr, "	--originserverurl/-o	Set the origin HTTP server to cache (eg: http://www.foo.org/), you can separate multiple URLs with ';'\n");
	fprintf(stderr, "	--cachedir/-C		Set the cache directory path for caching files from origin server url (eg:/usr/local/cache)\n");
	fprintf(stderr, "	--nocache/-n		Use numb as a classic static HTTP server (no cache)\n");
	fprintf(stderr, "	--sendbuffer/-s		Set the size of the socket send buffer (default: 131072)\n");
	fprintf(stderr, "	--recvbuffer/-b		Set the size of the socket receive buffer (default: 131072)\n");
	fprintf(stderr, "	--log/-l		Log message to <stdout> / <syslog> / <localfile> (default: syslog)\n");
	fprintf(stderr, "	--logfile/-f		Specify the file name for logging when --log=localfile is specified (default: /var/log/numb.log)\n");
	fprintf(stderr, "	--readtimeout/-R	HTTP read timeout in seconds (default: 10s)\n");
	fprintf(stderr, "	--writetimeout/-W	HTTP write timeout in seconds (default: 30s)\n");
	fprintf(stderr, "	--keytimeout/-K		Timeout for deleting non requested keys in seconds (default: 180s)\n");
	fprintf(stderr, "	--adminserver/-A	Enable administration server (default: Disabled)\n");
	fprintf(stderr, "	--shapping/-S		Shapping in kbits/s (default: none)\n");
	fprintf(stderr, "	--sharecatalog/-H	Enable distributed cache system via multicast (default: Disabled)\n");
	fprintf(stderr, "	--workerthreads/-w	Number of worker thread that takes events on the pool (default: 2)\n");
	fprintf(stderr, "	--cachetimeout/-a	Timeout for objects in disk/memory cache in seconds (default: 86400s)\n");
	fprintf(stderr, "	--burst/-B		Number of packets to burst in the beginning of the connection (default: 0)\n");
	fprintf(stderr, "	--aeskey/-e		AES key (256 Bits) in hexadecimal format for decrypting relative URL\n");
	fprintf(stderr,	"	--aesvhost/-v		AES Virtual Host name (used to detect if we must decrypt relative url with\n");
	fprintf(stderr, "	             		<aeskey> parameter or not\n");
	fprintf(stderr, "	--help/-h		This help *TTooooooooooo*\n");
	fprintf(stderr, "\n");

	return;
}

int Streamer::parseAndLoadCatalog(char *catalogBuffer) {
	Parser *parser;
	Parser *parser2;
	char delimiter[2];
	char delimiter2[2];
	List<String *> *catalogEntriesList;
	List<String *> *catalogArgumentsList;
	struct CatalogData *catalogData;
	char *key;
	HashTableElt *hashtableElt;
	uint32_t hashPosition;

	delimiter[0] = '\n';
        delimiter[1] = '\0';
	delimiter2[0] = '|';
	delimiter2[1] = '\0';
        parser = new Parser(delimiter);
	if (! parser) {
		systemLog->sysLog(CRITICAL, "cannot create a Parser object for loading catalog: %s", strerror(errno));
		return -1;
	}
	parser2 = new Parser(delimiter2);
	if (! parser2) {
		systemLog->sysLog(CRITICAL, "cannot create a Parser object for loading catalog: %s", strerror(errno));
		delete parser;
		return -1;
	}
	catalogEntriesList = parser->tokenizeString(catalogBuffer, strlen(catalogBuffer));
	if (! catalogEntriesList) {
		delete parser;
		delete parser2;
		return -1;
	}
	while (catalogEntriesList->getListSize()) {
		catalogArgumentsList = parser2->tokenizeString(catalogEntriesList->getFirstElement()->bloc, strlen(catalogEntriesList->getFirstElement()->bloc));
		catalogData = (struct CatalogData *)malloc(sizeof(struct CatalogData));
		key = (char *)malloc(strlen(catalogArgumentsList->getFirstElement()->bloc+1));
		strcpy(key, catalogArgumentsList->getFirstElement()->bloc);
		catalogArgumentsList->removeFirst();
		catalogData->host = inet_addr(catalogArgumentsList->getFirstElement()->bloc);
		catalogArgumentsList->removeFirst();
		catalogData->counter = atoi(catalogArgumentsList->getFirstElement()->bloc);
		catalogArgumentsList->removeFirst();
		if (strstr(key, ".flv") || strstr(key, ".mp4")) {
			catalogHashtable->lock();
			hashtableElt = catalogHashtable->add(key, catalogData, &hashPosition);
			if (! hashtableElt)
				systemLog->sysLog(ERROR, "cannot add a redirect on the catalog hashtable");
			catalogHashtable->unlock();
		}
		delete catalogArgumentsList;
		catalogEntriesList->removeFirst();
	}

	delete catalogEntriesList;
	delete parser;
	delete parser2;

	return 0;
}

int Streamer::listenAndGetCatalog(void) {
	time_t timeout;
	int sDescriptor;
	struct sockaddr_in saddr;
	char buffer[256];
	char *catalogBuffer;
	int catalogBufferSize = 0;
	int returnCode;
	int catalogSize;
	FILE *stream;
	char *returnString;

	timeout = time(NULL) + 10;
	while (time(NULL) < timeout) {
		systemLog->sysLog(INFO, "Start to server objects in %d seconds", timeout - time(NULL));
		if (serverList->getListSize()) {
			systemLog->sysLog(INFO, "a proxy has been found on the cluster");
			systemLog->sysLog(INFO, "asking for transfering complete catalog");
			sDescriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sDescriptor < 0) {
				systemLog->sysLog(ERROR, "cannot create socket: %s", strerror(errno));
				return -1;
			}
			bzero(&saddr, sizeof(saddr));
			saddr.sin_len = sizeof(saddr);
			saddr.sin_family = PF_INET;
			saddr.sin_port = htons(9321);
			saddr.sin_addr.s_addr = serverList->getFirstElement()->ipAddress;
			systemLog->sysLog(DEBUG, "trying to connect to %s:%d", inet_ntoa(*((struct in_addr *)&saddr.sin_addr.s_addr)), ntohs(saddr.sin_port));
			returnCode = connect(sDescriptor, (struct sockaddr *)&saddr, sizeof(saddr));
			if (returnCode == -1) {
				systemLog->sysLog(ERROR, "cannot connect to server %s:%d : %s", inet_ntoa(*((struct in_addr *)&saddr.sin_addr.s_addr)), configuration->listeningPort, strerror(errno));
				close(sDescriptor);
				return -1;
			}
			systemLog->sysLog(DEBUG, "connected to %s:%d", inet_ntoa(*((struct in_addr *)&saddr.sin_addr.s_addr)), ntohs(saddr.sin_port));
			stream = fdopen(sDescriptor, "r+");
			if (! stream) {
				systemLog->sysLog(DEBUG, "cannot convert descriptor to FILE*: %s", strerror(errno));
				fclose(stream);
				close(sDescriptor);
				return -1;
			}
			//returnCode = recv(sDescriptor, buffer, sizeof(buffer)-1, 0);
			returnString = fgets(buffer, sizeof(buffer), stream);
			if (! returnString) {
				systemLog->sysLog(ERROR, "cannot receive hello message from server: %s", strerror(errno));
				fclose(stream);
				close(sDescriptor);
				return -1;
			}
			systemLog->sysLog(DEBUG, "client <- server: hello message form server received: %s", buffer);
			snprintf(buffer, sizeof(buffer), "AUTH admin;a02sg32\n");
			systemLog->sysLog(DEBUG, "client -> server: sending authentication: %s", buffer);
			//returnCode = send(sDescriptor, buffer, strlen(buffer), 0);
			returnCode = fwrite(buffer, sizeof(buffer), 1, stream);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "cannot send buffer [ %s ]: %s", buffer, strerror(errno));
				close(sDescriptor);
				return -1;
			}
			systemLog->sysLog(DEBUG, "client <- server: authentication sent, waiting for response");
			//returnCode = recv(sDescriptor, buffer, sizeof(buffer)-1, 0);
			returnString = fgets(buffer, sizeof(buffer), stream);
			if (! returnString) {
				systemLog->sysLog(ERROR, "cannot receive response from server: %s", strerror(errno));
				close(sDescriptor);
				return -1;
			}
			systemLog->sysLog(DEBUG, "client <- server: authentication response: %s", buffer);
			if (! strstr(buffer, "201")) {
				systemLog->sysLog(ERROR, "authentication is not succesfull, cannot receive catalog");
				close(sDescriptor);
				return -1;
			}
			snprintf(buffer, sizeof(buffer), "GETCATALOG\n");
			systemLog->sysLog(DEBUG, "client -> server: sending %s", buffer);
			//returnCode = send(sDescriptor, buffer, strlen(buffer), 0);
			returnCode = fwrite(buffer, sizeof(buffer), 1, stream);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "cannot send GETCATALOG command to the server: %s", strerror(errno));
				close(sDescriptor);
				return -1;
			}
			systemLog->sysLog(DEBUG, "client -> server: GETCATALOG sent, waiting for response...");
			//returnCode = recv(sDescriptor, buffer, sizeof(buffer)-1, 0);
			returnString = fgets(buffer, sizeof(buffer), stream);
			if (! returnString) {
				systemLog->sysLog(ERROR, "cannot receive response from server: %s", strerror(errno));
				close(sDescriptor);
				return -1;
			}
			systemLog->sysLog(DEBUG, "client <- server: %s", buffer);
			if (strstr(buffer, "401")) {
				systemLog->sysLog(INFO, "catalog is empty on the remote server, beginning with a blank catalog");
				close(sDescriptor);
				return -2;
			}
			if (! strstr(buffer, "205 ")) {
				systemLog->sysLog(ERROR, "cannot execute GETCATALOG command on the server: %s", buffer);
				close(sDescriptor);
				return -1;
			}
			systemLog->sysLog(DEBUG, "client <- server: waiting for catalog size");
			//returnCode = recv(sDescriptor, buffer, sizeof(buffer)-1, 0);
			returnString = fgets(buffer, sizeof(buffer), stream);
			if (! returnString) {
				systemLog->sysLog(ERROR, "cannot receive size of the catalog from server: %s", buffer);
				close(sDescriptor);
				return -1;
			}
			systemLog->sysLog(DEBUG, "client <- server: response is #%s#", buffer);
			catalogSize = atoi(buffer);
			systemLog->sysLog(DEBUG, "client <- server: catalog size is %d bytes", catalogSize);
			catalogBuffer = (char *)malloc(catalogSize+1);
			catalogBuffer[0] = '\0';
			catalogBufferSize = 0;
			while (catalogBufferSize != catalogSize) {
				buffer[0] = '\0';
				systemLog->sysLog(DEBUG, "client <- server: waiting for catalog bytes");
				//returnCode = recv(sDescriptor, buffer, sizeof(buffer)-1, 0);
				returnString = fgets(buffer, sizeof(buffer), stream);
				if (! returnString) {
					systemLog->sysLog(ERROR, "cannot receive response from server for GETCATALOG: %s", strerror(errno));
					close(sDescriptor);
					return -1;
				}
				systemLog->sysLog(DEBUG, "client <- server: catalog bytes received: #%s#", buffer);
				catalogBufferSize += strlen(buffer);
				strcat(catalogBuffer, buffer);
			}
			systemLog->sysLog(INFO, "catalog packet received is #%s#", catalogBuffer);
			returnCode = parseAndLoadCatalog(catalogBuffer);
			if (returnCode < 0)
				systemLog->sysLog(ERROR, "cannot parse and load catalog, continue with an empty catalog");
			free(catalogBuffer);
			snprintf(buffer, sizeof(buffer), "QUIT\n");
			returnCode = send(sDescriptor, buffer, strlen(buffer), 0);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "cannot send QUIT command to the server: %s", strerror(errno));
				close(sDescriptor);
				return -1;
			}
			close(sDescriptor);
			return 0;
		}
		sleep(1);
	}

	return 0;
}

int Streamer::loadExistingDiskCache(char *directory) {
	DIR *dirStream;
	struct dirent *directoryEntry;
	char *absolutePath;
	size_t len;
	char *key;
	struct CatalogData *catalogData;
	HashTableElt *hashtableElt;
	uint32_t hashPosition;
	int returnCode;
	char hostName[256];
	char *buffer;
	size_t bufferLength;
	struct stat st;
	struct timeval now;
	char fullPath[2048];
	int objectTimeout;

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "trying to open directory name %s", directory);
#endif
	dirStream = opendir(directory);
	if (! dirStream) {
		systemLog->sysLog(ERROR, "cannot open directory %s: %s", directory, strerror(errno));
		return -1;
	}
	while ((directoryEntry = readdir(dirStream)) != NULL) {
		if (directoryEntry->d_type == DT_DIR && strcmp(directoryEntry->d_name, ".") && strcmp(directoryEntry->d_name, "..")) {
			len = strlen(directory)+strlen(directoryEntry->d_name)+2;
			absolutePath = new char[len];
			if (! absolutePath) {
				systemLog->sysLog(CRITICAL, "cannot create object absolutePath with %d bytes: %s", len, strerror(errno));
				closedir(dirStream);
				return -1;
			}
			if (! strcmp(directory, "/"))
				snprintf(absolutePath, len, "/%s", directoryEntry->d_name);
			else
				snprintf(absolutePath, len, "%s/%s", directory, directoryEntry->d_name);
			loadExistingDiskCache(absolutePath);
			delete absolutePath;
		}
		if ((directoryEntry->d_type == DT_REG) && (strstr(directoryEntry->d_name, ".flv") || strstr(directoryEntry->d_name, ".mp4"))) {
			len = strlen(directory)+strlen(directoryEntry->d_name)+2;
			absolutePath = (char *)malloc(len);
			if (! absolutePath) {
				systemLog->sysLog(CRITICAL, "cannot create object absolutePath with %d bytes: %s", len, strerror(errno));
				closedir(dirStream);
				return -1;
			}
			snprintf(absolutePath, len, "%s/%s", &directory[strlen(configuration->cacheDirectory)], directoryEntry->d_name);
			key = (char *)malloc(strlen(absolutePath)+1);
			if (! key) {
				systemLog->sysLog(CRITICAL, "cannot allocate key object: %s", strerror(errno));
				return 1;
			}
			strcpy(key, absolutePath);
			returnCode = gettimeofday(&now, NULL);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "cannot gettimeofday: %s", strerror(errno));
				free(key);
				return 1;
			}
			snprintf(fullPath, sizeof(fullPath), "%s%s", configuration->cacheDirectory, key);
			returnCode = lstat(fullPath, &st);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "cannot do lstat on %s: %s", fullPath, strerror(errno));
				free(key);
				return 1;
			}
			if (st.st_atime > now.tv_sec) {
				systemLog->sysLog(INFO, "%s access time in the future, setting timeout to %d", fullPath, configuration->cacheTimeout);
				objectTimeout = configuration->cacheTimeout;
			}
			else {
				if ((unsigned int)(now.tv_sec - st.st_atime) * 1000 >= configuration->cacheTimeout)
					objectTimeout = 1;
				else
					objectTimeout = configuration->cacheTimeout - (now.tv_sec - st.st_atime) * 1000;
			}

			returnCode = gethostname(hostName, sizeof(hostName)-1);
			if (returnCode < 0) {
				systemLog->sysLog(ERROR, "cannot get hostname: %s", strerror(errno));
				free(key);
				return 1;
			}
			catalogData = (struct CatalogData *)malloc(sizeof(struct CatalogData));
			if (! catalogData) {
				systemLog->sysLog(CRITICAL, "cannot create a CatalogData object: %s", strerror(errno));
				free(key);
				return 1;
			}
			catalogData->host = configuration->proxyIp.s_addr;
			catalogData->counter = 0;
			systemLog->sysLog(DEBUG, "[ %s ] -> ( %s ) added to catalog", key, inet_ntoa(*((struct in_addr *)&catalogData->host)));
			catalogHashtable->lock();
			hashtableElt = catalogHashtable->add(key, catalogData, &hashPosition);
			if (! hashtableElt) {
				systemLog->sysLog(ERROR, "cannot add a redirect on the catalog hashtable");
				free(catalogData);
				catalogHashtable->unlock();
				return 1;
			}
			catalogHashtable->unlock();
			printf("setting timeout for %s to %d seconds\n", fullPath, objectTimeout);
			returnCode = catalogHashtableTimeout->add(hashPosition, hashtableElt, objectTimeout);
			if (returnCode < 0) {
				catalogHashtable->lock();
				catalogHashtable->remove(key);
				catalogHashtable->unlock();
				free(catalogData);
				return -1;
			}
			bufferLength = 2+strlen(key)+1+strlen(inet_ntoa(*((struct in_addr *)&catalogData->host)));
			buffer = new char[bufferLength+1];
			if (! buffer) {
				systemLog->sysLog(CRITICAL, "cannot allocate buffer object with %d bytes: %s", bufferLength+1, strerror(errno));
				catalogHashtableTimeout->remove(hashtableElt);
				catalogHashtable->lock();
				catalogHashtable->remove(key);
				catalogHashtable->unlock();
				free(catalogData);
				return -1;
			}
			snprintf(buffer, bufferLength+1, "%d\n%s\n%s", CATALOGADD, inet_ntoa(*((struct in_addr *)&catalogData->host)), key);
#ifdef DEBUGCATALOG
			systemLog->sysLog(DEBUG, "sending multicast packet on network: #%s#", buffer);
#endif
			multicastServerCatalog->sendPacket(buffer, bufferLength);
			delete buffer;
		}
	}
	closedir(dirStream);

	return 0;
}

int Streamer::main(int argc, char **argv) {
	Thread *httpServerWorkerThreads;
	Thread *multicastServerThreads;
	Thread *multicastServerCatalogThreads;
	Thread *keyHashtableTimeoutThread;
	Thread *administrationServerThread;
	HttpClientConnection *httpClientConnection;
	StreamContent *streamContent = NULL;
	HashTable *keyHashtable = NULL;
	KeyHashtableTimeout *keyHashtableTimeout = NULL;
	HashAlgorithm *hashAlgorithm = NULL;
	HashAlgorithm *catalogHashAlgorithm = NULL;
	String *configurationFileName;
	String *hexKey;
	AdministrationServer *administrationServer;
	Parser *parser;
	List<String *> *originServerUrlList;
	const char *parserDelimiter = ",";
	bool optionsSetted = false, configurationFileNameSpecified = false;
	char ch;
	int returnCode;
	int originServerUrlNumber = 0;
	char *startString;
	struct hostent *hostEntry;
	Mutex *slotMutex;
	int *httpSlot;
	int i;

	catalogHashtable = NULL;
	serverList = NULL;

	// Create the LogError object for all classes
	// Default is SYSLOG
	systemLog = new LogError("numb", SYSLOG);

	// Create the configuration object
	configuration = new Configuration();
	if (! configuration) {
		systemLog->sysLog(CRITICAL, "cannot create a Configuration object: %s", strerror(errno));
		fprintf(stderr, "cannot create a Configuration object: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// first... take all options (from configuration file too)
	static struct option longopts[] = {
		{ "config",		required_argument,	NULL,	'c' },
		{ "listeningport",	required_argument,	NULL,	'p' },
		{ "multicastip",	required_argument,	NULL,	'M' },
		{ "multicastport",	required_argument,	NULL,	'P' },
		{ "multicastcatalogport", required_argument,	NULL,	'O' },
		{ "sourcemulticastip", required_argument, NULL, 'U'},
		{ "adminport",		required_argument,	NULL,	'D' },
		{ "nodaemon",		no_argument,		NULL,	'd' },
		{ "nokeycheck",		no_argument,		NULL,	'k' },
		{ "userid",		required_argument,	NULL,	'u' },
		{ "groupid",		required_argument,	NULL,	'g' },
		{ "documentroot",	required_argument,	NULL,	'r' },
		{ "originserverurl",	required_argument,	NULL,	'o' },
		{ "cachedir",		required_argument,	NULL,	'C' },
		{ "cachetimeout",	required_argument,	NULL,	'a' },
		{ "nocache",		no_argument,		NULL,	'n' },
		{ "sendbuffer",		required_argument,	NULL,	's' },
		{ "recvbuffer",		required_argument,	NULL,	'b' },
		{ "log",		required_argument,	NULL,	'l' },
		{ "logfile",		required_argument,	NULL,	'f' },
		{ "readtimeout",	required_argument,	NULL,	'R' },
		{ "writetimeout",	required_argument,	NULL,	'W' },
		{ "workerthreads",	required_argument,	NULL,	'w' },
		{ "shapping",		required_argument,	NULL,	'S' },
		{ "keytimeout",		required_argument,	NULL,	'K' },
		{ "maxconnections",	required_argument,	NULL,	'm' },
		{ "adminserver",	no_argument,		NULL,	'A' },
		{ "sharecatalog",	no_argument,		NULL,	'H' },
		{ "proxyname",		required_argument,	NULL,	'x' },
		{ "burst",		required_argument,	NULL,	'B' },
		{ "aeskey",		required_argument,	NULL,	'e' },
		{ "aesvhost",		required_argument,	NULL,	'v' },
		{ "help",		no_argument,		NULL,	'h' },
		{ "nobyterange",	required_argument,	NULL,	'N' },
		{ NULL,			0,			NULL,	0   }
	};

	while ((ch = getopt_long(argc, argv, "c:p:M:P:dku:g:r:o:O:nhs:b:l:f:R:W:m:Hx:w:a:B:e:v:N:D:U:", longopts, NULL)) != -1) {
		switch (ch) {
			case 'a':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->cacheTimeout = atoi(optarg);
				configuration->cacheTimeout *= 1000;
				
				break;
			case 'c':
				if (optionsSetted == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				delete configuration;
				configurationFileName = new String(optarg);
				configuration = new Configuration(configurationFileName);
				delete configurationFileName;
				configuration->openFile();
				configuration->parseConfigurationFile();
				configurationFileNameSpecified = true;
				break;
			case 'C':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				strncpy(configuration->cacheDirectory, optarg, sizeof(configuration->cacheDirectory)-1);
				configuration->cacheDirectory[sizeof(configuration->cacheDirectory)-1] = '\0';
				break;
			case 'e':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}

				char *binary;
				int binaryLength;
				
				hexKey = new String(optarg);
				returnCode = hexKey->hexToBinary(&binary, &binaryLength);

				if ((returnCode == -1) || (binaryLength != 32)) {
					delete hexKey;
					fprintf(stderr, "invalid value '%s' for option --aesKey\n", optarg);
					usage(argv[0]);
					exit(EXIT_FAILURE);
				}

				configuration->aesKeySize = binaryLength;
				configuration->aesKey = (char *)malloc(binaryLength);
				memcpy(configuration->aesKey, binary, binaryLength);

				delete hexKey;
				
				break;
			case 'p':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->listeningPort = atoi(optarg);
				break;
			case 'M':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				strncpy(configuration->multicastIp, optarg, sizeof(configuration->multicastIp)-1);
				configuration->multicastIp[sizeof(configuration->multicastIp)-1] = '\0';
				break;
			case 'P':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->multicastPort = atoi(optarg);
				break;
			case 'd':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->noDaemon = true;
				break;
			case 'k':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->noKeyCheck = true;
				break;
			case 'u':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->userId = atoi(optarg);
				break;
			case 'U':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->sourceMulticastIp = inet_addr(optarg);
				break;
			case 'v':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				strncpy(configuration->aesVHost, optarg, sizeof(configuration->aesVHost)-1);
				configuration->aesVHost[sizeof(configuration->aesVHost)-1] = '\0';
				break;
			case 'g':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->groupId = atoi(optarg);
				break;
			case 'r':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				strncpy(configuration->documentRoot, optarg, sizeof(configuration->documentRoot)-1);
				configuration->documentRoot[sizeof(configuration->documentRoot)-1] = '\0';
				break;
			case 'o':
				startString = optarg;
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				originServerUrlNumber = 0;
				parser = new Parser(parserDelimiter);
				originServerUrlList = parser->tokenizeString(optarg, 16);
				while (originServerUrlList->getListSize()) {
					snprintf(configuration->originServerUrl[originServerUrlNumber], sizeof(configuration->originServerUrl[originServerUrlNumber]), "%s", originServerUrlList->getFirstElement()->bloc);
					originServerUrlList->removeFirst();
					originServerUrlNumber++;
				}
				delete originServerUrlList;
				delete parser;

				break;
			case 'O':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->multicastCatalogPort = atoi(optarg);
				break;
			case 'n':
				configuration->noCache = true;
				break;
			case 's':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->sendBuffer = atoi(optarg);
				break;
			case 'b':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->receiveBuffer = atoi(optarg);
				break;
			case 'B':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->burst = atoi(optarg);
				break;
			case 'l':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				if (! strcmp(optarg, "syslog"))
					configuration->logMode = SYSLOG;
				if (! strcmp(optarg, "stdout"))
					configuration->logMode = STDOUT;
				if (! strcmp(optarg, "localfile"))
					configuration->logMode = LOCALFILE;
					
				break;
			case 'f':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				strncpy(configuration->logFile, optarg, sizeof(configuration->logFile)-1);
				configuration->logFile[sizeof(configuration->logFile)-1] = '\0';
				break;
			case 'w':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->workerNumber = atoi(optarg);
				break;
			case 'R':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->readTimeout = atoi(optarg);
				configuration->readTimeout *= 1000;
				break;
			case 'W':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->writeTimeout = atoi(optarg);
				configuration->writeTimeout *= 1000;
				break;
			case 'S':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->shappingTimeout = atoi(optarg);				
				break;
			case 'K':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->keyTimeout = atoi(optarg);
				configuration->keyTimeout *= 1000;
				break;
			case 'm':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->listenQueue = atoi(optarg);
				break;
			case 'A':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->administrationServerEnable = true;
				break;
			case 'H':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->shareCatalog = true;
				break;
			case 'x':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->proxyName = new char[strlen(optarg)+1];
				if (! configuration->proxyName) {
					systemLog->sysLog(CRITICAL, "cannot create a configuration->proxyName object: %s", strerror(errno));
					return -1;
				}
				strcpy(configuration->proxyName, optarg);
				hostEntry = gethostbyname2(configuration->proxyName, AF_INET);
				if (! hostEntry) {
					systemLog->sysLog(WARNING, "cannot gethostbyname2 on %s, cannot send catalog multicast: %s", configuration->proxyName, strerror(errno));
					return -1;
				}
				configuration->proxyIp.s_addr = ((struct in_addr *)hostEntry->h_addr_list[0])->s_addr;

				break;
			case 'N':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				strcpy(configuration->noByteRange, optarg);
				break;
			case 'D':
				if (configurationFileNameSpecified == true) {
					errorConfigurationFileAndOptions();
					exit(EXIT_FAILURE);
				}
				configuration->administrationServerPort = atoi(optarg);
				break;
			case 'h':
			default:
				usage(argv[0]);
				exit(EXIT_FAILURE);
				break;
		}
	}
	argc -= optind;
	argv += optind;
	
	// If log mode is specified, must destruct and recreate LogError object !
	if (configuration->logMode != SYSLOG) {
		delete systemLog;
		if (configuration->logMode == LOCALFILE)
			systemLog = new LogError(configuration->logFile, LOCALFILE);
		else
			systemLog = new LogError();
	}

	// Check that all needed arguments are set
	if (configuration->noCache == false) {
		if (configuration->originServerUrl[0][0] == '\0') {
			fprintf(stderr, "you must specify an origin HTTP server for replication/cache with --originserverurl <origin_server_url>\n");
			fprintf(stderr, "if you don't want to use a cache system, use --nocache\n");
			systemLog->sysLog(CRITICAL, "you must specify an origin HTTP server for replication/cache\n");
			systemLog->sysLog(CRITICAL, "if you don't want to use a cache system, use --nocache/-n\n");
			exit(EXIT_FAILURE);
		}
		if (configuration->cacheDirectory[0] == '\0') {
			fprintf(stderr, "you must specify a cache directory path to store files from origin http server url with --cachedir <cache_directory_path>\n");
			fprintf(stderr, "if you don't want to use a cache system, use --nocache\n");
			systemLog->sysLog(CRITICAL, "you must specify a cache directory path to store files from origin http server url with --cachedir <cache_directory_path>\n");
			systemLog->sysLog(CRITICAL, "if you don't want to use a cache system, use --nocache/-n\n");
			exit(EXIT_FAILURE);
		}
	}
	else {
		if (configuration->documentRoot[0] == '\0') {
			fprintf(stderr, "you must specify a default document root for the HTTP server with --documentroot <document_root_path>\n");
			systemLog->sysLog(CRITICAL, "you must specify a default document root for the HTTP server with --documentroot <document_root_path>\n");
			exit(EXIT_FAILURE);
		}
		strncpy(configuration->cacheDirectory, configuration->documentRoot, sizeof(configuration->cacheDirectory)-1);
		configuration->cacheDirectory[sizeof(configuration->cacheDirectory)-1] = '\0';
	}
	if (configuration->shareCatalog == true) {
		if (! configuration->proxyName) {
			fprintf(stderr, "you must specify a proxy name parameter for redirecting requests with --proxyname\n");
			fprintf(stderr, "eg: HTTP address of this host is http://www.foo.com/ so I use --proxyname www.foo.com\n");
			exit(EXIT_FAILURE);
		}
	}

	// Compute Shapping Timeout if present
	if (configuration->shappingTimeout)
		configuration->shappingTimeout = (int)((((float)(configuration->sendBuffer * 8)) / configuration->shappingTimeout));
	

	// Change working directory and Daemonize
	if (configuration->noDaemon == false)
		daemon(1, 0);

	// Set time zone
	tzset();

	// Create the control key hashtable
	hashAlgorithm = new HashAlgorithm(ALGO_PAULHSIEH);
	if (! hashAlgorithm) {
		systemLog->sysLog(CRITICAL, "cannot create an HashTableAlgorithm. Must exit...");
		return -1;
	}
	// Pass the hastable to 0x40000 => sufficient
	keyHashtable = new HashTable(hashAlgorithm, 0x3FFFF);
	if (! keyHashtable) {
		systemLog->sysLog(CRITICAL, "cannot create a HashTable object. Must exit...");
		delete hashAlgorithm;
		return -1;
	}
	// New cache manager object
	cacheManager = new CacheManager(configuration);

	if (configuration->shareCatalog) {
		systemLog->sysLog(INFO, "creating catalog hashtable needed for storing cache objects");
		// Create the control key hashtable
		catalogHashAlgorithm = new HashAlgorithm(ALGO_PAULHSIEH);
		if (! catalogHashAlgorithm) {
			systemLog->sysLog(CRITICAL, "cannot create an HashTableAlgorithm. Must exit...");
			return -1;
		}
	
		// Catalog hashtable
		catalogHashtable = new HashTable(hashAlgorithm, 0x3FFFF);
		if (! catalogHashtable) {
			systemLog->sysLog(CRITICAL, "cannot create a HashTable object. Must exit...");
			delete hashAlgorithm;
			delete keyHashtable;
			return -1;
		}

		systemLog->sysLog(INFO, "catalog hashtable is created successfully");
		systemLog->sysLog(INFO, "creating multicast catalog server and timeout monitoring");

		// Add a Multicast Catalog Server object
		serverList = new List<MonitoredHost *>();
		serverList->setDestroyData(2);
		multicastServerCatalog = new MulticastServerCatalog(configuration, configuration->multicastIp, configuration->multicastCatalogPort, catalogHashtable, serverList);
		if (! multicastServerCatalog) {
			systemLog->sysLog(CRITICAL, "cannot create a MulticastServerCatalog object. Must exit...");
			delete hashAlgorithm;
			delete keyHashtable;
			delete httpServer;
			delete multicastServer;
			delete serverList;
			return -1;
		}
		// Add a timer object for the hashtable catalog
		catalogHashtableTimeout = new CatalogHashtableTimeout(catalogHashtable, configuration->cacheTimeout, cacheManager, multicastServerCatalog);

		systemLog->sysLog(INFO, "multicast catalog server and timeout monitoring created successfully");
		systemLog->sysLog(INFO, "starting multicast catalog server thread");

		// Listening on the wire before trying to get catalog :)
		multicastServerCatalogThreads = new Thread(multicastServerCatalog);
		multicastServerCatalogThreads->createThread(NULL);

		systemLog->sysLog(INFO, "multicast catalog server is running");

		// If sharecatalog option is activated, we must listen the wire 30 seconds and
		// check if another proxy are on the cluster, if this is right, we ask to transfer
		// the complete catalog and add all keys to catalogHashtable
		systemLog->sysLog(INFO, "listening and getting cluster catalog");
		returnCode = listenAndGetCatalog();
		if (returnCode == -1)
			systemLog->sysLog(INFO, "no proxies are running on the cluster, starting with a blank video catalog");
		systemLog->sysLog(INFO, "object catalog is now ready to server");

		systemLog->sysLog(INFO, "load existing disk cache into objects catalog");
		returnCode = loadExistingDiskCache(configuration->cacheDirectory);
		if (returnCode < 0) {
			systemLog->sysLog(ERROR, "cannot load existing disk cache into catalog");
			systemLog->sysLog(ERROR, "exiting...");
			exit(EXIT_FAILURE);
		}
		systemLog->sysLog(INFO, "disk cache is loaded successfully");
	}
	// Add a timer object for the hashtable keys
	keyHashtableTimeout = new KeyHashtableTimeout(keyHashtable, configuration->keyTimeout);
	// New Content Coordinator
	streamContent = new StreamContent(configuration, keyHashtable, keyHashtableTimeout, catalogHashtable, catalogHashtableTimeout, multicastServerCatalog, cacheManager);
	// Create an HTTP server object
	slotMutex = new Mutex();
	httpSlot = new int;
	*httpSlot = 0;
	httpServer = new HttpServer(streamContent, configuration->listenQueue, configuration->readTimeout, configuration->writeTimeout, configuration->shappingTimeout, configuration->burst, httpSlot, slotMutex, configuration->aesKey, configuration->aesKeySize, configuration->aesVHost, configuration->noByteRange, configuration->listeningPort);
	// And a Multicast Server object
        systemLog->sysLog(INFO, "creating Multicast server on source ip address");
        multicastServer = new MulticastServer(configuration->multicastIp, configuration->sourceMulticastIp, configuration->multicastPort, keyHashtable, keyHashtableTimeout);
	
	// Ignore some signals
//	signal(SIGHUP, SIG_IGN);
//	signal(SIGQUIT, SIG_IGN);
//	signal(SIGABRT, SIG_IGN);
//	signal(SIGIOT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
//	signal(SIGALRM, SIG_IGN);
//	signal(SIGTERM, SIG_IGN);
//	signal(SIGXCPU, SIG_IGN);
 //	signal(SIGXFSZ, SIG_IGN);
//	signal(SIGVTALRM, SIG_IGN);
//	signal(SIGPROF, SIG_IGN);
//	signal(SIGUSR1, SIG_IGN);
//	signal(SIGUSR2, SIG_IGN);

	// Listen on the specified socket for HTTP
 	httpServer->listenSocket();

	// Log starting
	systemLog->sysLog(NOTICE, "starting the streaming daemon");

	// Set some options on http server kqueue/kevent model
	//httpServer->setAcceptFilterHttp();
	httpServer->setSendBuffer(configuration->sendBuffer);
	httpServer->setRecvBuffer(configuration->receiveBuffer);
	httpServer->setNonBlocking();
 	//httpServer->setMinimumToSend(configuration->sendBuffer / 2);
 	httpServer->setMinimumToSend(configuration->sendBuffer);

	// Change uid / gid
	returnCode = seteuid(configuration->userId);
	if (returnCode < 0) {
		fprintf(stderr, "cannot seteuid with id %d: %s\n", configuration->userId, strerror(errno));
		systemLog->sysLog(CRITICAL, "cannot seteuid with id %d: %s", configuration->userId, strerror(errno));
		exit(EXIT_FAILURE);
	}
	returnCode = setegid(configuration->groupId);
	if (returnCode < 0) {
		fprintf(stderr, "cannot setegid with id %d: %s\n", configuration->groupId, strerror(errno));
		systemLog->sysLog(CRITICAL, "cannot setegid with id %d: %s", configuration->groupId, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// The infinite run loop treat all events on the socket
	httpClientConnection = new HttpClientConnection(configuration);
	httpServer->createContext("*", httpClientConnection);
	httpServer->setKqueueEvents();

	httpServerWorkerThreads = new Thread(httpServer);
	for (i = 0; i < configuration->workerNumber; i++)
		httpServerWorkerThreads->createThread(NULL);

	if (configuration->noKeyCheck == false) {
		keyHashtableTimeoutThread = new Thread(keyHashtableTimeout);
		keyHashtableTimeoutThread->createThread(NULL);
	}

	if (configuration->shareCatalog == true) {
		catalogHashtableTimeoutThread = new Thread(catalogHashtableTimeout);
		catalogHashtableTimeoutThread->createThread(NULL);
	}

	multicastServerThreads = new Thread(multicastServer);
	multicastServerThreads->createThread(NULL);
	
	if (configuration->administrationServerEnable == true) {
		administrationServer = new AdministrationServer(configuration, catalogHashtable);
		administrationServerThread = new Thread(administrationServer);
		administrationServerThread->createThread(NULL);
	}

	pause();

	pthread_exit(NULL);
}

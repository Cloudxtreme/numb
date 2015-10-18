//
// C++ Interface: httpsession
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/event.h>
#include <sys/time.h>

#include "../toolkit/server.h"
#include "../toolkit/httpexchange.h"
#include "../toolkit/mutex.h"
#include "../src/multicastdata.h"

#include <string>
#include <list>

extern int numberOfConnections;

#define MAXHTTPREQUESTSIZE 4096

typedef struct ByteRange {
	int32_t start;
	int32_t end;
} ByteRange_t;

/**
	@author  <spe@>
*/
class HttpSession {
public:
	bool initialized;

	// Mutex protection
	Mutex *mutex;

	// Actual Kqueue Event
	struct kevent kEvent;

	// Communication Exchange (descriptors etc...)
	HttpExchange *httpExchange;

	// HTTP Request vars
	socketMsg *smsg;
	char videoName[MAXMSGSIZE];
	int httpCode;
	char httpFullRequest[MAXHTTPREQUESTSIZE + 1];
	ssize_t requestSize;
	// GET, HEAD, POST etc... and parsed http request
	char httpRequestType;
	char httpRequest[1024];
	char virtualHost[1024];
	char referer[1024];
	char userAgent[1024];
	char ipSource[16];
	ByteRange_t byteRange;
  std::list<std::string> requestArgs;

	// HTTP Answer vars
	char httpHeader[MAXMSGSIZE];
	char *preBuffer;
	unsigned int preBufferSize;
	unsigned int preBufferOffset;
	bool preBufferSent;

	// Request file informations
	int fileDescriptor;
	int fileSize;
	struct stat sourceFileStat;

	// Streaming vars
	int seekPosition;
	double seekSeconds;
	int mp4Position;

	// Some booleans, state of the connection etc...
	bool keepAliveConnection;
	bool endOfRequest;
	bool endOfAnswer;
	bool noDataToSend;

	char *videoChunk;
	int videoChunkSize;
	bool localFileCreated;
	char *videoNameFilePath;
	char *videoNameNfsFilePath;
	bool HTTPHeaderInitialized;
	bool HTTPHeaderSent;
	bool sourceFileNameOpened;
	bool downloadMimeType;
	int shapping;
	int shappingTimeout;
	bool shappingAuto;
	
	Server *server;
	int *numberOfCopy;

	int bytesToRead;
	char *bufferToSend;

	bool mmapEnabled;
	off_t fileOffset;

	struct sockaddr_in sourceAddress;

	int streamingTimestamp;

#ifndef SENDFILE
	// Offset of the current chunk
	ssize_t chunkOffset;
	char *chunkBuffer;
	int chunkBytesLeft;
	int chunkSize;
#endif

	// From the PHP frontend (transferred with multicast)
	struct MulticastData *multicastData;

	// If there is a 301 Redirect, we must save the redirectUrl here
	char *redirectUrl;

	bool mustCloseConnection;
	char burst;

	char mimeType;

	HttpSession();
	HttpSession(HttpSession *);
	~HttpSession();

	void init(int, struct sockaddr_in *);
	void init(void);
	void reinit(void);
	int create(void);
	void destroy(bool);
	void setBurst(char);
};

#endif

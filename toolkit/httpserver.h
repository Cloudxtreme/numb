//
// C++ Interface: httpserver
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#define MAXHTTPREQUESTSIZE 4096

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <openssl/bio.h>

#include "../toolkit/server.h"
#include "../toolkit/httpsession.h"
#include "../toolkit/httpcontext.h"
#include "../toolkit/httphandler.h"
#include "../toolkit/httpcontent.h"
#include "../toolkit/mutex.h"

/**
	@author  <spe@>
*/
class HttpServer : public Server {
private:
	int readTimeout;
	int writeTimeout;
	int shapping;
	char burst;
	HttpContent *httpContent;
	bool aesEnabled;
	char *aesKey;
	char *aesVHost;
	char *noByteRange;

protected:
	int maxConnectionsAuthorized;
	int kQueue;
	List<HttpContext *> httpContextList;
	HttpSession *httpSessionsIndex[65536];

public:
	HttpServer(HttpContent *, int, int, int, int, char, int *, Mutex *, char *, int, char *, char *, unsigned short);
	virtual ~HttpServer();

	int setKqueueEvents(void);
	int sendChunk(HttpSession *, char *, int);
	int sendChunk(HttpSession *);
	void initHeader(HttpSession *, const char *);
	int sendHeader(HttpSession *);
	int readRequest(HttpSession *);
	int decodeBase64(char *, char **, int *);
	void initAES(EVP_CIPHER_CTX *, unsigned char *, unsigned char *);
	char *decryptAES(EVP_CIPHER_CTX *, char *, int);
	void freeAES(EVP_CIPHER_CTX *);
	int verifyQuery(HttpSession *);
	void combinedLog(HttpSession *, char *, int);
	int acceptConnection(void);
	int writeHttpAnswer(HttpSession *);
	int endConnection(HttpSession *);
	int writeEvent(HttpSession *);
	int readEvent(HttpSession *);
	int run(void *);
	virtual void start(void *arguments) { run(arguments); delete this; return; };
	int startServer(void);
	int stopServer(void);
	HttpContext *createContext(const char *, HttpHandler *);
	int removeContext(char *);
	int getAddress(void);
	void setShapping(int _shapping) { shapping = _shapping; };
	HttpContent *getContent(void);
};

#endif

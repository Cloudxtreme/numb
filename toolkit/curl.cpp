//
// C++ Implementation: curl
//
// Description: 
//
//
// Author:  <spe@>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <errno.h>

#include "curl.h"

Curl::Curl()
{
	CURLcode curlReturnCode;

	curlInitialized = false;
	curlMutex = new Mutex();
	if (! curlMutex) {
		systemLog->sysLog(CRITICAL, "cannot crete a Mutex object: %s", strerror(errno));
		return;
	}

	curlMutex->lockMutex();
	// This function is not Thread safe, so we must add a Mutex to protect it in multithreaded environments
	curlReturnCode = curl_global_init(CURL_GLOBAL_ALL);
	if (curlReturnCode < 0) {
		systemLog->sysLog(ERROR, "can't initialize the CURL library (curl_global_init problem): %s", curl_easy_strerror(curlReturnCode));
		curlMutex->unlockMutex();
		return;
	}
	curlMutex->unlockMutex();

	curlSessionList = new List<CurlSession *>();
	if (! curlSessionList) {
		systemLog->sysLog(ERROR, "CurlSession * List could not be created. Cannot initalize Curl object");
		return;
	}
	curlSessionList->setDestroyData(DATA_DESTROY_WITH_DELETE);

	curlSessionNumber = 0;

	curlInitialized = true;

	return;
}

Curl::~Curl()
{
	CurlSession *curlSession;

	curlMutex->lockMutex();
	if (curlSessionList) {
		do {
			curlSession = curlSessionList->getFirstElement();
			if (curlSession && (curlSession->session)) {
				curl_easy_cleanup(curlSession->session);
				curlSessionList->removeFirst();;
			}
		}
		while (curlSession);
	}
	curlMutex->unlockMutex();

	return;
}

CurlSession *Curl::createSession(void) {
	CurlSession *curlSession;
	ListElt<CurlSession *> *curlSessionElement;

	curlSession = new CurlSession();
	curlSession->session = curl_easy_init();
	if (! curlSession->session) {
		systemLog->sysLog(ERROR, "cannot create a curl session: %s", strerror(errno));
		return NULL;
	}
	curlSessionElement = curlSessionList->addElement(curlSession);
	if (! curlSessionElement) {
		systemLog->sysLog(ERROR, "cannot add a curlSession element to the curlSessionList. Something is terribly wrong");
		curl_easy_cleanup(curlSession->session);
		return NULL;
	}
	curlSessionNumber++;

	return curlSession;
}

int Curl::deleteSession(CurlSession *curlSession) {
	if (! curlSession || (! curlSession->session)) {
		systemLog->sysLog(NOTICE, "session number is NULL, can't cleanup session, sorry");
		return -1;
	}
	curl_easy_cleanup(curlSession->session);

	return 0;
}

int Curl::setHeaderOutput(CurlSession *curlSession) {
	CURLcode curlReturnCode;

	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_HEADER, 1);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set CURLOPT_HEADER on the curlSession: %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}

	return 0;
}

int Curl::setRequestHeader(CurlSession *curlSession, struct curl_slist *slist) {
	CURLcode curlReturnCode;

	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_HTTPHEADER, slist);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set CURLOPT_HTTPHEADER on the curlSession: %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}

	return 0;
}

int Curl::fetchHttpUrl(CurlSession *curlSession, File *file, char *URL) {
	CURLcode curlReturnCode;

	if ((! URL) || (! file)) {
		if (! URL) {
			systemLog->sysLog(ERROR, "URL parameter is NULL, cannot fecth a NULL URL");
			return -1;
		}
		if (! file) {
			systemLog->sysLog(ERROR, "file parameter is NULL, cannot fecth in a NULL file");
			return -1;
		}
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_WRITEDATA, URL);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot create options for the CURL library initialization (curl_easy_setopt): %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}
	curlReturnCode = curl_easy_perform(curlSession->session);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot perform the session (URL access) '%s': %s", URL, curl_easy_strerror(curlReturnCode));
		return -1;
	}

	return 0;
}

int Curl::fetchHttpUrl(CurlSession *curlSession, char *fileName, char *URL) {
	CURLcode curlReturnCode;
	File *streamFile;

	if ((! URL) || (! fileName)) {
		if (! URL) {
			systemLog->sysLog(ERROR, "URL parameter is NULL, cannot fecth a NULL URL");
			return -1;
		}
		if (! fileName) {
			systemLog->sysLog(ERROR, "fileName parameter is NULL, cannot fecth in a NULL file");
			return -1;
		}
	}
	streamFile = new File(fileName, "w+", 0);
	if (! streamFile) {
		systemLog->sysLog(ERROR, "cannot create a file object with '%s', can't get the URL '%s'", fileName, URL);
		return -1;
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_WRITEDATA, streamFile->getStreamDescriptor());
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot create CURLOPT_WRITEDATA with '%s' for the initialization (curl_easy_setopt): %s", fileName, curl_easy_strerror(curlReturnCode));
		return -1;
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_URL, URL);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set the CURLOPT_URL option with '%s': %s", URL, curl_easy_strerror(curlReturnCode));
		return -1;
	}
	curlReturnCode = curl_easy_perform(curlSession->session);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot perform the session (URL access) '%s': %s", URL, curl_easy_strerror(curlReturnCode));
		return -1;
	}

	return 0;
}

int Curl::fetchHttpUrlWithCallback(CurlSession *curlSession, void *callbackFunction, void *arguments, void *callbackHeader, void *headerArguments, char *URL) {
	CURLcode curlReturnCode;
	long on = 1;

	if (! curlSession) {
		systemLog->sysLog(ERROR, "curlSession is NULL, cannot fetch with a NULL session");
		return -1;
	}
	if (! URL) {
		systemLog->sysLog(ERROR, "URL parameter is NULL, cannot fecth a NULL URL");
		return -1;
	}
	if (! callbackFunction) {
		systemLog->sysLog(ERROR, "callbackFunction parameter is NULL, cannot fecth in a NULL callback function");
		return -1;
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_WRITEFUNCTION, callbackFunction);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set CURLOPT_WRITEFUNCTION (curl_easy_setopt): %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_WRITEDATA, arguments);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set CURLOPT_WRITEDATA (curl_easy_setopt): %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}
	if (callbackHeader) {
		curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_HEADERFUNCTION, callbackHeader);
		if (curlReturnCode != CURLE_OK) {
			systemLog->sysLog(ERROR, "cannot set CURLOPT_HEADERFUNCTION (curl_easy_setopt): %s", curl_easy_strerror(curlReturnCode));
			return -1;
		}
		curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_WRITEHEADER, headerArguments);
		if (curlReturnCode != CURLE_OK) {
			systemLog->sysLog(ERROR, "cannot set CURLOPT_WRITEHEADER (curl_easy_setopt): %s", curl_easy_strerror(curlReturnCode));
			return -1;
		}
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_URL, URL);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set the CURLOPT_URL option with '%s': %s", URL, curl_easy_strerror(curlReturnCode));
		return -1;
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_CONNECTTIMEOUT, 10);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set the CURLOPT_CONNECTTIMEOUT option with 10 seconds: %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_BUFFERSIZE, 131072);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set the CURLOPT_BUFFERSIZE option with 131072 bytes: %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_HTTP_TRANSFER_DECODING, 0);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set the CURLOPT_BUFFERSIZE option with 131072 bytes: %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}
	curlReturnCode = curl_easy_setopt(curlSession->session, CURLOPT_FILETIME, on);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot set the CURLOPT_FILETIME to true: %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}
	curlReturnCode = curl_easy_perform(curlSession->session);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot perform the session (URL access) '%s': %s", URL, curl_easy_strerror(curlReturnCode));
		return -1;
	}

	return 0;
}

int Curl::getHttpCode(CurlSession *curlSession) {
	CURLcode curlReturnCode;
	long responseCode;

	curlReturnCode = curl_easy_getinfo(curlSession->session, CURLINFO_RESPONSE_CODE, &responseCode);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot get CURLINFO_RESPONSE_CODE: %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}

	return (int)responseCode;
}

int Curl::getDownloadSize(CurlSession *curlSession) {
	CURLcode curlReturnCode;
	double downloadSize;

	curlReturnCode = curl_easy_getinfo(curlSession->session, CURLINFO_SIZE_DOWNLOAD, &downloadSize);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot get CURLINFO_SIZE_DOWNLOAD: %s", curl_easy_strerror(curlReturnCode));
		return -1;
	}

	return (int)downloadSize;
}

int Curl::getContentLength(CurlSession *curlSession) {
	CURLcode curlReturnCode;
	double contentLength;

	curlReturnCode = curl_easy_getinfo(curlSession->session, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot get CURLINFO_CONTENT_LENGTH_DOWNLOAD: %s\n", curl_easy_strerror(curlReturnCode));
		return -1;
	}

	return (int)contentLength;
}

long Curl::getLastModified(CurlSession *curlSession) {
	CURLcode curlReturnCode;
	long time;

	curlReturnCode = curl_easy_getinfo(curlSession->session, CURLINFO_FILETIME, &time);
	if (curlReturnCode != CURLE_OK) {
		systemLog->sysLog(ERROR, "cannot get CURLINFO_FILETIME: %s\n", curl_easy_strerror(curlReturnCode));
		return -1;
	}

	return time;
}


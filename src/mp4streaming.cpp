#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "mp4streaming.h"

#include "../toolkit/log.h"

Mp4Streaming::Mp4Streaming(HttpSession *_httpSession) {
	httpSession = _httpSession;

	return;
}

Mp4Streaming::~Mp4Streaming() {
	return;
}

int Mp4Streaming::seek(void) {
	uint64_t mdat_offset;
	uint64_t mdat_size;
	int returnCode;
	char **preBufferPtr;

	if (httpSession->videoNameFilePath == NULL)
		return -1;

	preBufferPtr = &httpSession->preBuffer;
	returnCode = mp4_split(httpSession->videoNameFilePath, httpSession->fileSize, httpSession->seekSeconds, 0, (void **)preBufferPtr, &httpSession->preBufferSize, &mdat_offset, &mdat_size, 1);

	if (! returnCode) {
		systemLog->sysLog(ERROR, "cannot seek mp4 file '%s': %s", httpSession->videoNameFilePath, strerror(errno));
		return -1;
	}

	returnCode = lseek(httpSession->httpExchange->inputDescriptor, mdat_offset, SEEK_SET);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot lseek on descriptor %d: %s", httpSession->fileDescriptor, strerror(errno));
		return -1;
	}

	httpSession->mp4Position = mdat_offset;
	httpSession->fileSize = mdat_size + httpSession->preBufferSize;;

#ifdef DEBUGOUTPUT
	systemLog->sysLog(DEBUG, "httpSession->fileSize = %d", httpSession->fileSize);
#endif

	return 0;
}

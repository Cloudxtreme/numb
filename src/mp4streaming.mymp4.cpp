#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "../src/mp4streaming.h"
#include "../toolkit/mp4reader.h"

#include "../toolkit/log.h"

Mp4Streaming::Mp4Streaming(HttpSession *_httpSession) {
	httpSession = _httpSession;
	mp4Header = NULL;

	return;
}

Mp4Streaming::~Mp4Streaming() {
	if (mp4Header)
		free(mp4Header);
	return;
}

int Mp4Streaming::sendHeader(void) {
	int returnCode;

	fprintf(stderr, "mp4Header is %p\n", mp4Header);
	fprintf(stderr, "mp4HeaderSize is %u\n", mp4HeaderSize);
	if (! mp4Header) {
		systemLog->sysLog(ERROR, "buffer is NULL, cannot send a mp4 header, call seek() first");
		return -1;
	}
	returnCode = send(httpSession->httpExchange->outputDescriptor, mp4Header, mp4HeaderSize, 0);
	if (returnCode < 0) {
		systemLog->sysLog(ERROR, "cannot send mp4 header: %s", strerror(errno));
		free(mp4Header);
		return -1;
	}
	httpSession->fileOffset = ntohl(metadata.mdat.offset) + 8;
	fprintf(stderr, "fileOffset is %u\n", httpSession->fileOffset);

	free(mp4Header);

	return 0;
}

int Mp4Streaming::readMetadata(void) {
	File *metadataFile;
	char *metadataFileName;
	size_t metadataLenght = strlen(httpSession->videoNameFilePath) + strlen(".metadata") + 1;
	size_t bytesRead;

	metadataFileName = new char[metadataLenght];
	snprintf(metadataFileName, metadataLenght, "%s.metadata", httpSession->videoNameFilePath);
	metadataFile = new File(metadataFileName, "r", 1);
	if (! metadataFile) {
		systemLog->sysLog(WARNING, "%s not present, cannot seek", httpSession->videoNameFilePath);
		delete metadataFileName;
		return -1;
	}
	delete metadataFileName;
	bytesRead = metadataFile->readStreamFile((char *)&metadata, sizeof(metadata));
	if (bytesRead < 0) {
		systemLog->sysLog(WARNING, "cannot read meta datas, cannot seek %s: %s", httpSession->videoNameFilePath, strerror(errno));
		delete metadataFile;
		return -1;
	}
	if (bytesRead != sizeof(metadata)) {
		systemLog->sysLog(WARNING, "cannot read meta datas, need %d bytes, only have %d bytes", sizeof(metadata), bytesRead);
		delete metadataFile;
		return -1;
	}

	delete metadataFile;

	return 0;
}

uint32_t *Mp4Streaming::getPtrByOffset(uint32_t offset) {
	if (offset >= mp4HeaderSize) {
		systemLog->sysLog(ERROR, "metadata.tkhd.offset (%d) is greater than mp4Headersize (%d), segfault may occur, seeking disabled", offset, mp4HeaderSize);
		free(mp4Header);
		lseek(httpSession->httpExchange->inputDescriptor, 0, SEEK_SET);
		return NULL;
	}
	return (uint32_t *)(mp4Header + offset);
}

void Mp4Streaming::copy(char *start1, char *start2, char *end) {
	fprintf(stderr, "copying %d bytes\n", end - start2);
	while (start2 != end) {
		*start1 = *start2;
		start1++;
		start2++;
	}

	return;
}

void Mp4Streaming::shiftTables(void) {
	int i;

	for (i = 0; i < copyTableCount; i++)
		copy(copyTable[i].start1, copyTable[i].start2, &mp4Header[mp4HeaderSize - 1]);

	mp4HeaderSize -= sizeof(uint32_t) * 3;
}

int Mp4Streaming::createMp4Header(void) {
	size_t bytesRead;
	uint32_t *uint32Ptr;
	uint32_t *table;
	cttsTable_t *cttsTable;
	uint32_t teta, teta2, teta3;
	uint32_t sum;
	uint32_t i, j;
	uint32_t delta;
	uint32_t startOffsetVideo, offsetVideo;
	uint32_t mdhdDuration;
	char *copyPtr;
	uint32_t *moovSize;

	copyTableCount = 0;
	mp4HeaderSize = ntohl(metadata.mdat.offset) + 8;
	fprintf(stderr, "mp4HeaderSize is %u\n", mp4HeaderSize);
	mp4Header = (char *)malloc(mp4HeaderSize);
	if (! mp4Header) {
		systemLog->sysLog(CRITICAL, "cannot create moovBuffer object: %s", strerror(errno));
		return -1;
	}
	bytesRead = read(httpSession->httpExchange->inputDescriptor, mp4Header, mp4HeaderSize);
	if (bytesRead != mp4HeaderSize) {
		systemLog->sysLog(WARNING, "cannot read %d bytes (only %d bytes read) for the moov section on %s: %s", mp4HeaderSize, bytesRead, httpSession->videoNameFilePath, strerror(errno));
		free(mp4Header);
		lseek(httpSession->httpExchange->inputDescriptor, 0, SEEK_SET);
		return -1;
	}

	moovSize = getPtrByOffset(ntohl(metadata.moov.offset));
	if (! moovSize)
		return -1;
	teta = httpSession->seekSeconds * ntohl(metadata.mvhd.timescale);

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[mvhd] found @ %u", ntohl(metadata.mvhd.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.mvhd.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[mvhd] old duration is %d", ntohl(*uint32Ptr));
#endif

	*uint32Ptr = htonl(ntohl(*uint32Ptr) - teta);

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "teta is %u", teta);
	systemLog->sysLog(DEBUG, "[mvhd] duration is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[tkhd] found @ %u", ntohl(metadata.tkhd.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.tkhd.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[tkhd] old duration is %u", ntohl(*uint32Ptr));
#endif

	*uint32Ptr = htonl(ntohl(*uint32Ptr) - teta);

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[tkhd] duration is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[mdhd] found @ %u", ntohl(metadata.mdhd.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.mdhd.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[mdhd] old duration is %u", ntohl(*uint32Ptr));
#endif

	teta2 = httpSession->seekSeconds * ntohl(metadata.mdhd.timescale);
	*uint32Ptr = htonl(ntohl(*uint32Ptr) - teta2);

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[mdhd] duration is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[stts] found @ %u", ntohl(metadata.stts.offset));
#endif
	uint32Ptr = getPtrByOffset(ntohl(metadata.stts.offset));
	if (! uint32Ptr)
		return -1;
	uint32Ptr = uint32Ptr + 1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stts] old sampleCount is %u", ntohl(*uint32Ptr));
#endif

	*uint32Ptr = htonl(ntohl(*uint32Ptr) - teta2);

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stts] sampleCount is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[stss] found @ %u", ntohl(metadata.stss.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.stss.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stss] old entryCount is %u", ntohl(*uint32Ptr));
#endif

	table = uint32Ptr + 1;
	i = 0;
	while ((ntohl(table[i]) < teta2) && (i < ntohl(*uint32Ptr)))
		i++;

	if (i > 1) {
		j = i;
		while (j < ntohl(*uint32Ptr)) {
			table[j - i] = htonl(ntohl(table[j]) - teta2);
			j++;
		}
	}
	copyTable[copyTableCount].start1 = (char *)&table[ntohl(*uint32Ptr) - i];
 	copyTable[copyTableCount].start2 = (char *)&table[ntohl(*uint32Ptr)];
	copyTableCount++;
	*uint32Ptr = htonl(ntohl(*uint32Ptr) - i);
	uint32Ptr -= 2;
	*uint32Ptr = htonl(ntohl(*uint32Ptr) - (i * sizeof(*table)));
	*moovSize = htonl(ntohl(*moovSize) - (i * sizeof(*table)));

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[moov] new size %u bytes", ntohl(*moovSize));
	systemLog->sysLog(DEBUG, "[stss] entryCount is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[stss] table id updated");
	systemLog->sysLog(DEBUG, "[stsz] found @ %u", ntohl(metadata.stsz.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.stsz.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stsz] old sampleCount is %u", ntohl(*uint32Ptr));
#endif

	table = uint32Ptr + 1;
	*uint32Ptr = htonl(ntohl(*uint32Ptr) - teta2);
	if (teta2 > 1)
		bcopy(&table[teta2], &table[0], ntohl(*uint32Ptr) * sizeof(*table));

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stsz] sampleCount is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[stsz] table id updated");
	systemLog->sysLog(DEBUG, "[stco] found @ %u", ntohl(metadata.stco.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.stco.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stco] old entryCount is %u", ntohl(*uint32Ptr));
#endif

	table = uint32Ptr + 1;

	fprintf(stderr, "fileSize is %u\n", httpSession->fileSize);
	fprintf(stderr, "extract %u\n", ntohl(table[teta2]));
	fprintf(stderr, "seeking to table[%u] = %u\n", teta2, ntohl(table[teta2]));
	lseek(httpSession->httpExchange->inputDescriptor, ntohl(table[teta2]), SEEK_SET);

	if (teta2 > 1) {
		offsetVideo = ntohl(table[teta2]);
		startOffsetVideo = ntohl(table[0]);
		delta = ntohl(table[teta2]) - ntohl(table[0]);
		httpSession->fileSize -= delta;
		j = teta2;
		while (j < ntohl(*uint32Ptr)) {
			table[j - teta2] = htonl(ntohl(table[j]) - delta);
			j++;
		}
	}
	*uint32Ptr = htonl(ntohl(*uint32Ptr) - teta2);


#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stco] entryCount is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[stco] table id updated");
	systemLog->sysLog(DEBUG, "[ctts] found @ %u", ntohl(metadata.ctts.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.ctts.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[ctts] old entryCount is %u", ntohl(*uint32Ptr));
#endif

	cttsTable = (cttsTable_t *)uint32Ptr + 1;
	i = 0;
	sum = 0;
	while ((sum < teta2) && (i < ntohl(*uint32Ptr))) {
		sum += ntohl(cttsTable[i].sampleCount);
		i++;
	}
	*uint32Ptr = htonl(ntohl(*uint32Ptr) - i);
	if (i > 1)
		bcopy(&cttsTable[i - 1], &cttsTable[0], ntohl(*uint32Ptr) * sizeof(*cttsTable));

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[ctts] entryCount is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[ctts] sum is %u, teta2 is %u, copy cttsTable[%u] -> cttsTable[0] size %u", sum, teta2, i-1, ntohl(*uint32Ptr) * sizeof(*cttsTable));
	systemLog->sysLog(DEBUG, "[mdhd Audio] found @ %u", ntohl(metadata.mdhdAudio.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.mdhdAudio.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[mdhd Audio] old duration is %u", ntohl(*uint32Ptr));
#endif

	teta3 = (httpSession->seekSeconds * ntohl(metadata.mdhdAudio.timescale)) / 1024;
	mdhdDuration = ntohl(*uint32Ptr) - (teta3 * 1024);
	*uint32Ptr = htonl(mdhdDuration);

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[mdhd Audio] duration is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[tkhd Audio] found @ %u", ntohl(metadata.tkhdAudio.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.tkhdAudio.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[tkhd Audio] old duration is %u", ntohl(*uint32Ptr));
#endif

	*uint32Ptr = htonl(((float)mdhdDuration / ntohl(metadata.mdhdAudio.timescale)) * ntohl(metadata.mvhd.timescale));

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[tkhd Audio] teta3 is %u", teta3);
	systemLog->sysLog(DEBUG, "[tkhd Audio] duration is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[stts Audio] found @ %u", ntohl(metadata.sttsAudio.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.sttsAudio.offset));
	if (! uint32Ptr)
		return -1;
	uint32Ptr = uint32Ptr + 1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stts Audio] old sampleCount is %u", ntohl(*uint32Ptr));
#endif

	*uint32Ptr = htonl(ntohl(*uint32Ptr) - teta3);

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stts Audio] sampleCount is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[stsz Audio] found @ %u", ntohl(metadata.stszAudio.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.stszAudio.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stsz Audio] old sampleCount is %u", ntohl(*uint32Ptr));
#endif

	*uint32Ptr = htonl(ntohl(*uint32Ptr) - teta3);
	table = uint32Ptr + 1;
	if (teta3 > 0)
		bcopy(&table[teta3], &table[0], ntohl(*uint32Ptr) * sizeof(*table));

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stsz Audio] table[%u] <- table[%u] (%u bytes)", teta3, 0, ntohl(*uint32Ptr) * sizeof(*table));
	systemLog->sysLog(DEBUG, "[stsz Audio] sampleCount is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[stsz Audio] table id is updated");
	systemLog->sysLog(DEBUG, "[stco Audio] found @ %u", ntohl(metadata.stcoAudio.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.stcoAudio.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stco Audio] old sampleCount is %u", ntohl(*uint32Ptr));
#endif

	table = uint32Ptr + 1;

	if (teta3 > 0) {
		j = teta3;
		while (j < ntohl(*uint32Ptr)) {
			table[j - teta3] = htonl(ntohl(table[j]) - delta);
			j++;
		}
	}
	*uint32Ptr = htonl(ntohl(*uint32Ptr) - teta3);


#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[stco Audio] last table[%u] <- table[%u]", j - 1 - teta3, j - 1);
	systemLog->sysLog(DEBUG, "[stco Audio] sampleCount is %u", ntohl(*uint32Ptr));
	systemLog->sysLog(DEBUG, "[mdat] found @ %u", ntohl(metadata.mdat.offset));
#endif

	uint32Ptr = getPtrByOffset(ntohl(metadata.mdat.offset));
	if (! uint32Ptr)
		return -1;

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[mdat] old size is %u bytes", ntohl(*uint32Ptr));
#endif

	*uint32Ptr = htonl(ntohl(*uint32Ptr) - delta);

#ifdef DEBUGMP4STREAMING
	systemLog->sysLog(DEBUG, "[mdat] size is %u bytes", ntohl(*uint32Ptr));
#endif

	//shiftTables();

	return 0;
}

int Mp4Streaming::seek(void) {
	FILE* infile;
	uint64_t mdat_offset;
	uint64_t mdat_size;
	int returnCode;
	void **mp4HeaderPtr;
	char *chrPtr;
	uint32_t ftypSize, minorVersion, freeSize, mdatSize;

	mp4Reader = new Mp4Reader(httpSession->videoNameFilePath);
	if (! mp4Reader)
		return -1;
	returnCode = createMp4Header();
	if (returnCode < 0)
		return -1;

	return 0;
}


//
// C++ Implementation: mp4reader
//
// Description: 
//
//
// Author:  <spe@>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "mp4reader.h"

#include <netinet/in.h>
#include <fcntl.h>

const char *singleBoxNames[] = { "ftyp", "pdin", "moov", "mvhd", "trak", "tkhd", "mdia", "mdhd", "minf", "stbl", "stss", "ctts", "stsz", "stco", "mdat", "free", "stts", NULL };
const char *containerNames[] = { "moov", "trak", "edts", "mdia", "minf", "dinf", "stbl", "mvex", "moof", "traf", "mfra", "skip", "meta", "ipro", "sinf", NULL };
const char *moovBoxNames[] = { "mvhd", "tref", "elst", "mdhd", "hdlr", "vmhd", "smhd", "hmhd", "nmhd", "dref", "stsd", "stts", "ctts", "stsc", "stsz", "stz2", "stco", "co64", "stss", "stsh", "padb", "stdp", "sdtp", "sbgp", "sgpd", "subs", "mehd", "trex", "ipmc", NULL };

Mp4Reader::Mp4Reader(char *fileName) {
	mp4File = new File(fileName, "r", true);
	if (! mp4File) {
		systemLog->sysLog(ERROR, "cannot open file '%s': %s", fileName, strerror(errno));
		return;
	}
	sb = mp4File->getStat();
	if (! sb) {
		systemLog->sysLog(ERROR, "cannot get stat on file '%s': %s", fileName, strerror(errno));
		mp4File->closeFile();
		return;
	}
	fileOffset = 0;
	offsetCount = 0;
	memset(offsets, 0, sizeof(offsets));
	
	return;
}

Mp4Reader::~Mp4Reader() {
	if (mp4File)
		delete mp4File;

	return;
}

int Mp4Reader::parseBox(char *boxName, uint32_t *boxSize) {
	size_t bytesRead;

	if (! boxName)
		return EINVAL;
	bytesRead = mp4File->readStreamFile((char *)boxSize, sizeof(*boxSize));
	if (! bytesRead)
		return EINVAL;
	if (bytesRead != sizeof(*boxSize))
		return EIO;
	fileOffset += bytesRead;
	*boxSize = ntohl(*boxSize);
	if (*boxSize + fileOffset > sb->st_size) {
		systemLog->sysLog(ERROR, "invalid atom size (greater than the file size): %d bytes > %d bytes", *boxSize, sb->st_size);
		return EINVAL;
	}
	bytesRead = mp4File->readStreamFile(boxName, MP4READER_MAXBOXNAMESIZE);
	if (bytesRead != MP4READER_MAXBOXNAMESIZE)
		return EIO;
	boxName[MP4READER_MAXBOXNAMESIZE] = '\0';

	fileOffset += bytesRead;

	return 0;
}

int Mp4Reader::isContainer(char *boxName) {
	int i;

	if (strlen(boxName) != 4) {
		systemLog->sysLog(ERROR, "invalid size (%d bytes) for boxName '%s', boxName must have exactly 4 bytes terminated by NULL", boxName, strlen(boxName));
		return -1;
	}
	i = 0;
	while (containerNames[i] != NULL) {
		if (! strcmp(containerNames[i], boxName)) {
#ifdef DEBUGMP4READER
			systemLog->sysLog(DEBUG, "boxName '%c%c%c%c' is a box container", boxName[0], boxName[1], boxName[2], boxName[3]);
#endif
			return i;
		}
		i++;
	}

	return -1;
}

int Mp4Reader::hashMoovBoxName(char *boxName) {
	int i;

	i = 0;
	while (moovBoxNames[i] != NULL) {
		if (! strcmp(moovBoxNames[i], boxName))
			return i;
		i++;
	}

	return -1;
}

int Mp4Reader::hashSingleBoxName(char *boxName) {
	int i;

	i = 0;
	while (singleBoxNames[i] != NULL) {
		if (! strcmp(singleBoxNames[i], boxName))
			return i;
		i++;
	}

	return -1;
}

mvhd_t *Mp4Reader::parseMvhd(uint32_t boxSize) {
	ssize_t bytesRead;
	int bytesToRead;
	uint32_t flags;
	mvhd_t *mvhd;

	mvhd = new mvhd_t;
	if (! mvhd) {
		systemLog->sysLog(CRITICAL, "cannot allocate mvhd object: %s", strerror(errno));
		return NULL;
	}
	bytesToRead = sizeof(*mvhd);
	bytesRead = mp4File->readStreamFile((char *)mvhd, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read majorBrand correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	flags = (mvhd->flags[0] << 16) || (mvhd->flags[1] << 8) || mvhd->flags[2];
	flags = ntohl(flags);
	mvhd->creationTime = ntohl(mvhd->creationTime);
	mvhd->modificationTime = ntohl(mvhd->modificationTime);
	mvhd->timeScale = ntohl(mvhd->timeScale);
	mvhd->duration = ntohl(mvhd->duration);
	mvhd->rate = ntohl(mvhd->rate);
	mvhd->volume = ntohs(mvhd->volume);
	mvhd->nextTrackId = ntohl(mvhd->nextTrackId);
#ifdef DEBUGMP4READER
	systemLog->sysLog(DEBUG, "[mvhd] version = %u", mvhd->version);
	systemLog->sysLog(DEBUG, "[mvhd] flags = %u", flags);
	systemLog->sysLog(DEBUG, "[mvhd] creationTime = %u", mvhd->creationTime);
	systemLog->sysLog(DEBUG, "[mvhd] modificationTime = %u", mvhd->modificationTime);
	systemLog->sysLog(DEBUG, "[mvhd] timeScale = %u", mvhd->timeScale);
	systemLog->sysLog(DEBUG, "[mvhd] duration = %u", mvhd->duration);
	systemLog->sysLog(DEBUG, "[mvhd] rate = %u", mvhd->rate);
 	systemLog->sysLog(DEBUG, "[mvhd] volume = %u", mvhd->volume);
	systemLog->sysLog(DEBUG, "[mvhd] nextTrackId = %u", mvhd->nextTrackId);
#endif
	boxSize -= bytesRead;

	return mvhd;
}

tkhd_t *Mp4Reader::parseTkhd(uint32_t boxSize) {
	ssize_t bytesRead;
	int bytesToRead;
	uint32_t flags;
	tkhd_t *tkhd;

	tkhd = new tkhd_t;
	if (! tkhd) {
		systemLog->sysLog(CRITICAL, "cannot allocate mvhd object: %s", strerror(errno));
		return NULL;
	}
	bytesToRead = sizeof(*tkhd);
	bytesRead = mp4File->readStreamFile((char *)tkhd, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read majorBrand correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	flags = (tkhd->flags[0] << 16) || (tkhd->flags[1] << 8) || tkhd->flags[2];
	flags = ntohl(flags);
	tkhd->creationTime = ntohl(tkhd->creationTime);
	tkhd->modificationTime = ntohl(tkhd->modificationTime);
	tkhd->trackId = ntohl(tkhd->trackId);
	tkhd->duration = ntohl(tkhd->duration);
	tkhd->volume = ntohl(tkhd->volume);
	tkhd->width = ntohl(tkhd->width);
	tkhd->height = ntohl(tkhd->height);
#ifdef DEBUGMP4READER
	systemLog->sysLog(DEBUG, "[tkhd] version = %u", tkhd->version);
	systemLog->sysLog(DEBUG, "[tkhd] flags = %u", flags);
	systemLog->sysLog(DEBUG, "[tkhd] creationTime = %u", tkhd->creationTime);
	systemLog->sysLog(DEBUG, "[tkhd] modificationTime = %u", tkhd->modificationTime);
	systemLog->sysLog(DEBUG, "[tkhd] trackId = %u", tkhd->trackId);
	systemLog->sysLog(DEBUG, "[tkhd] duration = %u", tkhd->duration);
 	systemLog->sysLog(DEBUG, "[tkhd] volume = %u", tkhd->volume);
	systemLog->sysLog(DEBUG, "[tkhd] width = %u", tkhd->width);
	systemLog->sysLog(DEBUG, "[tkhd] height = %u", tkhd->height);
#endif
	boxSize -= bytesRead;

	return tkhd;
}

ftyp_t *Mp4Reader::parseFtyp(int32_t boxSize) {
	size_t bytesRead = 0;
	size_t bytesToRead = 0;
	int i;
	ftyp_t *ftyp = NULL;

	try {
		ftyp = new ftyp_t;
		bytesToRead = sizeof(ftyp->majorBrand);
		bytesRead = mp4File->readStreamFile((char *)&ftyp->majorBrand, bytesToRead);
		if (bytesRead != bytesToRead)
			throw 0;
#ifdef DEBUGMP4READER
		systemLog->sysLog(DEBUG, "[ftyp] majorBrand = %c%c%c%c", ((char *)&ftyp->majorBrand)[0], ((char *)&ftyp->majorBrand)[1], ((char *)&ftyp->majorBrand)[2], ((char *)&ftyp->majorBrand)[3]);
#endif
		boxSize -= bytesRead;
		bytesToRead = sizeof(ftyp->minorVersion);
		bytesRead = mp4File->readStreamFile((char *)&ftyp->minorVersion, bytesToRead);
		if (bytesRead != bytesToRead)
			throw 0;
		ftyp->minorVersion = ntohl(ftyp->minorVersion);
#ifdef DEBUGMP4READER
		systemLog->sysLog(DEBUG, "[ftyp] minorVersion = %u", ftyp->minorVersion);
#endif
		boxSize -= bytesRead;
		if (boxSize % 4) {
			systemLog->sysLog(ERROR, "number of compatibleBrands field is incorrect (%d bytes left), must be a multiple of 4", boxSize);
			delete ftyp;
			return NULL;
		}
		ftyp->compatibleBrands = new uint32_t[(boxSize / 4) + 1];
		i = 0;
		while (boxSize) {
			bytesToRead = sizeof(*(ftyp->compatibleBrands));
			bytesRead = mp4File->readStreamFile((char *)&ftyp->compatibleBrands[i], bytesToRead);
			if (bytesRead != bytesToRead)
				throw 0;
#ifdef DEBUGMP4READER
			systemLog->sysLog(DEBUG, "[ftyp] compatibleBrands[%d] = %c%c%c%c", i, ((char *)&ftyp->compatibleBrands[i])[0], ((char *)&ftyp->compatibleBrands[i])[1], ((char *)&ftyp->compatibleBrands[i])[2], ((char *)&ftyp->compatibleBrands[i])[3]);
#endif
			boxSize -= bytesRead;
		}
	}
	catch(int e) {
		if (e == 0) {
			systemLog->sysLog(ERROR, "cannot read majorBrand correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		}
	}

	return ftyp;
}

mdhd_t *Mp4Reader::parseMdhd(uint32_t boxSize) {
	size_t bytesRead;
	size_t bytesToRead;
	uint32_t flags;
	mdhd_t *mdhd;

	mdhd = new mdhd_t;
	if (! mdhd) {
		systemLog->sysLog(CRITICAL, "cannot allocate mvhd object: %s", strerror(errno));
		return NULL;
	}
	bytesToRead = sizeof(*mdhd);
	bytesRead = mp4File->readStreamFile((char *)mdhd, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read majorBrand correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	flags = (mdhd->flags[0] << 16) || (mdhd->flags[1] << 8) || mdhd->flags[2];
	flags = ntohl(flags);
	mdhd->creationTime = ntohl(mdhd->creationTime);
	mdhd->modificationTime = ntohl(mdhd->modificationTime);
	mdhd->timeScale = ntohl(mdhd->timeScale);
	mdhd->duration = ntohl(mdhd->duration);
	mdhd->language = ntohs(mdhd->language);
#ifdef DEBUGMP4READER
	systemLog->sysLog(DEBUG, "[mdhd] version = %u", mdhd->version);
	systemLog->sysLog(DEBUG, "[mdhd] flags = %u", flags);
	systemLog->sysLog(DEBUG, "[mdhd] creationTime = %u", mdhd->creationTime);
	systemLog->sysLog(DEBUG, "[mdhd] modificationTime = %u", mdhd->modificationTime);
	systemLog->sysLog(DEBUG, "[mdhd] timeScale = %u", mdhd->timeScale);
	systemLog->sysLog(DEBUG, "[mdhd] duration = %u", mdhd->duration);
	systemLog->sysLog(DEBUG, "[mdhd] language = %u", mdhd->language);
#endif
	boxSize -= bytesRead;

	return mdhd;
}

stts_t *Mp4Reader::parseStts(uint32_t boxSize) {
	size_t bytesRead;
	size_t bytesToRead;
	uint32_t flags;
	stts_t *stts;
	uint32_t i;

	stts = new stts_t;
	if (! stts) {
		systemLog->sysLog(CRITICAL, "cannot allocate stts object: %s", strerror(errno));
		return NULL;
	}
	bytesToRead = sizeof(stts->version) + sizeof(stts->flags) + sizeof(stts->entryCount);
	bytesRead = mp4File->readStreamFile((char *)stts, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	flags = (stts->flags[0] << 16) || (stts->flags[1] << 8) || stts->flags[2];
	flags = ntohl(flags);
	stts->entryCount = ntohl(stts->entryCount);
	stts->samples = new sttsSamples_t[stts->entryCount];
	bytesToRead = sizeof(*stts->samples) * stts->entryCount;
	bytesRead = mp4File->readStreamFile((char *)stts->samples, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	for (i = 0; i < stts->entryCount; i++) {
		stts->samples[i].sampleCount = ntohl(stts->samples[i].sampleCount);
		stts->samples[i].sampleDelta = ntohl(stts->samples[i].sampleDelta);
	}

#ifdef DEBUGMP4READER
	systemLog->sysLog(DEBUG, "[stts] version = %u", stts->version);
	systemLog->sysLog(DEBUG, "[stts] flags = %u", flags);
	systemLog->sysLog(DEBUG, "[stts] entryCount = %u", stts->entryCount);
	for (i = 0; i < stts->entryCount; i++) {
		systemLog->sysLog(DEBUG, "[stts] sampleCount[%d] = %u", i, stts->samples[i].sampleCount);
		systemLog->sysLog(DEBUG, "[stts] sampleDelta[%d] = %u", i, stts->samples[i].sampleDelta);
	}
#endif
	boxSize -= bytesRead;
	
	return stts;
}

stss_t *Mp4Reader::parseStss(uint32_t boxSize) {
	size_t bytesRead;
	size_t bytesToRead;
	uint32_t i;
	uint32_t flags;
	stss_t *stss;

	stss = new stss_t;
	if (! stss) {
		systemLog->sysLog(CRITICAL, "cannot allocate stss object: %s", strerror(errno));
		return NULL;
	}
	bytesToRead = sizeof(stss->version) + sizeof(stss->flags) + sizeof(stss->entryCount);
	bytesRead = mp4File->readStreamFile((char *)stss, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	boxSize -= bytesRead;
	flags = (stss->flags[0] << 16) || (stss->flags[1] << 8) || stss->flags[2];
	flags = ntohl(flags);
	stss->entryCount = ntohl(stss->entryCount);
	stss->sampleNumber = new uint32_t[stss->entryCount];
	printf("pointer %p\n", stss->sampleNumber);
	bytesToRead = sizeof(*stss->sampleNumber) * stss->entryCount;
	bytesRead = mp4File->readStreamFile((char *)stss->sampleNumber, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	for (i = 0; i < stss->entryCount; i++)
		stss->sampleNumber[i] = ntohl(stss->sampleNumber[i]);
#ifdef DEBUGMP4READER
	systemLog->sysLog(DEBUG, "[stss] version = %u", stss->version);
	systemLog->sysLog(DEBUG, "[stss] flags = %u", flags);
	systemLog->sysLog(DEBUG, "[stss] entryCount = %u", stss->entryCount);
	for (i = 0; i < stss->entryCount; i++)
		systemLog->sysLog(DEBUG, "[stss] sampleNumber[%d] = %u", i, stss->sampleNumber[i]);
#endif
	boxSize -= bytesRead;

	return stss;
}

ctts_t *Mp4Reader::parseCtts(uint32_t boxSize) {
	size_t bytesRead;
	size_t bytesToRead;
	uint32_t i;
	uint32_t flags;
	ctts_t *ctts;

	ctts = new ctts_t;
	if (! ctts) {
		systemLog->sysLog(CRITICAL, "cannot allocate ctts object: %s", strerror(errno));
		return NULL;
	}
	bytesToRead = sizeof(ctts->version) + sizeof(ctts->flags) + sizeof(ctts->entryCount);
	bytesRead = mp4File->readStreamFile((char *)ctts, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	boxSize -= bytesRead;
	flags = (ctts->flags[0] << 16) || (ctts->flags[1] << 8) || ctts->flags[2];
	flags = ntohl(flags);
	ctts->entryCount = ntohl(ctts->entryCount);
	ctts->samples = new cttsSamples_t[ctts->entryCount];
	bytesToRead = sizeof(*ctts->samples) * ctts->entryCount;
	bytesRead = mp4File->readStreamFile((char *)ctts->samples, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	for (i = 0; i < ctts->entryCount; i++) {
		ctts->samples[i].count = ntohl(ctts->samples[i].count);
		ctts->samples[i].offset = ntohl(ctts->samples[i].offset);
	}
#ifdef DEBUGMP4READER
	systemLog->sysLog(DEBUG, "[ctts] version = %u", ctts->version);
	systemLog->sysLog(DEBUG, "[ctts] flags = %u", flags);
	systemLog->sysLog(DEBUG, "[ctts] entryCount = %u", ctts->entryCount);
	for (i = 0; i < ctts->entryCount; i++) {
		systemLog->sysLog(DEBUG, "[ctts] samples[%d].count = %u", i, ctts->samples[i].count);
		systemLog->sysLog(DEBUG, "[ctts] samples[%d].offset = %u", i, ctts->samples[i].offset);
	}
#endif
	boxSize -= bytesRead;

	return ctts;
}

stsz_t *Mp4Reader::parseStsz(uint32_t boxSize) {
	size_t bytesRead;
	size_t bytesToRead;
	uint32_t i;
	uint32_t flags;
	stsz_t *stsz;

	stsz = new stsz_t;
	if (! stsz) {
		systemLog->sysLog(CRITICAL, "cannot allocate stsz object: %s", strerror(errno));
		return NULL;
	}
	bytesToRead = sizeof(stsz->version) + sizeof(stsz->flags) + sizeof(stsz->sampleSize) + sizeof(stsz->sampleCount);
	bytesRead = mp4File->readStreamFile((char *)stsz, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	boxSize -= bytesRead;
	flags = (stsz->flags[0] << 16) || (stsz->flags[1] << 8) || stsz->flags[2];
	flags = ntohl(flags);
	stsz->sampleSize = ntohl(stsz->sampleSize);
	stsz->sampleCount = ntohl(stsz->sampleCount);
	stsz->entrySize = new uint32_t[stsz->sampleCount];
	bytesToRead = sizeof(*stsz->entrySize) * stsz->sampleCount;
	bytesRead = mp4File->readStreamFile((char *)stsz->entrySize, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	for (i = 0; i < stsz->sampleCount; i++)
		stsz->entrySize[i] = ntohl(stsz->entrySize[i]);
#ifdef DEBUGMP4READER
	systemLog->sysLog(DEBUG, "[stsz] version = %u", stsz->version);
	systemLog->sysLog(DEBUG, "[stsz] flags = %u", flags);
	systemLog->sysLog(DEBUG, "[stsz] sampleSize = %u", stsz->sampleSize);
	systemLog->sysLog(DEBUG, "[stsz] sampleCount = %u", stsz->sampleCount);
	for (i = 0; i < stsz->sampleCount; i++)
		systemLog->sysLog(DEBUG, "[stsz] entrySize[%d] = %u", i, stsz->entrySize[i]);
#endif
	boxSize -= bytesRead;

	return stsz;
}

stco_t *Mp4Reader::parseStco(uint32_t boxSize) {
	size_t bytesRead;
	size_t bytesToRead;
	uint32_t flags;
	stco_t *stco;
	uint32_t i;

	stco = new stco_t;
	if (! stco) {
		systemLog->sysLog(CRITICAL, "cannot allocate stco object: %s", strerror(errno));
		return NULL;
	}
	bytesToRead = sizeof(stco->version) + sizeof(stco->flags) + sizeof(stco->entryCount);
	bytesRead = mp4File->readStreamFile((char *)stco, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	boxSize -= bytesRead;
	flags = (stco->flags[0] << 16) || (stco->flags[1] << 8) || stco->flags[2];
	flags = ntohl(flags);
	stco->entryCount = ntohl(stco->entryCount);
	stco->chunkOffset = new uint32_t[stco->entryCount];
	bytesToRead = sizeof(*stco->chunkOffset) * stco->entryCount;
	bytesRead = mp4File->readStreamFile((char *)stco->chunkOffset, bytesToRead);
	if (bytesRead != bytesToRead) {
		systemLog->sysLog(ERROR, "cannot read correctly, expected %d bytes but %d bytes read", bytesToRead, bytesRead);
		return NULL;
	}
	for (i = 0; i < stco->entryCount; i++)
		stco->chunkOffset[i] = ntohl(stco->chunkOffset[i]);
#ifdef DEBUGMP4READER
	systemLog->sysLog(DEBUG, "[stco] version = %u", stco->version);
	systemLog->sysLog(DEBUG, "[stco] flags = %u", flags);
	systemLog->sysLog(DEBUG, "[stco] entryCount = %u", stco->entryCount);
	for (i = 0; i < stco->entryCount; i++)
		systemLog->sysLog(DEBUG, "[stco] chunkOffset[%d] = %u", i, stco->chunkOffset[i]);
#endif
	boxSize -= bytesRead;

	return stco;
}

int Mp4Reader::parseContainer(uint32_t containerSize) {
	char boxName[MP4READER_MAXBOXNAMESIZE + 1];
	uint32_t boxSize;
	uint32_t totalSize;
	int32_t returnCode;
	int32_t singleId;
	uint32_t position;

	totalSize = 0;
	while (! mp4File->feofStreamFile()) {
		returnCode = parseBox(boxName, &boxSize);
#ifdef DEBUGMP4READER
		systemLog->sysLog(DEBUG, "boxName %s, boxSize %u\n", boxName, boxSize);
#endif
		if (returnCode) {
			systemLog->sysLog(ERROR, "cannot parse box, mp4 format error: %s", strerror(errno));
			break;
		}
		
//		containerId = isContainer(boxName);
//		if (containerId != -1) {
//			parseContainer(boxSize, containerId);
//		}
//		else {
			singleId = hashSingleBoxName(boxName);
#ifdef DEBUGMP4READER
			systemLog->sysLog(DEBUG, "singleId is %u\n", singleId);
#endif
			switch (singleId) {
				case FTYPID:
					position = mp4File->getFilePosition() - 8;
					ftyp = parseFtyp(boxSize - 8);
					ftyp->property = new property_t;
					ftyp->property->offset = position;
					ftyp->property->size = boxSize;
					break;
				case MOOVID:
					moov = (moov_t *)malloc(sizeof(moov_t));
					if (! moov) {
						systemLog->sysLog(CRITICAL, "cannot allocate moov_t object: %s", strerror(errno));
						exit(ENOMEM);
					}
					moov->trakCount = 0;
					moov->property = new property_t;
					moov->property->offset = mp4File->getFilePosition() - 8;
					moov->property->size = boxSize;
					parseContainer(boxSize);
					break;
				case MVHDID:
					moov->mvhd = parseMvhd(boxSize - 8);
					break;
				case TRAKID:
					if (moov->trak)
						moov->trakCount++;
					moov->trak = (trak_t **)realloc(moov->trak, (moov->trakCount + 1) * (sizeof(trak_t *)));
					if (! moov->trak) {
						systemLog->sysLog(CRITICAL, "cannot allocate trak_t object: %s", strerror(errno));
						exit(ENOMEM);
					}
					moov->trak[moov->trakCount] = (trak_t *)malloc(sizeof(trak_t));
					if (! moov->trak[moov->trakCount]) {
						systemLog->sysLog(CRITICAL, "cannot allocate trak_t[%u] object: %s", moov->trakCount, strerror(errno));
						exit(ENOMEM);
					}
					parseContainer(boxSize);
					break;
				case TKHDID:
					moov->trak[moov->trakCount]->tkhd = parseTkhd(boxSize - 8);
					break;
				case MDIAID:
					moov->trak[moov->trakCount]->mdia = (mdia_t *)malloc(sizeof(mdia_t));
					if (! moov->trak[moov->trakCount]->mdia) {
						systemLog->sysLog(CRITICAL, "cannot allocate moov->trak[%u]->mdia object: %s", moov->trakCount, strerror(errno));
						exit(ENOMEM);
					}
					parseContainer(boxSize);
					break;
				case MDHDID:
					moov->trak[moov->trakCount]->mdia->mdhd = parseMdhd(boxSize - 8);
					break;
				case MINFID:
					moov->trak[moov->trakCount]->mdia->minf = (minf_t *)malloc(sizeof(minf_t));
					if (! moov->trak[moov->trakCount]->mdia->minf) {
						systemLog->sysLog(CRITICAL, "cannot allocate moov->trak[%u]->mdia->minf object: %s", moov->trakCount, strerror(errno));
						exit(ENOMEM);
					}
					parseContainer(boxSize);
					break;
				case STBLID:
					moov->trak[moov->trakCount]->mdia->minf->stbl = (stbl_t *)malloc(sizeof(stbl_t));
					if (! moov->trak[moov->trakCount]->mdia->minf->stbl) {
						systemLog->sysLog(CRITICAL, "cannot allocate moov->trak[%u]->mdia->minf->stbl object: %s]", moov->trakCount, strerror(errno));
						exit(ENOMEM);
					}
					parseContainer(boxSize);
					break;
				case STTSID:
					moov->trak[moov->trakCount]->mdia->minf->stbl->stts = parseStts(boxSize - 8);
					break;
				case STSSID:
					moov->trak[moov->trakCount]->mdia->minf->stbl->stss = parseStss(boxSize - 8);
					break;
				case CTTSID:
					moov->trak[moov->trakCount]->mdia->minf->stbl->ctts = parseCtts(boxSize - 8);
					break;
				case STSZID:
					moov->trak[moov->trakCount]->mdia->minf->stbl->stsz = parseStsz(boxSize - 8);
					break;
				case STCOID:
					position = mp4File->getFilePosition() - 8;
					moov->trak[moov->trakCount]->mdia->minf->stbl->stco = parseStco(boxSize - 8);
					moov->trak[moov->trakCount]->mdia->minf->stbl->stco->property = new property_t;
					moov->trak[moov->trakCount]->mdia->minf->stbl->stco->property->offset = position;
					moov->trak[moov->trakCount]->mdia->minf->stbl->stco->property->size = boxSize;
					break;
				case MDATID:
					mdat = (mdat_t *)malloc(sizeof(mdat_t));
					if (! mdat) {
						systemLog->sysLog(CRITICAL, "cannot allocate mdat_t object: %s", strerror(errno));
						exit(ENOMEM);
					}
					mdat->property = new property_t;
					mdat->property->offset = mp4File->getFilePosition() - 8;
					mdat->property->size = boxSize;
					mp4File->seekStreamFile(boxSize - 8, SEEK_CUR);
					break;
 				default:
					mp4File->seekStreamFile(boxSize - 8, SEEK_CUR);
					break;
			}
//		}
		totalSize += boxSize;
	}

	return 0;
}

int Mp4Reader::parse(void) {
	int returnCode;

	returnCode = parseContainer(0);

	return returnCode;
}

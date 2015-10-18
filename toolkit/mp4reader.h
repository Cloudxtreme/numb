//
// C++ Interface: mp4reader
//
// Description: 
//
//
// Author:  <spe@>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MP4READER_H
#define MP4READER_H

#include "../toolkit/file.h"

/**
	@author  <spe@>
*/

#define MP4READER_MAXBOXNAMESIZE	4

#define	MOOVCONTAINERID			0

#define	FTYPID				0
#define PDINID				1
#define MOOVID				2
#define MVHDID				3
#define TRAKID				4
#define TKHDID				5
#define MDIAID				6
#define	MDHDID				7
#define MINFID				8
#define STBLID				9
#define	STTSID				16
#define STSSID				10
#define CTTSID				11
#define	STSZID				12
#define	STCOID				13
#define MDATID				14
#define FREEID				15

typedef struct property {
	uint32_t offset;
	uint32_t size;
} property_t;

typedef struct ftyp {
	uint32_t majorBrand;
	uint32_t minorVersion;
	uint32_t *compatibleBrands;
	property_t *property;
} ftyp_t;

typedef struct mvhd {
	u_char version;
	u_char flags[3];
	uint32_t creationTime;
	uint32_t modificationTime;
	uint32_t timeScale;
	uint32_t duration;
	int32_t rate;
	int16_t volume;
	u_char reserved[10];
	uint32_t matrix[9];
	u_char reserved2[24];
	uint32_t nextTrackId;
} mvhd_t;

typedef struct tkhd {
	u_char version;
	u_char flags[3];
	uint32_t creationTime;
	uint32_t modificationTime;
	uint32_t trackId;
	u_char reserved1[4];
	uint32_t duration;
	u_char reserved2[12];
	int16_t volume;
	u_char reserved3[2];
	int32_t matrix[9];
	int32_t width;
	int32_t height;
} tkhd_t;

typedef struct elstSamples {
	uint32_t segmentDuration;
	int32_t mediaTime;
	int16_t mediaRateInteger;
	int16_t mediaRateFraction;
} elstSamples_t;

typedef struct elst {
	u_char version;
	u_char flags[3];
	uint32_t entryCount;
	elstSamples_t *samples;
} elst_t;

typedef struct mdhd {
	u_char version;
	u_char flags[3];
	uint32_t creationTime;
	uint32_t modificationTime;
	uint32_t timeScale;
	uint32_t duration;
	uint16_t language;
	u_char reserved[2];
} mdhd_t;

typedef struct hdlr {
	u_char version;
	u_char flags[3];
	u_char reserved[4];
	uint32_t handlerType;
	u_char reserved2[12];
	char *name;	// string terminated by '\0'
} hdlr_t;

typedef struct vmhd {
	u_char version;
	uint32_t flags;
	u_char reserved;
} vmhd_t;

typedef struct dref {
	u_char version;
	u_char flags[3];
	uint32_t entryCount;
} dref_t;

typedef struct url {
	u_char version;
	u_char flags[3];
	char *location; // utf-8 nul terminated string
} url_t;

typedef struct avc1 {
	u_char reserved1[6];
	uint16_t dataReferenceIndex;
	u_char reserved2[16];
	uint16_t width;
	uint16_t height;
	u_char reserved3[14];
	char *compressorName;
	u_char reserved4[4];
} avc1_t;

typedef struct avcC {
	u_char configurationVersion;
	u_char AVCProfileIndication;
	u_char profileCompatibility;
	u_char AVCLevelIndication;
	u_char reserved; // only 6 bits
	u_char lengthSizeMinusOne; // only 6 bits
	u_char reserved1; // only 3 bits
	u_char numOfSequenceParamaterSet; // only 5 bits
	uint16_t sequenceParameterSetLength;
	u_char *sequenceParameterSetNALUnit;
	u_char numOfPictureParameterSets;
	uint16_t pictureParameterSetLength;
	u_char *pictureParameterSetNALUnit;
} avcC_t;

typedef struct stss {
	u_char version;
	u_char flags[3];
	uint32_t entryCount;
	uint32_t *sampleNumber;
} stss_t;

typedef struct cttsSamples {
	uint32_t count;
	uint32_t offset;
} cttsSamples_t;

typedef struct ctts {
	u_char version;
	u_char flags[3];
	uint32_t entryCount;
	cttsSamples_t *samples;
} ctts_t;

typedef struct stscSamples {
	uint32_t firstChunk;
	uint32_t samplesPerChunk;
	uint32_t sampleDescriptionIndex;
} stscSamples_t;

typedef struct stsc {
	u_char version;
	u_char flags[3];
	uint32_t entryCount;
	stscSamples_t *samples;
} stsc_t;

typedef struct stsz {
	u_char version;
	u_char flags[3];
	uint32_t sampleSize;
	uint32_t sampleCount;
	uint32_t *entrySize;
} stsz_t;

typedef struct stco {
	u_char version;
	u_char flags[3];
	uint32_t entryCount;
	uint32_t *chunkOffset;
	property_t *property;
} stco_t;

typedef struct stsd {
	u_char version;
	u_char flags[3];
	uint32_t entryCount;
} stsd_t;

typedef struct mp4a {
	u_char reserved1[6];
	uint16_t dataReferenceIndex;
	uint16_t soundVersion;
	u_char reserved2[6];
	uint16_t channels;
	uint16_t sampleSize;
	uint16_t packetSize;
	uint32_t timeScale;
	u_char reserved3[2];
} mp4a_t;

typedef struct dinf {
	dref_t *dref;
} dinf_t;

typedef struct sttsSamples {
	uint32_t sampleCount;
	uint32_t sampleDelta;
} sttsSamples_t;

typedef struct stts {
	u_char version;
	u_char flags[3];
	uint32_t entryCount;
	sttsSamples_t *samples;
} stts_t;

typedef struct stbl {
	stsd_t *stsd;
	stts_t *stts;
	ctts_t *ctts;
	stsc_t *stsc;
	stsz_t *stsz;
	stco_t *stco;
	stss_t *stss;
} stbl_t;

typedef struct minf {
	vmhd_t *vmhd;
	dinf_t *dinf;
	stbl_t *stbl;
} minf_t;

typedef struct mdia {
	mdhd_t *mdhd;
	hdlr_t *hdlr;
	minf_t *minf;
} mdia_t;

typedef struct trak {
	tkhd_t *tkhd;
	mdia_t *mdia;
} trak_t;

typedef struct moov {
	mvhd_t *mvhd;
	uint32_t trakCount;
	trak_t **trak;
	property_t *property;
} moov_t;

typedef struct mdat {
	property_t *property;
} mdat_t;

typedef struct offsets {
	char boxName[5];
	uint32_t relative;
} offsets_t;

class Mp4Reader{
private:
	File *mp4File;
	struct stat *sb;
	uint32_t fileOffset;
	ftyp_t *ftyp;
	moov_t *moov;
	mdat_t *mdat;
	offsets_t offsets[128];
	u_char offsetCount;

public:
	Mp4Reader(char *);
	~Mp4Reader();

	int parseMvhdBox();
	int parseBox(char *, uint32_t *);
	int isContainer(char *);
	int hashMoovBoxName(char *);
	int hashSingleBoxName(char *);
	ftyp_t *parseFtyp(int32_t);
	mvhd_t *parseMvhd(uint32_t);
	tkhd_t *parseTkhd(uint32_t);
	mdhd_t *parseMdhd(uint32_t);
	stts_t *parseStts(uint32_t);
	stss_t *parseStss(uint32_t);
	ctts_t *parseCtts(uint32_t);
	stsz_t *parseStsz(uint32_t);
	stco_t *parseStco(uint32_t);
	int parseContainer(uint32_t);
	int parse(void);

	ftyp_t *getFtyp(void) { return ftyp; };
	moov_t *getMoov(void) { return moov; };
	mvhd_t *getMvhd(void) { return moov->mvhd; };
	mdat_t *getMdat(void) { return mdat; };
	trak_t **getTrak(void) { return moov->trak; };
	offsets_t *getOffsets(void) { return offsets; };
	u_char getOffsetCount(void) { return offsetCount; };
};

#endif

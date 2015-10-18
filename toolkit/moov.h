/*******************************************************************************
 moov.h (version 2)

 moov - A library for splitting Quicktime/MPEG4 files.
 http://h264.code-shop.com

 Copyright (C) 2007-2009 CodeShop B.V.

 Licensing
 The H264 Streaming Module is licened under a Creative Common License. It allows
 you to use, modify and redistribute the module, but only for *noncommercial*
 purposes. For corporate use, please apply for a commercial license.

 Creative Commons License:
 http://creativecommons.org/licenses/by-nc-sa/3.0/

 Commercial License:
 http://h264.code-shop.com/trac/wiki/Mod-H264-Streaming-License-Version2
******************************************************************************/ 

// NOTE: don't include stdio.h (for FILE) or sys/types.h (for off_t).
// nginx redefines _FILE_OFFSET_BITS and off_t will have different sizes
// depending on include order

#include "../toolkit/log.h"

#include <inttypes.h>

extern "C" {

extern int mp4_split(const char* infile, int64_t filesize,
                     float start_time, float end_time,
                     void** mp4_header, uint32_t* mp4_header_size,
                     uint64_t* mdat_offset, uint64_t* mdat_size,
                     int client_is_flash);

/* Returns true when the test string is a prefix of the input */
extern int starts_with(const char* input, const char* test);

} /* extern C definitions */

// End Of File


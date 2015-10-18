//
// C++ Implementation: encoder
//
// Description: 
//
//
// Author: spe <spe@ulyssia.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "encoder.h"
#include "../toolkit/log.h"
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#include <errno.h>

Encoder::Encoder() {
  av_register_all();
  avFormatContext = NULL;
  avCodecContext = NULL;
  audioCodecContext = NULL;
  videoCodecContext = NULL;
  avCodec = NULL;
  audioCodec = NULL;
  videoCodec = NULL;
  avFrameRGB = NULL;
  avFrame = NULL;
  buffer = NULL;
  videoStream = 0;
  rawData = NULL;
  bytesRemaining = 0;
  frameRead = 0;
  bzero(&avPacket, sizeof(avPacket));
  bzero(&videoPacket, sizeof(videoPacket));
  bzero(&audioPacket, sizeof(audioPacket));

  return;
}

Encoder::~Encoder() {
  cleanup();

  return;
}

int Encoder::openVideoFile(char *_videoFileName) {
  int returnCode;

  returnCode = av_open_input_file(&avFormatContext, _videoFileName, NULL, 0, NULL);
  if (returnCode < 0) {
    systemLog->sysLog(ERROR, "cannot open the video file: %s", strerror(errno));
    return -1;
  }
  videoFileName = _videoFileName;

  return returnCode;
}

int Encoder::retrieveStreamInformation(void) {
  int returnCode;

  if (! avFormatContext) {
    systemLog->sysLog(ERROR, "avcodec context format is NULL. call openVideoFile first !");
    return -1;
  }
  returnCode = av_find_stream_info(avFormatContext);
  if (returnCode < 0)
    systemLog->sysLog(ERROR, "cannot retrieve stream information: %s", strerror(errno));

  return returnCode;
}

int Encoder::dumpStreamInformation(void) {
  if (! avFormatContext || ! videoFileName) {
    systemLog->sysLog(ERROR, "avcodec context format or videoFileName not initialized. call openVideoFile first !");
    return -1;
  }
  dump_format(avFormatContext, 0, videoFileName, false);

  return 0;
}

int Encoder::searchFirstVideoStream(void) {
  int i;

  for (i = 0; i < avFormatContext->nb_streams; i++) {
    if (avFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
      videoStream = i;
      break;
    }
  }
  if (videoStream < 0) {
    systemLog->sysLog(ERROR, "there is no video stream in this video file");
    return -1;
  }

  // Get a pointer to the codec context for the video stream
  videoCodecContext = avFormatContext->streams[videoStream]->codec;

  // Set packet.data to NULL for starting to decoder
  videoPacket.data = NULL;

  return 0;
}

int Encoder::searchFirstAudioStream(void) {
  int i;

  for (i = 0; i < avFormatContext->nb_streams; i++) {
    if (avFormatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
      audioStream = i;
      break;
    }
  }
  if (audioStream < 0) {
    systemLog->sysLog(ERROR, "there is no audio stream in this video file");
    return -1;
  }

  // Get a pointer to the codec context for the audio stream
  audioCodecContext = avFormatContext->streams[audioStream]->codec;

  // Set packet.data to NULL for starting to decoder
  audioPacket.data = NULL;

  return 0;
}

int Encoder::searchFirstAVStream(void) {
  int returnCode;

  returnCode = searchFirstVideoStream();
  if (returnCode < 0)
    return returnCode;
  returnCode = searchFirstAudioStream();

  return returnCode;
}

/* AVCodec *Encoder::openCodec(void) {
  AVCodec *avCodec;

  // Find the decoder for the audio/video stream
  avCodec = avcodec_find_decoder(avCodecContext->codec_id);
  if (! avCodec) {
    systemLog->sysLog(ERROR, "codec was not found or not supported");
    return NULL;
  }

  // Finally open the codec
  returnCode = avcodec_open(avCodecContext, avCodec);
  if (returnCode < 0) {
    systemLog->sysLog(ERROR, "could not open the codec, something is terribly wrong in the ffmpeg installation");
    return NULL;
  }

  return avCodec;
} */

int Encoder::openCodec(void) {
  // Find the decoder for the audio/video stream
  avCodec = avcodec_find_decoder(avCodecContext->codec_id);
  if (! avCodec) {
    systemLog->sysLog(ERROR, "codec was not found or not supported");
    return -1;
  }

  // Finally open the codec
  returnCode = avcodec_open(avCodecContext, avCodec);
  if (returnCode < 0) {
    systemLog->sysLog(ERROR, "could not open the codec, something is terribly wrong in the ffmpeg installation");
    return -1;
  }

  return 0;
}

bool Encoder::getNextFrame(void) {
  int bytesDecoded;
  int isFrameFinished;
  int returnCode;

  if (avFrame)
    av_free(avFrame);

  avFrame = avcodec_alloc_frame();
  if (avFrame == NULL) {
    systemLog->sysLog(ERROR, "cannot allocate a YUV frame. cannot decode frame");
    return false;
  }

  while (av_read_frame(avFormatContext, &avPacket) >= 0) {
    frameRead++;
    // Is this packet a packet from this video stream ?
    if (avPacket.stream_index == videoStream) {
      // Decode video frame
      systemLog->sysLog(NOTICE, "BLAHHHHHH");
      avcodec_decode_video(avCodecContext, avFrame, &isFrameFinished, avPacket.data, avPacket.size);
      if (isFrameFinished)
        return true;
    }
  }

  return false;
}

int Encoder::getNextFrame(void) {
  int bytesDecoded;
  int isFrameFinished;
  int returnCode;

  if (avFrame)
    av_free(avFrame);

  avFrame = avcodec_alloc_frame();
  if (avFrame == NULL) {
    systemLog->sysLog(ERROR, "cannot allocate a YUV frame. cannot decode frame");
    return false;
  }

  if (audioSamples)
    delete audioSamples;


  while (av_read_frame(avFormatContext, &avPacket) >= 0) {
    // Is this packet a packet from this video stream ?
    if (videoPacket.stream_index == videoStream) {

      // Decode video frame
      systemLog->sysLog(NOTICE, "decoding a video frame");
      returnCode = avcodec_decode_video(videoCodecContext, avFrame, &isFrameFinished, videoPacket.data, videoPacket.size);
      if (returnCode < 0)
        systemLog->sysLog(ERROR, "problem while decoding a video frame");
      if (isFrameFinished)
        return true;
    }
    if (audioPacket.stream_index == audioStream) {
      // Decode audio frame
      systemLog->sysLog(NOTICE, "decoding an audio frame");
      returnCode = avcodec_decode_audio(audioCodecContext, audioSamples, &isFrameFinished, audioPacket.data, audioPacket.size);
      if (returnCode < 0)
        systemLog->sysLog(ERROR, "problem while decoding an audio frame");
      if (isFrameFinished)
        return true;
    }
  }

  return false;
}

int Encoder::allocateFrameRGB(void) {
  int numBytes;

  // Allocate an AVFrame structure
  if (buffer)
    delete [] buffer;
  if (avFrameRGB)
    av_free(avFrameRGB);

  avFrameRGB = avcodec_alloc_frame();
  if (avFrameRGB == NULL) {
    systemLog->sysLog(ERROR, "can't allocate an RGB frame");
    return -1;
  }

  // Determine required buffer size and allocate buffer
  numBytes = avpicture_get_size(PIX_FMT_RGB24, avCodecContext->width, avCodecContext->height);
  buffer = new uint8_t[numBytes];

  // Assign appropriate parts of buffer to image planes in avFrameRGB
  avpicture_fill((AVPicture *)avFrameRGB, buffer, PIX_FMT_RGB24, avCodecContext->width, avCodecContext->height);

  return 0;
}

void Encoder::YUVtoRGB(void) {
  // Allocate a RGB Frame
  allocateFrameRGB();

  // convert a YUV Frame into RGB Frame
  img_convert((AVPicture *)avFrameRGB, PIX_FMT_RGB24, (AVPicture *)avFrame, avCodecContext->pix_fmt, avCodecContext->width, avCodecContext->height);

  return;
}

int Encoder::saveFrame(int frameNumber) {
  FILE *pFile;
  char szFilename[32];
  int i;

  // Open File
  snprintf(szFilename, sizeof(szFilename), "frame%d.ppm", frameNumber);
  pFile = fopen(szFilename, "w+");
  if (pFile == NULL) {
    systemLog->sysLog(ERROR, "cannot open file for writing the frame on disk: %s", strerror(errno));
    return -1;
  }

  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", avCodecContext->width, avCodecContext->height);

  // Write data
  for (i = 0; i < avCodecContext->height; i++)
    fwrite(avFrameRGB->data[0]+i*avFrameRGB->linesize[0], 1, avCodecContext->width*3, pFile);

  // Close file
  fclose(pFile);

  // Notice the file creation
  systemLog->sysLog(NOTICE, "the frame %s is wrote on disk", szFilename);

  return 0;
}

void Encoder::cleanup(void) {
  if (buffer)
    delete buffer;
  if (avFrameRGB)
    av_free(avFrameRGB);
  if (avFrame)
    av_free(avFrame);

  if (avCodec)
    avcodec_close(avCodecContext);
  if (avFormatContext)
    av_close_input_file(avFormatContext);

  return;
}

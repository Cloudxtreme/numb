//
// C++ Interface: encoder
//
// Description: 
//
//
// Author: spe <spe@ulyssia.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef ENCODER_H
#define ENCODER_H

#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>

/**
@author spe
*/
class Encoder {
private:
  int videoStream;
  int audioStream;
  int audioVideoStream;
  char *videoFileName;

public:
  AVFormatContext *avFormatContext;
  AVCodecContext *avCodecContext;
  AVCodecContext *audioCodecContext;
  AVCodecContext *videoCodecContext;
  AVCodec *avCodec;
  AVCodec *audioCodec;
  AVCodec *videoCodec;
  AVFrame *avFrameRGB; // an RGB Frame
  AVFrame *avFrame;    // a YUV Frame
  uint8_t *buffer;
  int frameRead;

  // Necessary for decoding frames
  AVPacket avPacket;
  AVPacket audioPacket;
  AVPacket videoPacket;
  int bytesRemaining;
  uint8_t *rawData;

  Encoder();
  ~Encoder();

  int Encoder::openVideoFile(char *);
  int Encoder::retrieveStreamInformation(void);
  int Encoder::dumpStreamInformation(void);
  int Encoder::searchFirstVideoStream(void);
  //AVCodec *Encoder::openCodec(void);
  int Encoder::openCodec(void);
  bool Encoder::getNextFrame(void);
  int Encoder::allocateFrameRGB(void);
  void Encoder::YUVtoRGB(void);
  int Encoder::saveFrame(int);
  void Encoder::cleanup(void);
};

#endif

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
                 TrafficGenerator.h  -  simple traffic generator
                             -------------------
    begin                : Wed Jul 13 2005
    copyright            : (C) 2005 by M. Bohge
    email                : bohge@tkn.tu-berlin.de
 ***************************************************************************/

#ifndef VOIPTOOL_VOIPGENERATOR_H
#define VOIPTOOL_VOIPGENERATOR_H

#include <fnmatch.h>
#include <vector>
#include <omnetpp.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

#include "IPAddressResolver.h"
#include "UDPAppBase.h"
#include "UDPControlInfo_m.h"
#include "VoIPPacket_m.h"

//using namespace std;

class INET_API VoIPGenerator : public UDPAppBase
{
  public:
    ~VoIPGenerator();

  protected:
    virtual void initialize(int stage);
    virtual int numInitStages() const {return 4;}
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    void openSoundFile(const char *name);
    VoIPPacket* generatePacket();
    bool checkSilence(void* _buf, int samples);
    void readFrame();

  protected:
    int localPort;
    int destPort;
    IPvXAddress destAddress;
    int voipPktSize;                // size of VoIP packets
    int voipHeaderSize;
    int voipSilenceThreshold;       // the maximum amplitude of a silence packet
    int sampleRate;                 // samples/sec [Hz]
    short int sampleBits;           // bits/sample (8,16,32)  // the 24 is not supported by ffmpeg
    short int sampleBytes;          // bytes/sample (1,2,4)  // the 3 is not supported by ffmpeg
    const char *codec;
    int compressedBitRate;
    simtime_t packetTimeLength;
    const char *soundFile;          // input audio file name
    int repeatCount;
    const char *traceFileName;      // how the tracefiles should be named

    int voipSilenceSize;            // size of a silence packet
    int noWavFiles;                 // number of VoIP wav files available

    enum SampleFormat sampleFormat; // ffmpeg: enum SampleFormat
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;                // input decoder codec
    ReSampleContext *pReSampleCtx;
    AVCodecContext *pEncoderCtx;
    AVCodec *pCodecEncoder;         // output encoder codec
    int streamIndex;
    int unreadSamples;
    uint32_t pktID;                 // increasing packet sequence number
    bool writeTracesToDisk;         // bool value - parameter if VoIP tracefiles should be written to disk
    int samplesPerPacket;

    char *samplePtr;
    char *newSamples;
    char *samples;

    cMessage timer;
};

#endif //VOIPTOOL_VOIPGENERATOR_H

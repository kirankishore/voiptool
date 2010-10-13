//
// Copyright (C) 2005 M. Bohge (bohge@tkn.tu-berlin.de), M. Renwanz
// Copyright (C) 2010 Zoltan Bojthe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


#ifndef VOIPTOOL_VOIPSOURCEAPP_H
#define VOIPTOOL_VOIPSOURCEAPP_H

#include <fnmatch.h>
#include <vector>
#include <omnetpp.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

#include "AudioOutFile.h"
#include "IPAddressResolver.h"
#include "UDPAppBase.h"
#include "UDPControlInfo_m.h"
#include "VoIPPacket_m.h"

//using namespace std;

class INET_API VoIPSourceApp : public UDPAppBase
{
  public:
    ~VoIPSourceApp();

  protected:
    virtual void initialize(int stage);
    virtual int numInitStages() const {return 4;}
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    virtual void openSoundFile(const char *name);
    virtual VoIPPacket* generatePacket();
    virtual bool checkSilence(void* _buf, int samples);
    virtual void readFrame();

  protected:
    class Buffer
    {
      public:
        enum { BUFSIZE = AVCODEC_MAX_AUDIO_FRAME_SIZE };
      protected:
        char *samples;
        int bufferSize;
        int readOffset;
        int writeOffset;
      public:
        Buffer();
        ~Buffer();
        void clear(int framesize);
        int length() const {return writeOffset - readOffset; }
        bool empty() const {return writeOffset <= readOffset; }
        char* readPtr() { return samples + readOffset; }
        char* writePtr() { return samples + writeOffset; }
        int availableSpace() const {return bufferSize - writeOffset; }
        void notifyRead(int length) { readOffset += length; ASSERT(readOffset <= writeOffset); }
        void notifyWrote(int length) { writeOffset += length; ASSERT(writeOffset <= bufferSize); }
        void align();
    };
    AudioOutFile outFile;
    int localPort;
    int destPort;
    IPvXAddress destAddress;
    int voipPktSize;                // size of VoIP packets
    int voipHeaderSize;
    int voipSilenceThreshold;       // the maximum amplitude of a silence packet
    int sampleRate;                 // samples/sec [Hz]
    short int bitsPerSample;           // bits/sample (8,16,32)  // the 24 is not supported by ffmpeg
    short int bytesPerSample;          // bytes/sample (1,2,4)  // the 3 is not supported by ffmpeg
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
    uint32_t pktID;                 // increasing packet sequence number
    bool writeTracesToDisk;         // bool value - parameter if VoIP tracefiles should be written to disk
    int samplesPerPacket;
    Buffer sampleBuffer;
    AVPacket packet;

    cMessage timer;
};

#endif //VOIPTOOL_VOIPSOURCEAPP_H

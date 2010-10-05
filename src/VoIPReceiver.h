#ifndef VOIPTOOL_VOIPRECEIVER_H
#define VOIPTOOL_VOIPRECEIVER_H

#include <omnetpp.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

#include <iostream>
#include <sys/stat.h>

#include "IPAddressResolver.h"
#include "UDPAppBase.h"
#include "UDPControlInfo_m.h"

#include "VoIPGenerator.h"
#include "VoIPPacket_m.h"


class VoIPReceiver : public UDPAppBase
{
  public:
    VoIPReceiver() : samples(NULL), g726buf(NULL), timer(NULL) {}
    ~VoIPReceiver();
	
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

    void initializeAudio();
    bool createConnect(VoIPPacket *vp);
    bool checkConnect(VoIPPacket *vp);
    void closeConnect();
    void handleVoIPMessage(VoIPPacket *vp);
    void decodePacket(VoIPPacket *vp);
    static void initialiseStatics();

    //write Fake Header without information about the length of the file... the length will be added later (since the length is still unknown)
    void writeFakeWavHeader(const char *filename);
    void correctWavHeader(const char *filename);

    class Connection
    {
      public:
        Connection() : offline(true) {}
        void addAudioStream(enum CodecID codec_id);
        void openAudio();
        void writeAudioFrame(uint8_t *buf, int len);
        void writeLostFrames(int frameCount);
        void closeAudio();

        bool offline;
        uint16_t seqNo;
        uint32_t timeStamp;
        uint32_t ssrc;
        enum CodecID codec;
        short sampleBits;
        int sampleRate;
        AVFormatContext *oc;
        AVOutputFormat *fmt;
        AVStream *audio_st;
        AVCodecContext *DecCtx;
        AVCodec *pCodecDec;
        int pktBytes;
    };

  protected:
    int localPort;
    simtime_t timeout;
    const char *resultFile;

    Connection curConn;

    int pktno;						// packet number for voip packets
    int audiostream;					// audiostream number inside audiofile
    int16_t *samples;					// buffer for 16 bit raw audio samples
    uint8_t *g726buf;					// buffer for encoded g726 data
    int unreadSamples;				// number of samples not processed yet
    int startPos;						// startposition in samples buffer of the current frame
    int psamples;					// current position in samples buffer
    int transmissionErrors;
    int numberOfVoIpSilence;
/*
    AVFormatContext *pFormatCtx;
    AVCodecContext *p726EncCtx;
    AVCodecContext *p726DecCtx;
    ReSampleContext *pReSampleCtx;
    AVCodec *pCodec;
    AVCodec *pCodec726Enc;
    AVCodec *pCodec726Dec;
    AVPacket packet;
    FILE *original;			// file pointer to original audiofile (after resampling, will be created)
    FILE *degenerated;		// file pointer to degenerated file (inkl. codec loss, packet loss and silence packets)
*/
    cMessage *timer;
    static simsignal_t receivedBytes;
    static simsignal_t missingPackets;
    static simsignal_t droppedBytes;
    static simsignal_t packetHasVoice;
    static simsignal_t connState;
};

#endif // VOIPTOOL_VOIPRECEIVER_H

#ifndef VOIPSINK_H
#define VOIPSINK_H

#include <omnetpp.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

#include <iostream>
#include <sys/stat.h>
#include "IpPacket_m.h"
#include "SysMsg_m.h"
#include "VoIPGenerator.h"
#include  "UDPAppBase.h"
#include <UDPControlInfo_m.h>
#include "IPAddressResolver.h"

using namespace std;

class VoIPSink : public UDPAppBase
{
	//Module_Class_Members(VoIPSink, cSimpleModule, 0);
	public:
	
	protected:
		virtual void initialize();
		virtual void handleMessage(cMessage *msg);
		virtual void finish();
		
		void initializeAudio();
		void handleMessage2(cMessage *msg);
		
		//write Fake Header without information about the length of the file... the length will be added later (since the length is still unknown)
		void writeFakeWavHeader(const char *filename);
		void correctWavHeader(const char *filename);

		//returns 0 in case of success or -1 in case of EOF
		int readNextFrame();
		void encodeNextPacket();
		
	protected:
		int samplesPerPacket;
		bool computePesqValue;
        bool resample;                  // resample check
		int codingRate;
		
		int pktno;						// packet number for voip packets
		int audiostream;					// audiostream number inside audiofile
		int localPort;
		char *cur_file;				// pointer to current audio filename
		const char *originalWavFileName;
		const char *degeneratedWavFileName;
		const char *resultFile;
		int16_t *samples;					// buffer for 16 bit raw audio samples
		uint8_t *g726buf;					// buffer for encoded g726 data
		int unreadSamples;				// number of samples not processed yet
		int startPos;						// startposition in samples buffer of the current frame
		int psamples;					// current position in samples buffer
		int transmissionErrors;
		int numberOfVoIpSilence;
		AVFormatContext *pFormatCtx;
		AVCodecContext *pCodecCtx;
		AVCodecContext *p726EncCtx;
		AVCodecContext *p726DecCtx;
		ReSampleContext *pReSampleCtx;
		AVCodec *pCodec;
		AVCodec *pCodec726Enc;
		AVCodec *pCodec726Dec;
		AVPacket packet;
		FILE *original;			// file pointer to original audiofile (after resampling, will be created)
		FILE *degenerated;		// file pointer to degenerated file (inkl. codec loss, packet loss and silence packets)
};

#endif

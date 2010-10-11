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


#include "VoIPSinkApp.h"

#include "INETEndians.h"

// FIXME check on WINDOWS!!
#define INT64_C(x) int64_t(x##ULL)
// FIXME check on WINDOWS!!

Define_Module(VoIPSinkApp);

simsignal_t VoIPSinkApp::receivedBytesSignal = SIMSIGNAL_NULL;
simsignal_t VoIPSinkApp::lostSamplesSignal = SIMSIGNAL_NULL;
simsignal_t VoIPSinkApp::lostPacketsSignal = SIMSIGNAL_NULL;
simsignal_t VoIPSinkApp::droppedBytesSignal = SIMSIGNAL_NULL;
simsignal_t VoIPSinkApp::packetHasVoiceSignal = SIMSIGNAL_NULL;
simsignal_t VoIPSinkApp::connStateSignal = SIMSIGNAL_NULL;
simsignal_t VoIPSinkApp::delaySignal = SIMSIGNAL_NULL;

VoIPSinkApp::~VoIPSinkApp()
{
    closeConnect();
}

void VoIPSinkApp::initialiseStatistics()
{
    if (receivedBytesSignal != SIMSIGNAL_NULL)
        return;

    receivedBytesSignal = registerSignal("receivedBytes");
    lostSamplesSignal = registerSignal("lostSamples");
    lostPacketsSignal = registerSignal("lostPackets");
    droppedBytesSignal  = registerSignal("droppedBytes");
    packetHasVoiceSignal = registerSignal("packetHasVoice");
    connStateSignal = registerSignal("connState");
    delaySignal = registerSignal("delay");
}

void VoIPSinkApp::initialize()
{
    UDPAppBase::initialize();
    initialiseStatistics();

    // Say Hello to the world
	ev << "VoIPSinkApp initialize()" << endl;

	//read in omnet parameters
	localPort = par("localPort");
	resultFile = par("resultFile");

	//initialize avcodec library
	av_register_all();

	bindToPort(localPort);
}

void VoIPSinkApp::handleMessage(cMessage *msg)
{
    VoIPPacket *vp = dynamic_cast<VoIPPacket *>(msg);
    if(vp)
        handleVoIPMessage(vp);
    else
        delete msg;
}

void VoIPSinkApp::Connection::openAudio()
{
    AVCodecContext *c;
    AVCodec *avcodec;

    c = audio_st->codec;

    /* find the audio encoder */
    avcodec = avcodec_find_encoder(c->codec_id);
    if (!avcodec)
        throw cRuntimeError("codec %d not found\n", c->codec_id);

    /* open it */
    if (avcodec_open(c, avcodec) < 0)
        throw cRuntimeError("could not open codec %d\n", c->codec_id);

}

/* prepare a 16 bit dummy audio frame of 'frame_size' samples and
   'nb_channels' channels */
/*
void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
    int j, i, v;
    int16_t *q;

    q = samples;
    for(j=0;j<frame_size;j++) {
        v = (int)(sin(t) * 10000);
        for(i = 0; i < nb_channels; i++)
            *q++ = v;
        t += tincr;
        tincr += tincr2;
    }
}
*/

void VoIPSinkApp::Connection::writeLostSamples(int sampleCount)
{
    int pktBytes = sampleCount * sampleBits / 8;
    uint8_t decBuf[pktBytes];
    memset(decBuf, 0, pktBytes);
    outFile.write(decBuf, pktBytes);
}

void VoIPSinkApp::Connection::writeAudioFrame(uint8_t *inbuf, int inbytes)
{
   int decBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    int16_t *decBuf = new int16_t[decBufSize]; // output is 16bit
    int ret = avcodec_decode_audio2(DecCtx, decBuf, &decBufSize, inbuf, inbytes);
    if (ret < 0)
        throw cRuntimeError("avcodec_decode_audio2(): received packet decoding error: %d", ret);

    lastPacketFinish += simtime_t(1.0) * (decBufSize * 8 / sampleBits) / sampleRate;
    outFile.write(decBuf, decBufSize);
    delete[] decBuf;
}

void VoIPSinkApp::Connection::closeAudio()
{
    outFile.close();
}


bool VoIPSinkApp::createConnect(VoIPPacket *vp)
{
    if (!curConn.offline)
        return false;

    emit(connStateSignal, 1);

    curConn.offline = false;
    curConn.seqNo = vp->getSeqNo() - 1;
    curConn.timeStamp = vp->getTimeStamp();
    curConn.ssrc = vp->getSsrc();
    curConn.codec = (enum CodecID)(vp->getCodec());
    curConn.sampleBits = vp->getSampleBits();
    curConn.sampleRate = vp->getSampleRate();
    curConn.transmitBitrate = vp->getTransmitBitrate();
    curConn.samplesPerPackets = vp->getSamplesPerPackets();
    curConn.lastPacketFinish = simTime() + playOutDelay;

    curConn.DecCtx = avcodec_alloc_context();

    curConn.DecCtx->bit_rate = curConn.transmitBitrate;
    curConn.DecCtx->sample_rate = curConn.sampleRate;
    curConn.DecCtx->channels = 1;

    curConn.pCodecDec = avcodec_find_decoder(curConn.codec);
    if (curConn.pCodecDec == NULL)
        error("Codec %d not found!", curConn.codec);
    int ret = avcodec_open(curConn.DecCtx, curConn.pCodecDec);
    if (ret < 0)
        error("could not open decoding codec!");

    return curConn.outFile.open(resultFile, curConn.sampleRate, curConn.sampleBits);
}

bool VoIPSinkApp::checkConnect(VoIPPacket *vp)
{
    return  (!curConn.offline)
            && vp->getSsrc() == curConn.ssrc
            && vp->getCodec() == curConn.codec
            && vp->getSampleBits() == curConn.sampleBits
            && vp->getSampleRate() == curConn.sampleRate
            && vp->getSamplesPerPackets() == curConn.samplesPerPackets
            && vp->getTransmitBitrate() == curConn.transmitBitrate
            && vp->getSeqNo() > curConn.seqNo
            && vp->getTimeStamp() > curConn.timeStamp
            ;
}

void VoIPSinkApp::closeConnect()
{
    if (!curConn.offline)
    {
        curConn.offline = true;
        avcodec_close(curConn.DecCtx);
        //FIXME implementation: delete buffers, close output file if need
        curConn.outFile.close();
        emit(connStateSignal, 0);
    }
}

void VoIPSinkApp::handleVoIPMessage(VoIPPacket *vp)
{
    long int bytes = (long int)vp->getByteLength();
    bool ok = (curConn.offline) ? createConnect(vp) : checkConnect(vp);
    emit(ok ? receivedBytesSignal : droppedBytesSignal, bytes);

    if (ok)
        decodePacket(vp);

	delete vp;
}

void VoIPSinkApp::decodePacket(VoIPPacket *vp)
{
    switch(vp->getType())
    {
        case VOICE:
            emit(packetHasVoiceSignal, 1);
            break;

        case SILENT:
            emit(packetHasVoiceSignal, 0);
            break;

        default:
            error("The received VoIPPacket has unknown type:%d!", vp->getType());
            return;
    }
    uint16_t newSeqNo = vp->getSeqNo();
    if (newSeqNo > curConn.seqNo + 1)
        emit(lostPacketsSignal, newSeqNo - (curConn.seqNo + 1));
    if (simTime() > curConn.lastPacketFinish)
    {
        int lostSamples = (int)SIMTIME_DBL((simTime() - curConn.lastPacketFinish) * curConn.sampleRate);
        ev << "Lost " << lostSamples << " samples\n";
        emit(lostSamplesSignal, lostSamples);
        curConn.writeLostSamples(lostSamples);
        curConn.lastPacketFinish = simTime();
    }
    emit(delaySignal, curConn.lastPacketFinish - vp->getCreationTime());
    curConn.seqNo = newSeqNo;
    int len = vp->getDataArraySize();
    uint8_t buff[len];
    vp->copyDataToBuffer(buff, len);
    curConn.writeAudioFrame(buff, len);
}

void VoIPSinkApp::finish()
{
	ev << "Sink finish()" << endl;
	closeConnect();
}

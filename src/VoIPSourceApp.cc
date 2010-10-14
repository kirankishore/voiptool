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

#include "VoIPSourceApp.h"

Define_Module(VoIPSourceApp);


VoIPSourceApp::~VoIPSourceApp()
{
    if (timer.isScheduled())
        cancelEvent(&timer);
}

VoIPSourceApp::Buffer::Buffer() :
    samples(NULL),
    bufferSize(0),
    readOffset(0),
    writeOffset(0)
{
}

VoIPSourceApp::Buffer::~Buffer()
{
    delete[] samples;
}

void VoIPSourceApp::Buffer::clear(int framesize)
{
    delete samples;
    bufferSize = BUFSIZE + framesize;
    samples = new char[bufferSize];
    readOffset = 0;
    writeOffset = 0;
}

void VoIPSourceApp::initialize(int stage)
{
    UDPAppBase::initialize(stage);

    if(stage != 3)  //wait until stage 3 - The Address resolver does not work before that!
        return;

    // say HELLO to the world
    ev << "VoIPSourceApp -> initialize(" << stage << ")" << endl;

    localPort = par("localPort");
    destPort = par("destPort");
    destAddress = IPAddressResolver().resolve(par("destAddress").stringValue());

    voipHeaderSize = par("voipHeaderSize");
    voipSilenceThreshold = par("voipSilenceThreshold");
    sampleRate = par("sampleRate");
    bitsPerSample = par("bitsPerSample");
    switch(bitsPerSample)
    {
        case  8: sampleFormat = SAMPLE_FMT_U8;  break;
        case 16: sampleFormat = SAMPLE_FMT_S16; break;
        case 32: sampleFormat = SAMPLE_FMT_S32; break;
        default:
            error("Invalid bitsPerSample=%d parameter, valid values only 8, 16 or 32", bitsPerSample);
    }
    bytesPerSample = bitsPerSample/8;
    codec = par("codec").stringValue();
    compressedBitRate = par("compressedBitRate");
    packetTimeLength = par("packetTimeLength");

    soundFile = par("soundFile").stringValue();
    repeatCount = par("repeatCount");
    traceFileName = par("traceFileName").stringValue();
    if (traceFileName && *traceFileName)
        outFile.open(traceFileName, sampleRate, bitsPerSample);
    simtime_t start = par("start");

    samplesPerPacket = (int)round(sampleRate * SIMTIME_DBL(packetTimeLength));
    int bytesPerPacket = samplesPerPacket * bytesPerSample;
    if (bytesPerPacket & 1)
    {
        samplesPerPacket++;
        bytesPerPacket = samplesPerPacket * bytesPerSample;
    }
    ev << "The packetTimeLength parameter is " << packetTimeLength * 1000.0 << "ms, ";
    packetTimeLength = ((double)samplesPerPacket) / sampleRate;
    ev << "recalculated to " << packetTimeLength * 1000.0 << "ms!" << endl;

    sampleBuffer.clear(bytesPerPacket);

    // initialize avcodec library
    av_register_all();

    av_init_packet(&packet);

    openSoundFile(soundFile);

    scheduleAt(start, &timer);

    voipSilenceSize = voipHeaderSize;

    switch(bitsPerSample)
    {
        case  8: sampleFormat = SAMPLE_FMT_U8;  break;
        case 16: sampleFormat = SAMPLE_FMT_S16; break;
        case 32: sampleFormat = SAMPLE_FMT_S32; break;
        default:
            error("Invalid 'bitsPerSample='%d parameter", bitsPerSample);
    }
    // initialize the sequence number
    pktID = 1;
}

void VoIPSourceApp::handleMessage(cMessage *msg)
{
    // create an IP message
    VoIPPacket *packet;

    if (msg->isSelfMessage())
    {
        if (msg == &timer)
        {
            packet = generatePacket();
            if(!packet)
            {
                if (repeatCount > 1)
                {
                    repeatCount--;
                    av_seek_frame(pFormatCtx, streamIndex, 0, 0);
                    packet = generatePacket();
                }
            }
            if (packet)
            {
                // reschedule trigger message
                scheduleAt(simTime() + packetTimeLength, packet);
                scheduleAt(simTime() + packetTimeLength, msg);
            }
        }
        else
        {
            sendToUDP(PK(msg), localPort, destAddress, destPort);
        }

    }
    else
        delete msg;
}

void VoIPSourceApp::finish()
{
    av_free_packet(&packet);
    outFile.close();
    if (this->pReSampleCtx)
        audio_resample_close(pReSampleCtx);
    pReSampleCtx = NULL;

    if (this->pFormatCtx)
        av_close_input_file(pFormatCtx);
}


void VoIPSourceApp::openSoundFile(const char *name)
{
    int ret = av_open_input_file(&pFormatCtx, name, NULL, 0, NULL);
    if (ret)
        error("Audiofile '%s' open error: %d", name, ret);

    av_find_stream_info(pFormatCtx);

    //get stream number
    for(unsigned int j=0; j<pFormatCtx->nb_streams; j++)
    {
        if(pFormatCtx->streams[j]->codec->codec_type==CODEC_TYPE_AUDIO)
        {
            streamIndex = j;
            break;
        }
    }
    pCodecCtx = pFormatCtx->streams[streamIndex]->codec;

    //find decoder and open the correct codec
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    ret = avcodec_open(pCodecCtx, pCodec);
    if (ret)
        error("avcodec_open() error on file '%s': %d", name, ret);
    if(pCodecCtx->sample_rate != sampleRate || pCodecCtx->channels != 1)
    {
        pReSampleCtx = av_audio_resample_init(1, pCodecCtx->channels, sampleRate, pCodecCtx->sample_rate,
                sampleFormat, pCodecCtx->sample_fmt, 16, 10, 0, 0.8);
                // parameters copied from the implementation of deprecated audio_resample_init()
    }
    else
    {
        pReSampleCtx = NULL;
    }
    //allocate encoder
    pEncoderCtx = avcodec_alloc_context();
    //set bitrate:
    pEncoderCtx->bit_rate = compressedBitRate;

    pEncoderCtx->sample_rate = sampleRate;
    pEncoderCtx->channels = 1;

    pCodecEncoder = avcodec_find_encoder_by_name(codec);
    if (pCodecEncoder == NULL)
        error("Codec '%s' not found!", codec);

    if (avcodec_open(pEncoderCtx, pCodecEncoder) < 0)
        error("could not open %s encoding codec!", codec);
}

VoIPPacket* VoIPSourceApp::generatePacket()
{
    readFrame();
    if (sampleBuffer.empty())
        return NULL;

    int samples = std::min(sampleBuffer.length()/bytesPerSample, samplesPerPacket);
    bool isSilent = checkSilence(sampleBuffer.readPtr(), samples);
    VoIPPacket *vp = new VoIPPacket();
    int encoderBufSize = (int)(compressedBitRate * SIMTIME_DBL(packetTimeLength))/8+256;
    uint8_t encoderBuf[encoderBufSize];
    memset(encoderBuf, 0, encoderBufSize);
    pEncoderCtx->frame_size = samples;

    // the 3rd parameter of avcodec_encode_audio() is the size of INPUT buffer!!!
    // It's wrong in the FFMPEG documentation/header file!!!
    encoderBufSize = samples;
    int encSize = avcodec_encode_audio(pEncoderCtx, encoderBuf, encoderBufSize, (short int*)(sampleBuffer.readPtr()));
    if (encSize <= 0)
        error("avcodec_encode_audio() error: %d", encSize);
    outFile.write(sampleBuffer.readPtr(), samples * bytesPerSample);

    vp->setDataFromBuffer(encoderBuf, encSize);

    if (isSilent)
    {
        vp->setName("SILENCE");
        vp->setType(SILENCE);
        vp->setByteLength(voipHeaderSize);
    }
    else
    {

        vp->setName("VOICE");
        vp->setType(VOICE);
        vp->setByteLength(voipHeaderSize + encSize);
    }
    vp->setTimeStamp(pktID);
    vp->setSeqNo(pktID);
    vp->setCodec(pEncoderCtx->codec_id);
    vp->setSampleRate(sampleRate);
    vp->setSampleBits(bitsPerSample);
    vp->setSamplesPerPacket(samplesPerPacket);
    vp->setTransmitBitrate(compressedBitRate);

    pktID++;
    sampleBuffer.notifyRead(samples * bytesPerSample);

    return vp;
}

bool VoIPSourceApp::checkSilence(void* _buf, int samples)
{
    int max = 0;
    int i;

    switch(sampleFormat)
    {
    case SAMPLE_FMT_U8:
        {
            uint8_t *buf = (uint8_t *)_buf;
            for (i=0; i<samples; ++i)
            {
                int s = abs(int(buf[i]) - 0x80);
                if (s > max)
                    max = s;
            }
        }
        break;

    case SAMPLE_FMT_S16:
        {
            int16_t *buf = (int16_t *)_buf;
            for (i=0; i<samples; ++i)
            {
                int s = abs(buf[i]);
                if (s > max)
                    max = s;
            }
        }
        break;

    case SAMPLE_FMT_S32:
        {
            int32_t *buf = (int32_t *)_buf;
            for (i=0; i<samples; ++i)
            {
                int s = abs(buf[i]);
                if (s > max)
                    max = s;
            }
        }
        break;

    default:
        error("invalid sampleFormat:%d", sampleFormat);
    }
    return max < voipSilenceThreshold;
}

void VoIPSourceApp::Buffer::align()
{
    if (readOffset)
    {
        if (length())
            memcpy(samples, samples+readOffset, length());
        writeOffset -= readOffset;
        readOffset = 0;
    }
}

void VoIPSourceApp::readFrame()
{
    if (sampleBuffer.length() >= samplesPerPacket * bytesPerSample)
        return;

    sampleBuffer.align();

    char *tmpSamples = NULL;
    if(pReSampleCtx)
    {
        tmpSamples = new char[Buffer::BUFSIZE];
    }
    while(sampleBuffer.length() < samplesPerPacket * bytesPerSample)
    {
        //read one frame
        int err = av_read_frame(pFormatCtx, &packet);
        if (err < 0)
            break;

        // if the frame doesn't belong to our audiostream, continue... is not supposed to happen,
        // since .wav contain only one media stream
        if (packet.stream_index != streamIndex)
            continue;

        // packet length == 0 ? read next packet
        if (packet.size == 0)
            continue;

        uint8_t *ptr = packet.data;
        int len = packet.size;

        while (len > 0)
        {
            // decode audio and save the decoded samples in our buffer
            int16_t *rbuf, *nbuf;
            nbuf = (int16_t*)(sampleBuffer.writePtr());
            rbuf = (pReSampleCtx) ? (int16_t*)tmpSamples : nbuf;

            int frame_size = (pReSampleCtx) ? Buffer::BUFSIZE : sampleBuffer.availableSpace();
            memset(rbuf, 0, frame_size);
            int decoded = avcodec_decode_audio2(pCodecCtx, rbuf, &frame_size, ptr, len);
            if (decoded < 0)
                error("Error decoding frame, err=%d", decoded);

            ptr += decoded;
            len -= decoded;

            if (frame_size == 0)
                continue;

            decoded = frame_size / (bytesPerSample * pCodecCtx->channels);
            ASSERT(frame_size == decoded * bytesPerSample * pCodecCtx->channels);
            if (pReSampleCtx)
            {
                decoded = audio_resample(pReSampleCtx, nbuf, rbuf, decoded);
            }
            sampleBuffer.notifyWrote(decoded * bytesPerSample);
        }
    }
    if(pReSampleCtx)
        delete[] tmpSamples;
}

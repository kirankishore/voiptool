
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/**************************************************************************
             TrafficGenerator.cc  -  simple traffic generator
                             -------------------
    begin                : Wed Jul 13 2005
    copyright            : (C) 2005 by M. Bohge
    email                : bohge@tkn.tu-berlin.de
    last modified        : added VoIP wav-tracinng - by M. Renwanz   
 ***************************************************************************/

// TODO: Zeile 310 - berechnung der Position korrekt ?

#include "VoIPGenerator.h"


Define_Module(VoIPGenerator);

void VoIPGenerator::initialize(int stage)
{
    UDPAppBase::initialize(stage);

    if(stage != 3)  //wait until stage 3 - The Address resolver does not work before that!
        return;

    // say HELLO to the world
    ev << "VoIPGenerator -> initialize(" << stage << ")" << endl;

    localPort = par("localPort");
    destPort = par("destPort");
    destAddress = IPAddressResolver().resolve(par("destAddress").stringValue());

    voipHeaderSize = par("voipHeaderSize");
    voipSilenceThreshold = par("voipSilenceThreshold");
    sampleRate = par("sampleRate");
    sampleBits = par("sampleBits");
    switch(sampleBits)
    {
        case  8: sampleFormat = SAMPLE_FMT_U8;  break;
        case 16: sampleFormat = SAMPLE_FMT_S16; break;
        case 32: sampleFormat = SAMPLE_FMT_S32; break;
        default:
            error("Invalid sampleBits=%d parameter, valid values only 8, 16 or 32", sampleBits);
    }
    sampleBytes = sampleBits/8;
    codec = par("codec").stringValue();
    compressedBitRate = par("compressedBitRate");
    packetTimeLength = par("packetTimeLength");

    soundFile = par("soundFile").stringValue();
    repeatCount = par("repeatCount");
    traceFileName = par("traceFileName").stringValue();
    simtime_t start = par("start");

    samplesPerPacket = (int)round(sampleRate * SIMTIME_DBL(packetTimeLength));
    int bytesPerPacket = samplesPerPacket * sampleBytes;
    if (bytesPerPacket & 1)
        samplesPerPacket++;

    ev << "The packetTimeLength parameter is " << packetTimeLength * 1000.0 << "ms, ";
    packetTimeLength = ((double)samplesPerPacket) / sampleRate;
    ev << "recalculated to " << packetTimeLength * 1000.0 << "ms!" << endl;

    samplePtr = NULL;
    // Buffer for audio samples
    samples = new char[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    newSamples = new char[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    samplePtr = newSamples;

    // initialize avcodec library
    av_register_all();

    openSoundFile(soundFile);
    //////////////////////////////////////////////////////////////////////////////////
    scheduleAt(start, &timer);
    //////////////////////////////////////////////////////////////////////////////////

    voipSilenceSize = voipHeaderSize;

    switch(sampleBits)
    {
        case  8: sampleFormat = SAMPLE_FMT_U8;  break;
        case 16: sampleFormat = SAMPLE_FMT_S16; break;
        case 32: sampleFormat = SAMPLE_FMT_S32; break;
        default:
            error("Invalid 'sampleBits='%d parameter", sampleBits);
    }
    // initialize the sequence number
    pktID = 1;
}

void VoIPGenerator::handleMessage(cMessage *msg)
{
    // create an IP message
    VoIPPacket *packet;

    if (msg->isSelfMessage())
    {
        packet = generatePacket();
        if(!packet)
        {
            if (repeatCount > 0)
            {
                repeatCount--;
                av_seek_frame(pFormatCtx, streamIndex, 0, 0);
                packet = generatePacket();
            }
        }
        if (packet)
        {
            sendToUDP(packet, localPort, destAddress, destPort);

            // reschedule trigger message
            scheduleAt(simTime() + packetTimeLength, msg);
        }

    }
    else
        delete msg;
}

void VoIPGenerator::finish()
{
}


void VoIPGenerator::openSoundFile(const char *name)
{
    av_open_input_file(&pFormatCtx, name, NULL, 0, NULL);
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
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    avcodec_open(pCodecCtx, pCodec);
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

VoIPPacket* VoIPGenerator::generatePacket()
{
    readFrame();
    if (unreadSamples == 0)
        return NULL;

    int samples = std::min(unreadSamples, samplesPerPacket);
    if (0 == samples)
        return NULL;

    int samplesBytes = samples * (sampleBytes);
    bool isSilent = checkSilence(samplePtr, samplesBytes);
    VoIPPacket *vp = new VoIPPacket();
    int encoderBufSize = (int)(compressedBitRate * SIMTIME_DBL(packetTimeLength))/8+256;
    uint8_t encoderBuf[encoderBufSize];
    pEncoderCtx->frame_size = samples;
    int encSize = avcodec_encode_audio(pEncoderCtx, encoderBuf, encoderBufSize, (short int*)samplePtr);
    if (encSize <= 0)
        error("avcodec_encode_audio() error: %d", encSize);

    vp->setDataFromBuffer(encoderBuf, encSize);

    if (isSilent)
    {
        vp->setName("SILENT");
        vp->setType(SILENT);
        vp->setByteLength(voipHeaderSize);
    }
    else
    {

        vp->setName("VOICE");
        vp->setType(VOICE);
        vp->setByteLength(voipHeaderSize + encSize);
    }
    vp->setSeqNo(pktID++);
    vp->setCodec(pEncoderCtx->codec_id);
    vp->setSampleRate(sampleRate);
    vp->setSampleBits(sampleBits);

    samplePtr += samplesBytes;
    unreadSamples -= samples;

    return vp;
}

bool VoIPGenerator::checkSilence(void* _buf, int samples)
{
    int max = 0;
    int i;

    switch(sampleFormat)
    {
    case SAMPLE_FMT_U8:
        {
            uint8_t *buf = (uint8_t *)_buf;
            for (i=0; i<samples; ++i)
                if (abs(buf[i]) > max)
                    max = abs(buf[i]);
        }
        break;

    case SAMPLE_FMT_S16:
        {
            int16_t *buf = (int16_t *)_buf;
            for (i=0; i<samples; ++i)
                if (abs(buf[i]) > max)
                    max = abs(buf[i]);
        }
        break;

    case SAMPLE_FMT_S32:
        {
            int32_t *buf = (int32_t *)_buf;
            for (i=0; i<samples; ++i)
                if (abs(buf[i]) > max)
                    max = abs(buf[i]);
        }
        break;

    default:
        error("invalid sampleFormat:%d", sampleFormat);
    }
    return max < voipSilenceThreshold;
}

void VoIPGenerator::readFrame()
{
    if (unreadSamples >= samplesPerPacket)
        return;

    if (unreadSamples)
        memcpy(newSamples, samplePtr, unreadSamples * sampleBytes);
    samplePtr = newSamples;

    AVPacket packet;

    for (;;)
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
        if (packet.duration == 0)
            continue;

        // decode audio and save the decoded samples in our buffer
        int frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        int16_t *rbuf, *nbuf;
        nbuf = (int16_t*)(newSamples + unreadSamples * sampleBytes);
        rbuf = (pReSampleCtx) ? (int16_t*)samples : nbuf;

        frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        int decoded = avcodec_decode_audio2(pCodecCtx, rbuf, &frame_size, packet.data, packet.size);
        if (decoded < 0)
            error("Error decoding frame, err=%d", decoded);

        if (frame_size == 0)
            continue;

        if (decoded != packet.size)
            error("Error decoding frame, not decoded the all samples of the frame (%d < %d)", decoded, packet.size);

        decoded = frame_size / sampleBytes;
        if (pReSampleCtx)
        {
            decoded = audio_resample(pReSampleCtx, nbuf, rbuf, decoded);
        }

        unreadSamples += decoded;

        av_free_packet(&packet);

        // number of packets in this frame, one sample = 2 bytes
        if (unreadSamples >= samplesPerPacket)
            break;
    }
}

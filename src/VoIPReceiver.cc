
#include "VoIPReceiver.h"

#include "INETEndians.h"

#define INT64_C(x) int64_t(x)

Define_Module(VoIPReceiver);

simsignal_t VoIPReceiver::receivedBytes = SIMSIGNAL_NULL;
simsignal_t VoIPReceiver::missingPackets = SIMSIGNAL_NULL;
simsignal_t VoIPReceiver::droppedBytes = SIMSIGNAL_NULL;
simsignal_t VoIPReceiver::packetHasVoice = SIMSIGNAL_NULL;
simsignal_t VoIPReceiver::connState = SIMSIGNAL_NULL;

VoIPReceiver::~VoIPReceiver()
{
    if (timer)
        delete cancelEvent(timer);
}

void VoIPReceiver::initialiseStatics()
{
    if (receivedBytes != SIMSIGNAL_NULL)
        return;

    receivedBytes = registerSignal("receivedBytes");
    missingPackets = registerSignal("missingPackets");
    droppedBytes  = registerSignal("droppedBytes");
    packetHasVoice = registerSignal("packetHasVoice");
    connState = registerSignal("connState");
}

void VoIPReceiver::initialize()
{
    UDPAppBase::initialize();
    initialiseStatics();

    // Say Hello to the world
	ev << "VoIPReceiver initialize()" << endl;

	timer = new cMessage("TIMEOUT");

	//read in omnet parameters
	localPort = par("localPort");
	timeout = par("timeout");
	resultFile = par("resultFile");
	
	//initialize avcodec library
	av_register_all();
	
	bindToPort(localPort);
}

void VoIPReceiver::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        // msg == &timer
        if (!curConn.offline)
            closeConnect();
    }
    else
    {
        VoIPPacket *vp = dynamic_cast<VoIPPacket *>(msg);
        if(vp)
            handleVoIPMessage(vp);
        else
            delete msg;
    }
}

/*
 * add an audio output stream
 */
void VoIPReceiver::Connection::addAudioStream(enum CodecID codec_id)
{
    AVStream *st = av_new_stream(oc, 1);
    if (!st)
        throw cRuntimeError("Could not alloc stream\n");

    AVCodecContext *c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = CODEC_TYPE_AUDIO;

    /* put sample parameters */
    c->bit_rate = sampleRate * 16; //FIXME what is valid multiplier?
    c->sample_rate = sampleRate;
    c->channels = 1;
    audio_st = st;
}

void VoIPReceiver::Connection::openAudio()
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
void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
/*    int j, i, v;
    int16_t *q;

    q = samples;
    for(j=0;j<frame_size;j++) {
        v = (int)(sin(t) * 10000);
        for(i = 0; i < nb_channels; i++)
            *q++ = v;
        t += tincr;
        tincr += tincr2;
    }
*/
}

void VoIPReceiver::Connection::writeLostFrames(int frameCount)
{
    if (pktBytes <= 0)
        return;

    AVCodecContext *c;
    c = audio_st->codec;
    uint8_t outbuf[pktBytes];
    uint8_t decBuf[pktBytes];
    memset(decBuf, 0, pktBytes);
    AVPacket pkt;
    av_init_packet(&pkt);

    for ( ; frameCount > 0 && pktBytes > 0; frameCount--)
    {

        // the 3rd parameter of avcodec_encode_audio() is the size of INPUT buffer!!!
        // It's wrong in the FFMPEG documentation/header file!!!
        pkt.size = avcodec_encode_audio(c, outbuf, pktBytes, (short int*)decBuf);
        if (c->coded_frame->pts != AV_NOPTS_VALUE)
            pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, audio_st->time_base);
        pkt.flags |= PKT_FLAG_KEY;
        pkt.stream_index= audio_st->index;
        pkt.data = outbuf;

        // write the compressed frame in the media file
        if (av_interleaved_write_frame(oc, &pkt) != 0)
            throw cRuntimeError("Error while writing audio frame\n");
    }
}

void VoIPReceiver::Connection::writeAudioFrame(uint8_t *inbuf, int inbytes)
{
    AVCodecContext *c;
    AVPacket pkt;
    av_init_packet(&pkt);

    int decBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    int16_t *decBuf = new int16_t[decBufSize]; // output is 16bit
    int ret = avcodec_decode_audio2(DecCtx, decBuf, &decBufSize, inbuf, inbytes);
    if (ret < 0)
        throw cRuntimeError("avcodec_decode_audio2(): received packet decoding error: %d", ret);

    c = audio_st->codec;

//    get_audio_frame(inbuf, audio_input_frame_size, c->channels);

    int outbufSize = decBufSize;
    if (pktBytes < decBufSize)
        pktBytes = decBufSize;
    uint8_t outbuf[outbufSize];
    // the 3rd parameter of avcodec_encode_audio() is the size of INPUT buffer!!!
    // It's wrong in the FFMPEG documentation/header file!!!
    pkt.size = avcodec_encode_audio(c, outbuf, outbufSize, decBuf);
    if (c->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, audio_st->time_base);
    pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index= audio_st->index;
    pkt.data = outbuf;

    // write the compressed frame in the media file
    if (av_interleaved_write_frame(oc, &pkt) != 0)
        throw cRuntimeError("Error while writing audio frame\n");
    delete[] decBuf;
}

void VoIPReceiver::Connection::closeAudio()
{
    avcodec_close(audio_st->codec);
//    av_free(samples);
//    av_free(audio_outbuf);
}


bool VoIPReceiver::createConnect(VoIPPacket *vp)
{
    if (!curConn.offline)
        return false;

    curConn.offline = false;
    curConn.seqNo = vp->getSeqNo() - 1;
    curConn.timeStamp = vp->getTimeStamp();
    curConn.ssrc = vp->getSsrc();
    curConn.codec = (enum CodecID)(vp->getCodec());
    curConn.sampleBits = vp->getSampleBits();
    curConn.sampleRate = vp->getSampleRate();
    curConn.pktBytes = 0;

    curConn.DecCtx = avcodec_alloc_context();

    curConn.DecCtx->bit_rate = curConn.sampleBits;
    curConn.DecCtx->bit_rate = 40000;
    curConn.DecCtx->sample_rate = curConn.sampleRate;
    curConn.DecCtx->channels = 1;

    curConn.pCodecDec = avcodec_find_decoder(curConn.codec);
    if (curConn.pCodecDec == NULL)
        error("Codec %d not found!", curConn.codec);
    int ret = avcodec_open(curConn.DecCtx, curConn.pCodecDec);
    if (ret < 0)
        error("could not open decoding codec!");

    AVOutputFormat *fmt;
    // auto detect the output format from the name. default is WAV
    fmt = guess_format(NULL, resultFile, NULL);
    if (!fmt)
    {
        ev << "Could not deduce output format from file extension: using WAV.\n";
        fmt = guess_format("wav", NULL, NULL);
    }
    if (!fmt)
    {
        error("Could not find suitable output format fro filename '%s'\n", resultFile);
    }

    // allocate the output media context
    curConn.oc = avformat_alloc_context();
    if (!curConn.oc)
        error("Memory error at avformat_alloc_context()\n");

    curConn.oc->oformat = fmt;
    snprintf(curConn.oc->filename, sizeof(curConn.oc->filename), "%s", resultFile);

    // add the audio stream using the default format codecs and initialize the codecs
    curConn.audio_st = NULL;
    if (fmt->audio_codec != CODEC_ID_NONE)
        curConn.addAudioStream(fmt->audio_codec);

    // set the output parameters (must be done even if no parameters).
    if (av_set_parameters(curConn.oc, NULL) < 0)
        error("Invalid output format parameters\n");

    dump_format(curConn.oc, 0, resultFile, 1);

    /* now that all the parameters are set, we can open the audio and
       video codecs and allocate the necessary encode buffers */
    if (curConn.audio_st)
        curConn.openAudio();

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE))
    {
        if (url_fopen(&curConn.oc->pb, resultFile, URL_WRONLY) < 0)
            error("Could not open '%s'\n", resultFile);
    }

    // write the stream header
    av_write_header(curConn.oc);

    return true;
}

bool VoIPReceiver::checkConnect(VoIPPacket *vp)
{
    return  (!curConn.offline)
            && vp->getSsrc() == curConn.ssrc
            && vp->getCodec() == curConn.codec
            && vp->getSampleBits() == curConn.sampleBits
            && vp->getSampleRate() == curConn.sampleRate
            && vp->getSeqNo() > curConn.seqNo
            && vp->getTimeStamp() > curConn.timeStamp
            ;
}

void VoIPReceiver::closeConnect()
{
    if (!curConn.offline)
    {
        avcodec_close(curConn.DecCtx);
        //FIXME implementation: delete buffers, close output file if need
        curConn.offline = true;

        /* write the trailer, if any.  the trailer must be written
         * before you close the CodecContexts open when you wrote the
         * header; otherwise write_trailer may try to use memory that
         * was freed on av_codec_close() */
        av_write_trailer(curConn.oc);

        /* close each codec */
        if (curConn.audio_st)
            curConn.closeAudio();

        /* free the streams */
        for(int i = 0; i < curConn.oc->nb_streams; i++)
        {
            av_freep(&curConn.oc->streams[i]->codec);
            av_freep(&curConn.oc->streams[i]);
        }

        if (!(curConn.oc->oformat->flags & AVFMT_NOFILE))
        {
            /* close the output file */
            url_fclose(curConn.oc->pb);
        }

        /* free the stream */
        av_free(curConn.oc);
        av_free(curConn.DecCtx);
    }
}

void VoIPReceiver::handleVoIPMessage(VoIPPacket *vp)
{
    bool ok = (curConn.offline) ? createConnect(vp) : checkConnect(vp);
    if(!ok)
    {
        emit(droppedBytes, (long int)vp->getByteLength());
        delete vp;
        return;
    }

    if (timer->isScheduled())
        cancelEvent(timer);
    scheduleAt(simTime() + timeout, timer);
    decodePacket(vp);

	delete vp;
}

void VoIPReceiver::decodePacket(VoIPPacket *vp)
{
    switch(vp->getType())
    {
        case VOICE:
        case SILENT:
            ev << "VoIP Packet received!" << endl;
            break;

        default:
            error("The received VoIPPacket has unknown type:%d!", vp->getType());
            return;
    }
    uint16_t newSeqNo = vp->getSeqNo();
    int lostPackets = newSeqNo - (curConn.seqNo + 1);
    if (newSeqNo > curConn.seqNo + 1)
    {
        ev << "Lost " << lostPackets << " packet(s)\n";
        emit(missingPackets, lostPackets);
        curConn.writeLostFrames(lostPackets);
    }
    int len = vp->getDataArraySize();
    uint8_t buff[len];
    vp->copyDataToBuffer(buff, len);
    curConn.writeAudioFrame(buff, len);
}

void VoIPReceiver::finish()
{
	struct stat statbuf;
	ev << "Sink finish()" << endl;
	char command[1000];
	const char *last;
	int len;
	closeConnect();
//	sprintf(command, "[Run %s %d]\n", ev.getConfigEx()->getActiveConfigName(), ev.getConfigEx()->getActiveRunNumber());
//	sprintf(command, "total number of VoIP packets:\t%d\n", pktno);
//	sprintf(command, "number of transmission errors:\t%d\n", transmissionErrors);
//	sprintf(command, "number of silence packets:\t%d\n", numberOfVoIpSilence);
//	delete[] samples; samples = NULL;
//	delete[] g726buf; g726buf = NULL;
//	fclose(result);
}

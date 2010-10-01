
#include "VoIPReceiver.h"

#include "INETEndians.h"

#define INT64_C(x) int64_t(x)

Define_Module(VoIPReceiver);

simsignal_t VoIPReceiver::receivedBytes = SIMSIGNAL_NULL;
simsignal_t VoIPReceiver::missingPackets = SIMSIGNAL_NULL;
simsignal_t VoIPReceiver::droppedBytes = SIMSIGNAL_NULL;
simsignal_t VoIPReceiver::packetHasVoice = SIMSIGNAL_NULL;
simsignal_t VoIPReceiver::connState = SIMSIGNAL_NULL;

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

//    audio_outbuf_size = 10000;
//    audio_outbuf = av_malloc(audio_outbuf_size);

    /* ugly hack for PCM codecs (will be removed ASAP with new PCM
       support to compute the input frame size in samples */
/*

    if (c->frame_size <= 1)
    {
        audio_input_frame_size = audio_outbuf_size / c->channels;
        switch(audio_st->codec->codec_id)
        {
        case CODEC_ID_PCM_S16LE:
        case CODEC_ID_PCM_S16BE:
        case CODEC_ID_PCM_U16LE:
        case CODEC_ID_PCM_U16BE:
            audio_input_frame_size >>= 1;
            break;
        default:
            break;
        }
    } else {
        audio_input_frame_size = c->frame_size;
    }
*/
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

void VoIPReceiver::Connection::writeAudioFrame(uint8_t *inbuf, int inbytes)
{
    AVCodecContext *c;
    AVPacket pkt;
    av_init_packet(&pkt);

    int decBufSize = sampleRate * (sampleBits/8);
    int16_t decBuf[decBufSize/2]; // output is 16bit
    int ret = avcodec_decode_audio2(DecCtx, decBuf, &decBufSize, inbuf, inbytes);
    if (ret < 0)
        throw cRuntimeError("avcodec_decode_audio2(): received packet decoding error: %d", ret);

    c = audio_st->codec;

//    get_audio_frame(inbuf, audio_input_frame_size, c->channels);

    int outbufSize = decBufSize; //FIXME mennyi legyen?
    uint8_t outbuf[outbufSize];
    pkt.size = avcodec_encode_audio(c, outbuf, outbufSize, decBuf);
    if (c->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, audio_st->time_base);
    pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index= audio_st->index;
    pkt.data = outbuf;

    // write the compressed frame in the media file
    if (av_interleaved_write_frame(oc, &pkt) != 0)
        throw cRuntimeError("Error while writing audio frame\n");
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
    curConn.seqNo = vp->getSeqNo();
    curConn.timeStamp = vp->getTimeStamp();
    curConn.ssrc = vp->getSsrc();
    curConn.codec = (enum CodecID)(vp->getCodec());
    curConn.sampleBits = vp->getSampleBits();
    curConn.sampleRate = vp->getSampleRate();

    curConn.DecCtx = avcodec_alloc_context();
/*
*/
    curConn.DecCtx->bit_rate = curConn.sampleBits;
    curConn.DecCtx->bit_rate = 40000;
    curConn.DecCtx->sample_rate = curConn.sampleRate;
    curConn.DecCtx->channels = 1;
//    curConn.DecCtx->bit_rate = curConn.sampleBits;

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

    if (timer.isScheduled())
        cancelEvent(&timer);
    scheduleAt(simTime() + timeout, &timer);
    decodePacket(vp);

	delete vp;
}

void VoIPReceiver::decodePacket(VoIPPacket *vp)
{
    switch(vp->getType())
    {
        case VOICE:
        case SILENT:
            ev << "Sink: VoIP Packet received!" << endl;
/*
            if(strcmp(cur_file, pkt->getWaveFile()) != 0)
            {
                cerr << "Changing audio file!" << endl;
                avcodec_close(pCodecCtx);
                av_close_input_file(pFormatCtx);
                unreadSamples = 0;
                psamples = 0;
                startPos = -1;
                initializeAudio();
            }
            if(unreadSamples < samplesPerPacket)
            {
                if(readNextFrame() == -1)
                {
                    cerr << cur_file << endl;
                    cerr << pkt->getWaveFile() << endl;
                    pkt = traceList->getPacket(pktno++);
                    cerr << pkt->getWaveFile() << endl;
                    error("readNextFrame failed - canceling simulation!");
                }
            }
            //write to original wav file the unaltered packet
            fwrite(&samples[psamples], 2, samplesPerPacket, original);
            if(vp->hasBitError())
            {
                ev << "Errorflag: 1!" << endl;
                transmissionErrors++;
                pkt->setBitErrorRate(true);
                // in case of an transmission error, silence is inserted into the output. There might be
                // smarter algorithms to hide those errors, but this is not part of this simple demonstration
                for(int i=psamples; i<(psamples+samplesPerPacket); i++)
                    samples[i]=0;
            }
            pkt->setPacketNo(pktno);
            pkt->setArrivalTime(simTime());
            if(vp->getType() == SILENT)
            {
                //silence packet, insert silence!
                numberOfVoIpSilence++;
                for(int i=psamples; i<(psamples+samplesPerPacket); i++)
                    samples[i]=0;
            }
            encodeNextPacket();
  */
            break;

        default:
            ev << "Sink: Unknown Packet received!" << endl;
            return;
    }
    int len = vp->getDataArraySize();
    uint8_t buff[len];
    vp->copyDataToBuffer(buff, len);
    curConn.writeAudioFrame(buff, len);
}

/*
void VoIPReceiver::encodeNextPacket()
{
	int g726_size, pcm_size;
	int16_t *newSamples;

	newSamples = new int16_t[2 * samplesPerPacket]; // doppelt h�lt besser ;)
    pcm_size = sizeof(int16_t) * samplesPerPacket;
	// at this point, the transmission errors and silence packets have been inserted into the samples buffer
	// encode it to G.726 !
	g726_size = avcodec_encode_audio(p726EncCtx, g726buf, samplesPerPacket, &samples[psamples]);
	psamples += samplesPerPacket;
	unreadSamples -= samplesPerPacket;
	//and decode it back to PCM wave
	avcodec_decode_audio2(p726DecCtx, newSamples, &pcm_size, g726buf, g726_size);
	// pcm_size: output size in bytes!
	pcm_size = pcm_size / 2;
	// write degenerated audio data
	ASSERT(pcm_size <= 2*samplesPerPacket);
	fwrite(newSamples, 2, pcm_size, degenerated);
}
 */
/*
int VoIPReceiver::readNextFrame()
{
	int frame_size, new_frame_size;
	int i;
	int16_t *newSamples, *resamples;
	newSamples = new int16_t[4000];

	for(; true; )
	{
        if(av_read_frame(pFormatCtx, &packet) < 0)              // if that is the case, eof is reached
            return -1;
        if(packet.duration == 0)   // this can actually happen - libavcodec reads the ID3v2 Header as an audio packet with duration 0...
        {
            av_free_packet(&packet);
            continue;
        }
        if(packet.stream_index != audiostream)
        {
            av_free_packet(&packet);
            continue;
        }
        // decode audio frame - frame_size is set to the number of bytes which have been written to newSamples
        frame_size = sizeof(int16_t)*4000;
        avcodec_decode_audio2(pCodecCtx, newSamples, &frame_size, packet.data, packet.size);
        // this should NOT happen... i'll check it for safety reasons
        if(frame_size > 0)
            break;

        av_free_packet(&packet);
	}
	frame_size = frame_size / 2; // convert from bytes to samples
	
	if(startPos == 0)
    // the last frame was positioned at the start of the buffer; in case that there are leftover samples
    // which were to small for a voip packet, we must place the next frame behind the last leftover - samples
	{
		resamples = &(samples[psamples+unreadSamples]);
		startPos = psamples;
	} else {
		// copy the leftover samples, if any, to the start of the buffer (since the startposition was not at 0)
		for(i=0; i < unreadSamples; i++)
			samples[i] = samples[psamples+i];
		psamples = 0;
		resamples = &(samples[unreadSamples]);
		startPos = 0;
	}

	if(pCodecCtx->channels == 2) // more than 2 channels will not be considered here
	{
		// convert to mono
		for (i=0; i<frame_size/2; i++)
		    newSamples[i] = (newSamples[2*i] + newSamples[2*i+1]) / 2;
		frame_size = frame_size / 2;
	}
	if(resample)
	{
		// the return value of audio_resample is in samples, not in bytes
		new_frame_size = audio_resample(pReSampleCtx, resamples, newSamples, frame_size);
		unreadSamples += new_frame_size;
	} else {
		//resampling is disabled - input file is already in correct sample format
		for(i=0; i<frame_size; i++)
		    resamples[i] = newSamples[i];
		unreadSamples += frame_size;
	}
	delete[] newSamples;
	av_free_packet(&packet);
	return 0;
}
*/
/*
void VoIPReceiver::writeFakeWavHeader(const char *filename)
{
    struct RiffWaveHeader
    {
        char riffTxt[4];        // "RIFF"
        uint32_t filesize;      // remaining filesize (= filesize-8)
        char waveTxt[4];        // "WAVE"
        char fmtTxt[4];         // "fmt "
        uint32_t headerlength;  // (length of the remaining header = 16 bytes)
        uint16_t formatTag;
        uint16_t channels;
        uint32_t sampleRate;
        uint32_t bytesPerSec;   // (sampleRate * sampleSize)
        uint16_t sampleSize;    // (channels * bitsPerSample / 8)
        uint16_t bitsPerSample; // 8, 16, 24
        char dataTxt[4];        // "data"
        uint32_t dataLength;
    };

    static const RiffWaveHeader riffWaveHeader =
    {
        {'R','I','F','F'},
        htole32(0),
        {'W','A','V','E'},
        {'f','m','t',' '},
        htole32(16),                    // length of remaining header
        htole16(1),                     // ID (PCM wave)
        htole16(1),                     // num. of channels
        htole32(G726_SAMPLERATE),       // sampleRate
        htole32(sizeof(int16) * G726_SAMPLERATE),   // bytesPerSec
        htole16(sizeof(int16)),         // sampleSize
        htole16(sizeof(int16) * 8),     // bitsPerSample
        {'d','a','t','a'},
        htole32(0)
    };

    FILE *fp;
	fp = fopen(filename, "wb");
	if(fp == NULL)
	    return;

	fwrite(&riffWaveHeader, 1, sizeof(riffWaveHeader), fp);
	fclose(fp);
}
*/
/*
void VoIPReceiver::initializeAudio()
{
	// open input file
	if(av_open_input_file(&pFormatCtx, cur_file, NULL, 0, NULL)!=0)
	{
		error("Open of audio file failed!!\n");
	}

	// detect file format
	if(av_find_stream_info(pFormatCtx)<0)
	{
		error("Invalid audio file!\n");
	}
	
	//search for audio stream (important with file formats which support several streams, like .avi for example)
	for (unsigned int i=0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
        {
            audiostream = i;
            break;
        }
	}
	if (audiostream == -1)
	{
		error("No audiostream found - invalid file!");
	}

	pCodecCtx = pFormatCtx->streams[audiostream]->codec;
	//find decoder for input file in library
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		error("No codec to decode input file found!");
	}

	if (avcodec_open(pCodecCtx, pCodec) < 0)
	{
		error("opening the correct codec failed!");
	}
	
	//allocate g726 encoder and decoder
	p726EncCtx = avcodec_alloc_context();
	//valid bitrates: 16000 bits/s, 32000 bits/s, 40000 bits/s
	p726EncCtx->bit_rate = codingRate;

	// G.726 supports only one channel with a sampling frequency of 8000 Hz !
	// Thus, any input files, which differ from that must be resampled!
	p726EncCtx->sample_rate = G726_SAMPLERATE;
	p726EncCtx->channels = 1;
	p726DecCtx = avcodec_alloc_context();
	p726DecCtx->bit_rate = codingRate;
	p726DecCtx->sample_rate = G726_SAMPLERATE;
	p726DecCtx->channels = 1;
	
	//search vor encoder and decoder codec in library
	pCodec726Enc = avcodec_find_encoder(CODEC_ID_ADPCM_G726);
	pCodec726Dec = avcodec_find_decoder(CODEC_ID_ADPCM_G726);
	if ((pCodec726Enc == NULL) || (pCodec726Dec == NULL))
		error("G.726 Codec not found!");

	if (avcodec_open(p726EncCtx, pCodec726Enc) < 0)
		error("could not open G.726 encoding codec!");

	if (avcodec_open(p726DecCtx, pCodec726Dec) < 0)
		error("could not open G.726 decoding codec!");
	
	if (pCodecCtx->sample_rate != G726_SAMPLERATE)
	{
		//sampling rate is not 8000 Hz, we must resample!
		resample = true;
		// if the file has more than one channel, we'll just take the average above all channels - this is not done with the ReSampleContext.
		// initialize resample context
        pReSampleCtx = av_audio_resample_init(1, 1, G726_SAMPLERATE, pCodecCtx->sample_rate,
                SAMPLE_FMT_S16, pCodecCtx->sample_fmt, 16, 10, 0, 0.8);
                // parameters copied from the implementation of deprecated audio_resample_init()
	}
	else
	    resample = false;
}
*/
/*
void VoIPReceiver::correctWavHeader(const char *filename)
{
	struct stat statbuf;
	FILE *fp;
	int32_t filesize;
	stat(filename, &statbuf);

	fp = fopen(filename, "r+b");

	fseek(fp, 4L, SEEK_SET);
    filesize = htole32((int)statbuf.st_size - 8);
	fwrite(&filesize, 4, 1, fp);

	fseek(fp, 40L, SEEK_SET);
	filesize = htole32((int)statbuf.st_size - 44);
	fwrite(&filesize, 4, 1, fp);
	fclose(fp);
}
*/

void VoIPReceiver::finish()
{
	struct stat statbuf;
	ev << "Sink finish()" << endl;
	char command[1000];
	const char *last;
	int len;
//	fclose(original);
//	fclose(degenerated);
//	correctWavHeader(originalWavFileName);
//	correctWavHeader(degeneratedWavFileName);
//	sprintf(command, "[Run %s %d]\n", ev.getConfigEx()->getActiveConfigName(), ev.getConfigEx()->getActiveRunNumber());
//	sprintf(command, "total number of VoIP packets:\t%d\n", pktno);
//	sprintf(command, "number of transmission errors:\t%d\n", transmissionErrors);
//	sprintf(command, "number of silence packets:\t%d\n", numberOfVoIpSilence);
//	delete[] samples; samples = NULL;
//	delete[] g726buf; g726buf = NULL;
//	fclose(result);
}

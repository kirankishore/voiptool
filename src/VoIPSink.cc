
#include "VoIPSink.h"

#include "INETEndians.h"

Define_Module(VoIPSink);

void VoIPSink::initialize()
{
	// Say Hello to the world
	ev << "VoIPSink initialize()" << endl;
	pktno = 0;
	audiostream = -1;
	pCodec726Enc = NULL;
	pCodec726Dec = NULL;
	pReSampleCtx = NULL;
	//read in omnet parameters
	samplesPerPacket = par("samplesPerPacket");
	computePesqValue = par("computePesqValue");
	codingRate = par("codingRate");
	localPort = par("localPort");
	originalWavFileName = par("originalWavFileName").stringValue();
	degeneratedWavFileName = par("degeneratedWavFileName").stringValue();
	resultFile = par("resultFile").stringValue();
	
	//initialize avcodec library
	av_register_all();
	
	//initialize resample check to false
	resample = false;
	SysMsg *smsg = new SysMsg("SINK_INIT_TRG",SINK_INIT_TRG);
	scheduleAt(0.0, smsg);
	
	//allocate audio buffers
	//in a wav file, one frame read in contains 2048 samples, in a mp3 file, one frame contains 1152 samples per channel.
	samples = new int16_t[6000];
	g726buf = new uint8_t[2*samplesPerPacket];
	cur_file = new char[1000];
	unreadSamples = 0;
	psamples = 0;
	startPos = -1;
	transmissionErrors = 0;
	numberOfVoIpSilence = 0;
	
	
	writeFakeWavHeader(originalWavFileName);
	writeFakeWavHeader(degeneratedWavFileName);
	original = fopen(originalWavFileName, "ab");
	degenerated = fopen(degeneratedWavFileName, "ab");
	if((original == NULL) || (degenerated == NULL))
	{
		error("Open file for writing failed! System error ");
	}
	packet.data = NULL;
	bindToPort(localPort);
}

void VoIPSink::handleMessage(cMessage *msg)
{
	VoIP_fileList *traceList=NULL;
	VoIP_fileEntry *pkt=NULL;
	traceList = VoIPGenerator::getList();
	switch(msg->getKind())
	{
		case SINK_INIT_TRG:
			pkt = traceList->getPacket(pktno);
			strcpy(cur_file, pkt->getWaveFile());
			initializeAudio();
			delete msg;
			break;

		default:
			handleMessage2(msg);
			break;
	}
}

void VoIPSink::handleMessage2(cMessage *msg)
{
	IpPacket *ip=NULL;
	
	VoIP_fileList *traceList=NULL;
	VoIP_fileEntry *pkt=NULL;
	
	traceList = VoIPGenerator::getList();
	
	ip = (IpPacket *)(PK(msg)->decapsulate());
	delete msg;
	if(ip == NULL)
	{
		ev << "Sink: Unknown Packet received!" << endl;
		return;
	}
	switch(ip->getKind())
	{
		case VOIP:
			ev << "Sink: VoIP Packet received!" << endl;
			pkt = traceList->getPacket(pktno++);
			if(strcmp(cur_file, pkt->getWaveFile()) != 0)
			{
				cerr << "Changing audio file!" << endl;
				cur_file = pkt->getWaveFile();
				avcodec_close(pCodecCtx);
				avcodec_close(p726EncCtx);
				avcodec_close(p726DecCtx);
				av_free(p726EncCtx);
				av_free(p726EncCtx);
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
			if(ip->hasBitError())
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
			if(ip->getType() == VoIP_fileEntry::SILENCE)
			{
				//silence packet, insert silence!
				numberOfVoIpSilence++;
				for(int i=psamples; i<(psamples+samplesPerPacket); i++)
				    samples[i]=0;
			}
			encodeNextPacket();
			break;

		default:
			ev << "Sink: Unknown Packet received!" << endl;
	}
	delete ip;
}

void VoIPSink::encodeNextPacket()
{
	int g726_size, pcm_size;
	int16_t *newSamples;

	newSamples = new int16_t[2*samplesPerPacket]; // doppelt h�lt besser ;)
    pcm_size = sizeof(int16) * 2 * samplesPerPacket;
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
	fwrite(newSamples, 2, pcm_size, degenerated);
}

int VoIPSink::readNextFrame()
{
	int frame_size, new_frame_size;
	int i;
	int16_t *newSamples, *resamples;
	newSamples = new int16_t[3000];


	for(;;)
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
        frame_size = 6000;
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

void VoIPSink::writeFakeWavHeader(const char *filename)
{
	/* RIFF WAVE format header:
	"RIFF"             -          4 bytes
	(filesize-8)     -          4 bytes
	"WAVE"         -          4 bytes
	"fmt "              -          4 bytes
	16                  -          4 bytes (length of the remaining header = 16 bytes)
	<format tag> -         2 bytes
	<channels> -         2 bytes
	<sample rate> -     4 bytes
	<bytes / second> - 4 bytes  (sample rate * block align)
	<block align>        - 2 bytes  (channels * bits/sample / 8)
	<bits / sample>    -  2 bytes ( 8, 16 oder 24)
	"data"                    -  4 bytes
	<length of data block> - 4 bytes (length of datablock = filesize - 44)
	
	-------------------------------------------
	format tag: see http://de.wikipedia.org/wiki/RIFF_WAVE for details,
	0x0001 means PCM wave
	*/
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
        htole32(16) /*remaining header*/,
        htole16(1) /*PCM wave*/,
        htole16(1)/*channels*/,
        htole32(8000),
        htole32(16000),
        htole16(2),
        htole16(16),
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

void VoIPSink::initializeAudio()
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
	for(unsigned int i=0; i<pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO)
        {
            audiostream = i;
            break;
        }
	}
	if(audiostream == -1)
	{
		error("No audiostream found - invalid file!");
	}

	pCodecCtx=pFormatCtx->streams[audiostream]->codec;
	//find decoder for input file in library
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec == NULL)
	{
		error("No codec to decode input file found!");
	}
	if(avcodec_open(pCodecCtx, pCodec)<0)
	{
		error("opening the correct codec failed!");
	}
	
	//allocate g726 encoder and decoder
	p726EncCtx = avcodec_alloc_context();
	//valid bitrates: 16000 bits/s, 32000 bits/s, 40000 bits/s
	p726EncCtx->bit_rate = codingRate;

	// G.726 supports only one channel with a sampling frequency of 8000 Hz !
	// Thus, any input files, which differ from that must be resampled!
	p726EncCtx->sample_rate = 8000;
	p726EncCtx->channels = 1;
	p726DecCtx = avcodec_alloc_context();
	p726DecCtx->bit_rate = codingRate;
	p726DecCtx->sample_rate = 8000;
	p726DecCtx->channels = 1;
	
	//search vor encoder and decoder codec in library
	pCodec726Enc = avcodec_find_encoder(CODEC_ID_ADPCM_G726);
	if(pCodec726Enc == NULL)
	{
		error("G.726 Codec not found!");
	}
	pCodec726Dec = avcodec_find_decoder(CODEC_ID_ADPCM_G726);
	if(pCodec726Dec == NULL)
	{
		error("G.726 Codec not found!");
	}
	if(avcodec_open(p726EncCtx, pCodec726Enc) < 0)
	{
		error("could not open G.726 encoding codec!");
	}
	if(avcodec_open(p726DecCtx, pCodec726Dec) < 0)
	{
		error("could not open G.726 decoding codec!");
	}
	
	if(pCodecCtx->sample_rate != 8000)
	{
		//sampling rate is not 8000 Hz, we must resample!
		resample = true;
		// if the file has more than one channel, we'll just take the average above all channels - this is not done with the ReSampleContext.
		// initialize resample context
        pReSampleCtx = av_audio_resample_init(1, 1, 8000, pCodecCtx->sample_rate,
                SAMPLE_FMT_S16, pCodecCtx->sample_fmt, 16, 10, 0, 0.8);
                // parameters copied from the implementation of deprecated audio_resample_init()
	}
	else
	    resample = false;
}

void VoIPSink::correctWavHeader(const char *filename)
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

void VoIPSink::finish()
{
	struct stat statbuf;
	ev << "Sink finish()" << endl;
	char command[1000];
	const char *last;
	double pesqValue;
	int len;
	FILE *in;
	FILE *result;
	fclose(original);
	fclose(degenerated);
	correctWavHeader(originalWavFileName);
	correctWavHeader(degeneratedWavFileName);
	result = fopen(resultFile, "at");
	sprintf(command, "[Run %s %d]\n", ev.getConfigEx()->getActiveConfigName(), ev.getConfigEx()->getActiveRunNumber());
	fputs(command, result);
	sprintf(command, "total number of VoIP packets:\t%d\n", pktno);
	fputs(command, result);
	sprintf(command, "number of transmission errors:\t%d\n", transmissionErrors);
	fputs(command, result);
	sprintf(command, "number of silence packets:\t%d\n", numberOfVoIpSilence);
	fputs(command, result);
	delete[] samples;
	delete[] g726buf;
	if(computePesqValue)
	{
		if(stat("pesq", &statbuf) != 0) // Check if file exists
		{
			error("computePesqValue is enabled, but an error occured ");
		}
		strcpy(command, "./pesq +8000 ");
		strcat(command, originalWavFileName);
		strcat(command, " ");
		strcat(command, degeneratedWavFileName);
		strcat(command, " > /dev/null");
		//make sure, the file doesn't exist
		remove("_pesq_results.txt");
		system(command);
		in = fopen("_pesq_results.txt", "rt");
		if(in == NULL)
		{
			ev << "Ooops! what happened ? for some reason, the pesq tool did not create the output files... did you download the correct version?" << endl;
			fclose(result);
			return;
		}
		fgets(command, 1000, in); // ignore first line
		fgets(command, 1000, in);
		fclose(in);
		
		//delete files created by the pesq programm
		remove("_pesq_results.txt");
		remove("_pesq_itu_results.txt");
		// find the last occurence of / in the filename, since in the file _pesq_results.txt, only the filename is written, the path is omitted
		last = strrchr(degeneratedWavFileName, '/');
		if(last == NULL)
			len = strlen(degeneratedWavFileName);
		else 
		{
			last++;
			len = strlen(last);
		}
		pesqValue = strtod(&(command[len]), NULL);
		ev << "pesq Value: " << pesqValue << endl;
		//here should be the filename of the degenerated file
		sprintf(command, "pesq Value: %lf\n", pesqValue);
		fputs(command, result);
	}
	fclose(result);
}

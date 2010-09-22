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

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

//static member variables must be declared AND implemented... and ISO standard forbids implementation in the class { } section
VoIPGenerator *VoIPGenerator::thisptr = (VoIPGenerator *)NULL;

Define_Module(VoIPGenerator);

// a function which checks if a found dir entry maches the pattern *.* - and returns 1 if matched, 0 if not matched.
// needed for scandir...
int filter(const struct dirent *e)
{
	return (fnmatch("*.*" ,e->d_name, FNM_PATHNAME | FNM_PERIOD) == 0) ? 1 : 0;
}

void VoIPGenerator::initialize(int stage)
{
    DIR *sound;
    char *name;
    VoIP_fileEntry *pkt;
    struct dirent  **namelist;
    // say HELLO to the world
    ev << "VoIPGenerator -> initialize(" << stage << ")" << endl;
    VoIPGenerator::thisptr = this;
    if(stage != 3)  //wait until stage 3 - The Adress resolver does not work before that!
	    return;

    name = new char[255];


    // read the parameters from ned files
    //intArrTime    = par("voipIntArrTime");
    //voipPktSize   = par("voipPacketSize");
    //voipSilenceSize = par("voipSilenceSize");
    voipHeaderSize = par("voipHeaderSize");
    samplesPerPacket = par("samplesPerPacket");
    codingRate = par("codingRate");
    intArrTime = (double)samplesPerPacket / 8000.0;
    ev << "interArrivalTime: a packet is send every " << intArrTime*1000.0 << "ms!" << endl;
    voipPktSize = voipHeaderSize + (int)SIMTIME_DBL(intArrTime * (double)codingRate);
    ev << "voip Pkt size in Bits: " << voipPktSize <<endl;
    voipSilenceSize = voipHeaderSize;
    voipSilenceThreshold = par("voipSilenceThreshold");
    soundFileDir = par("soundFileDir").stringValue();
    writeTracesToDisk = par("writeTracesToDisk");
    traceFileBasename = par("traceFileBasename").stringValue();
    filemode = par("filemode").stringValue();
    destAddress = par("destAddress").stringValue();
    localPort = par("localPort");
    destPort = par("destPort");
    //I need to cheat a little bit to access the "General" section of the omnetpp.ini :P
    simTimeLimit = 100.0; //FIXME do not use simtime limit!

    // initialize pointer to character array used for int-char[] conversions
    charVal = new char[33]; // this is enough for radix 2 and 32 bit computers

    // initialize the sequence number
    pktID = 1;

    //search for audio files
    sound = NULL;
    //check if dir is readable
    sound = opendir(soundFileDir);
    if(sound == NULL)
    {
	    error("An Error has occured while reading the wav dir ");
    }

    closedir(sound);
    // dir is readable - start scanning for .wav files
    noWavFiles = scandir(soundFileDir, &namelist, filter, alphasort);
    if(noWavFiles < 0)            //this case shouldn't happen
    {
	    error("Error scanning wav-dir for entries");
    }

    if(noWavFiles == 0)       // no matching files found...
    {
	    error("No wav Files found!");
    }

    //extract the needed infos (like length, sampling rate, etc) into a readable structure
    list = VoIPGenerator::getWavInfo(namelist, noWavFiles);

    //Debug output!
    for(int i=0; i<noWavFiles; i++)
    {
	    ev << "name: " << list[i]->name << endl;
	    ev << "length: " << list[i]->length << endl;
	    ev << "samplingrate: " << list[i]->sample_rate << endl;
	    ev << "-----------------------" << endl;
    }

    /*iii=0;
    while((pkt = blub->getPacket(iii++)) != NULL)
    {
      cerr << "Time: " << pkt->getTime() << " Typ: " << pkt->getPacketType() << " Groesse: " << pkt->getSize() << endl;
    }
    */

    traceList = generateTrace(simTimeLimit + 0.5);

    // create a triggering message
    if(traceList != NULL)
    {
        // debug output
        ev << "First VoIP Packet at: " << traceList->getCurrentPacket()->getTime() << endl;
        SysMsg *voipTrgMsg = new SysMsg("SRC_VOIP_TRG", SRC_VOIP_TRG);

        pkt = traceList->getCurrentPacket();
        // schedule trigger message for packet sending
        scheduleAt(pkt->getTime(), voipTrgMsg);
    }

    delete[] name;
}

void VoIPGenerator::handleMessage(cMessage *msg)
{
    // create an IP message
    IpPacket *ip;
    cPacket *enc;
    VoIP_fileEntry *pkt;
    IPvXAddress dest;

    // switch between different kinds of messages
    switch(msg->getKind())
    {
    // a selfmessage triggering a VOIP packet to be sent
    case SRC_VOIP_TRG:

        // create VoIPpacket
        ip = new IpPacket("VOIP",VOIP);

        //get current voIP packet
        pkt = traceList->getCurrentPacket();

        //advance to the next packet (to send the trigger message for the next packet correctly )
        traceList->next();

         // Length in Bits !
        ip->setBitLength(pkt->getSize());

         //Type: VoIP / Silence
        ip->setType(pkt->getPacketType());
        ip->setName(itoa(pktID));
        enc = new cPacket("VOIP", VOIP);
        enc->encapsulate(ip);  //the function sendToUDP alters the message kind, thus I'm encapsulating the real message to preserve the kind!
        pktID++;

         // send VoIP Packet
         //send(ip, gate("to_udp"));

        dest = IPAddressResolver().resolve(destAddress);
        bindToPort(localPort);

        sendToUDP(enc, localPort, dest, destPort);

        // reschedule trigger message
        scheduleAt(traceList->getCurrentPacket()->getTime(), msg);

        break;

    // if the message is not known...
    default :
        // ... it can't be handled: delete it!
        delete msg;
    }
}


// returns a string representation of the given int value
char* VoIPGenerator::itoa(int intVal)
{
    // convert int id into a string
    sprintf(charVal,"%d",intVal);

    return charVal;
}

VoIP_fileList *VoIPGenerator::getList()
{
	return thisptr->traceList;
}


void VoIPGenerator::finish()
{
    // say GOOD BYE...
    char name[255];
    ev << "TrafficGenerator -> finish()" << endl;
    strcpy(name, traceFileBasename);
    strcat(name, ".log");
    // For very low values of voip_rate, it can happen that one terminal doesn't get any VoIP packets at all... in this case, traceList[msID] is NULL !
    if(writeTracesToDisk)
    // If this flag is set, the VoIP Trace Files will be written to disk!
    {
	    if(traceList != NULL)
	        VoIPGenerator::writeToDisk(traceList, name);
	    else 
	    {
	    //Create an empty file!
		    FILE *touch = fopen(name,"wt");
		    fclose(touch);
	    }
    }
    delete traceList;
}


// this function uses the random number generator #3 - you can alter the standard seed if you want to produce
// more than one result
VoIP_fileList *VoIPGenerator::generateTrace(simtime_t sec_)
{
    double sec = SIMTIME_DBL(sec_);
	double  length, left, i, start, curpos;
	int audiostream, no,  frame_size, min, max;
	int new_frame_size, unreadSamples;
	int psamples;
	int64_t pos;
	bool resample, startPos;
	int16_t *samples;
	int16_t *newSamples;
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	ReSampleContext *pReSampleCtx;
	AVPacket packet;
	
	// initialize avcodec library
	av_register_all();
	
	resample = false;
	startPos = false;
	
	// Buffer for audio samples
	samples = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	newSamples = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	
	VoIP_fileList *packetList;
	
	//maximum Number of packets: number of  voip intervalls inside length of trace
	// 1 second extra buffer for safety
	packetList = new VoIP_fileList((int)(ceil((sec + 1.0) / intArrTime)));
	left = sec;
	unreadSamples = 0;
	psamples = 0;
	while(left > 0)
	{
		// determine which wav file to use
		no = intuniform(0,thisptr->noWavFiles-1, 3);
		if(list[no]->length > left) 
		{
			// the length, how much of the wavfile is to be used;
			length = left;
			// startposition inside the wavfile
			//start = uniform(0.0,list[no]->length - length, 3);
			start = 0.0;
		} else {
			length = list[no]->length;
			start = 0.0;
		}
	
		// If the length is smaller than one VoIP Packet - doesn't matter because in this case he will send one packet... covering the length ;)
		// open input file
		av_open_input_file(&pFormatCtx, list[no]->name, NULL, 0, NULL);
		av_find_stream_info(pFormatCtx);
		audiostream = -1;
	
		//get stream number
		for(unsigned int j=0; j<pFormatCtx->nb_streams; j++)
		{
			if(pFormatCtx->streams[j]->codec->codec_type==CODEC_TYPE_AUDIO)
            {
                audiostream=j;
                break;
            }
		}
		pCodecCtx = pFormatCtx->streams[audiostream]->codec;
	
		//find decoder and open the correct codec
		pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
		avcodec_open(pCodecCtx, pCodec);
		if(pCodecCtx->sample_rate != 8000)
		{
			resample = true;
//			pReSampleCtx = audio_resample_init(1, 1, 8000, pCodecCtx->sample_rate);
            pReSampleCtx = av_audio_resample_init(1, 1, 8000, pCodecCtx->sample_rate,
                    SAMPLE_FMT_S16, pCodecCtx->sample_fmt, 16, 10, 0, 0.8);
                    // parameters copied from the implementation of deprecated audio_resample_init()
		} else  {
			resample = false;
			pReSampleCtx = NULL;
		}

		// avcodec uses a quite wierd timestamp format, calculate the startposition
		pos = (int64_t)(start * ((double)pFormatCtx->streams[audiostream]->time_base.den) / ((double)pFormatCtx->streams[audiostream]->time_base.num));
		//seek to the start if it is greater than zero
		if(start > 0.0001) 
			av_seek_frame(pFormatCtx, audiostream, pos, 0);
		i = length;
		while(i > 0)
		{
			//read one frame
            int err = av_read_frame(pFormatCtx, &packet);
			if(err < 0)
			{
				error("Error reading frame: %s, err=%d", soundFileDir, err);
			}
			
			// if the frame doesn't belong to our audiostream, continue... is not supposed to happen,
			// since .wav contain only one media stream
			if(packet.stream_index != audiostream)
			    continue;
			
		 	// packet length == 0 ? read next packet
			if(packet.duration == 0)
			    continue;
			
			// decode audio and save the decoded samples in our buffer
			frame_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
			avcodec_decode_audio2(pCodecCtx, newSamples, &frame_size, packet.data, packet.size);

			if(frame_size == 0)
			    continue;

			if(pCodecCtx->channels == 2)
			{
				for(int j=0; j<frame_size/4; j++) newSamples[j] = (newSamples[2*j] + newSamples[2*j+1]) / 2;
				frame_size = frame_size / 2;
			}
			
			if(startPos)
			{
				for(int j=0; j<unreadSamples; j++)
					samples[j] = samples[psamples+j];
				psamples = 0;
			}
			startPos = !startPos;
			if(resample)
			{
				new_frame_size = audio_resample(pReSampleCtx, &samples[psamples+unreadSamples], newSamples, frame_size/2);
				unreadSamples += new_frame_size;
			} else {
				for(int j=0; j<frame_size/2; j++)
				    samples[psamples+unreadSamples+j] = newSamples[j];
				unreadSamples += frame_size / 2;
			}
		
			// number of packets in this frame, one sample = 2 bytes
			while(unreadSamples >= samplesPerPacket)
			{
				// search for min- and maximum amplitude
				min = max = samples[psamples];
			
				//if the upper bound is greater than the frame size, use the frame size as upper bound
				for(int k=psamples+1; k<psamples+samplesPerPacket; k++)
				{
					if(samples[k] < min) min = samples[k];
					if(samples[k] > max) max = samples[k];
				}
				// if the maximum amplitude in this intervall is below a given value, the 8 ms packet is send
				// as a silence-packet, which uses much less bandwidth (one sample is 16 bit wide... from -32768 until 32767 )
				curpos = sec - left;
				if((abs(min) > voipSilenceThreshold) || (abs(max) > voipSilenceThreshold))
				{
					packetList->setNewPacket(curpos, VoIP_fileEntry::VO_IP, thisptr->voipPktSize, list[no]->name, start+length-i);
				} else {
					packetList->setNewPacket(curpos, VoIP_fileEntry::SILENCE, thisptr->voipSilenceSize, list[no]->name, start+length-i);
				}
				left -= SIMTIME_DBL(thisptr->intArrTime);
				i -= SIMTIME_DBL(thisptr->intArrTime);
				
				psamples += samplesPerPacket;
				unreadSamples -= samplesPerPacket;
				
				if(i<=0)
				    break;
			}
			av_free_packet(&packet);
		}
		avcodec_close(pCodecCtx);
		av_close_input_file(pFormatCtx);
	}
	delete[] samples;
	delete[] newSamples;
	return packetList;
}

// extract some infos from the wav files!
struct wavinfo **VoIPGenerator::getWavInfo(struct dirent **namelist, int n)
{
	struct wavinfo **infos;
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	int audiostream, fm;
	infos = new struct wavinfo *[n];
	
	//initialization of the avcodec / avformat library
	av_register_all();
	fm = strcmp(thisptr->filemode, "random");
	for(int i=0; i<n; i++)
	{
		// the entries in namelist - the result of scandir - are without a path, so we need to concat the path to that name
		infos[i] = new struct wavinfo;
		strcpy(infos[i]->name, thisptr->soundFileDir);
		if(thisptr->soundFileDir[strlen(thisptr->soundFileDir)-1] != '/')
		    strcat(infos[i]->name, "/");
		
		if(fm == 0)
		    strcat(infos[i]->name, namelist[i]->d_name);
		else {
			strcat(infos[i]->name, thisptr->filemode);
			thisptr->noWavFiles = 1;
		}
		// try to open the file... should not fail!
		
		if(av_open_input_file(&pFormatCtx, infos[i]->name, NULL, 0, NULL) != 0)
		{
			ev << "Unable to open file "<< infos[i]->name << "!\n";
			// file could not be opened
			infos[i]->name[0] = 0;
			infos[i]->length = -1;
			infos[i]->sample_rate=0;
			if(fm != 0)
			    error("Unable to open file %s!", infos[i]->name); // in single-file mode, opening this file must not fail
			else
			    continue;
		}
		
		if(av_find_stream_info(pFormatCtx) < 0)
		{
			continue;
		}
		audiostream = -1;
		// get correct audiostream and codec
		// with this code, one could read more audio formats like mp3 etc; even reading the audio track of a video file
		// is possible.
		for(unsigned int j=0; j<pFormatCtx->nb_streams; j++)
		{
			if(pFormatCtx->streams[j]->codec->codec_type==CODEC_TYPE_AUDIO)
            {
                audiostream=j;
                break;
            }
		}
		//open the decoder codec
		pCodecCtx=pFormatCtx->streams[audiostream]->codec;
		pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
		avcodec_open(pCodecCtx, pCodec);
		
		//we assumed that the voip stream is encoded with the audio codec G.726 .
		// this codec only supports the sampling rate 8000 Hz. Thus, either all files have to
		// match this rate or, one would have to use a resampling function...
		if(pCodecCtx->sample_rate != 8000)
		{
			// Here would be the place for a resampling function - or an error message...
		}
		// save infos
		infos[i]->sample_rate = pCodecCtx->sample_rate;
		infos[i]->length = (double)pFormatCtx->duration / (double)AV_TIME_BASE;
		avcodec_close(pCodecCtx);
		av_close_input_file(pFormatCtx);
		if(fm != 0)
		    break;
	}
	return infos;
}

void VoIPGenerator::writeToDisk(VoIP_fileList *traceList, char *filename)
{
	int no,i;
	int current;
	VoIP_fileEntry *pkt;
	FILE *out;
	if((filename == NULL) || (traceList == NULL))
	{
		ev << "writeTracesToDisk failed!" << endl;
		perror("writeToDisk: ");
		return;
	}
	//File will be overwritten if it exists
	out = fopen(filename, "wt");
	if(out == NULL)
	{
		error("writeTracesToDisk failed!");
	}
	no = traceList->getNumber();
	current = traceList->getCurrent();
	traceList->setCurrent(0);
	fprintf(out, "time\t\tarrivalTime\ttype\tsize\tpos/wav\t\tpacketNo\terror\twavfile\n");
	for(i=0; i<no; i++)
	{
		pkt = traceList->getCurrentPacket();
		traceList->next();
		
		//write only the traces to disk that actually have been send (or queued to send)
		//if(pkt->getPacketNo() == -1) break;
		
		fprintf(out, "%s\t%s\t", SIMTIME_STR(pkt->getTime()), SIMTIME_STR(pkt->getArrivalTime()));
        fprintf(out, "%s\t", (pkt->getPacketType() == VoIP_fileEntry::SILENCE) ? "Silence" : "VoIP");
		fprintf(out, "%d\t%lf\t",pkt->getSize(), pkt->getPosInWav());
		fprintf(out, "%6d\t\t", pkt->getPacketNo());
        fprintf(out, "%s\t", (pkt->hasError()) ? "1" : "0");
		fprintf(out, "%s\n", pkt->getWaveFile());
	}
	traceList->setCurrent(current);
	fclose(out);
}

 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

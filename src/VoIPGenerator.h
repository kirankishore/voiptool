/***************************************************************************
                 TrafficGenerator.h  -  simple traffic generator
                             -------------------
    begin                : Wed Jul 13 2005
    copyright            : (C) 2005 by M. Bohge
    email                : bohge@tkn.tu-berlin.de
 ***************************************************************************/

#ifndef SOURCE_H
#define SOURCE_H

#include <dirent.h>
#include <fnmatch.h>
#include <vector>
#include <omnetpp.h>
#include "IpPacket_m.h"
#include "SysMsg_m.h"
#include "VoIP_fileList.h"
#include "VoIP_fileEntry.h"
#include "UDPAppBase.h"
#include <UDPControlInfo_m.h>
#include "IPAddressResolver.h"

using namespace std;

struct wavinfo {
	char name[256];			//filename
	double length;				//length in seconds
	int sample_rate;			//samplerate in Hz
};

class INET_API VoIPGenerator : public UDPAppBase
{
/*  Module_Class_Members(VoIPGenerator, cSimpleModule, 0);*/


  public:
	//generate Tracefile for ONE terminal
	//sec - length in seconds of tracefile
	//Function is static to be accessable from other classes
	
	static VoIPGenerator *thisptr;
	static VoIP_fileList *getList();

  protected:
    virtual void initialize(int stage);
    virtual int numInitStages() const {return 4;}
    virtual void handleMessage(cMessage *msg);
    virtual void finish();

// This function generates a packet list of VoIP-packets for a given length
    VoIP_fileList *generateTrace(simtime_t sec);
    /*static*/ struct wavinfo **getWavInfo(struct dirent **namelist, int n);
    /*static*/ void writeToDisk(VoIP_fileList *traceList, char *filename);

    char* itoa(int intVal); // returns a string representation of the given int value

            
  protected:
    bool writeTracesToDisk;      //bool value - parameter if VoIP tracefiles should be written to disk
    int pktID;             // increasing packet sequence number
    int voipPktSize;       // size of VoIP packets
    int voipHeaderSize;
    int samplesPerPacket;
    int codingRate;
    int voipSilenceSize;  // size of a silence packet
    int voipSilenceThreshold;  // the maximum amplitude of a silence packet
    int noWavFiles;    // number of VoIP wav files available
    int localPort;
    int destPort;
    const char *soundFileDir;       //Directory containing wav files
    const char *traceFileBasename;      //how the tracefiles should be named
    const char *filemode;                  // either "random" or a fixed filename
    const char *destAddress;
    struct wavinfo **list;     // list of available wavfiles
    VoIP_fileList *traceList;

    simtime_t intArrTime;     // VOIP packet interarrival time
    simtime_t simTimeLimit; //Simulation Time Limit

    char* charVal;        // pointer to character array used for int-char[] conversions

};
#endif

int filter(struct dirent *e);
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

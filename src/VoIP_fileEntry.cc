
#include "VoIP_fileEntry.h"

VoIP_fileEntry::VoIP_fileEntry(simtime_t t, VoIPPacketType type, int s, char *filename, double pos)
{
    //FIXME why double the pos???
	time = t;
	packetType = type;
	size = s;
	wavefile = filename;
	posInWav = pos;
	// Set the packet number to an invalid number... will be set later!
	packetNo = -1;
	arrivalTime = -1.0;
	error = false;
}

VoIP_fileEntry::~VoIP_fileEntry()
{
}

void VoIP_fileEntry::setArrivalTime(simtime_t t)
{
	arrivalTime = t;
}

void VoIP_fileEntry::setBitErrorRate(bool e)
{
	error = e;
}

void VoIP_fileEntry::setPacketNo(int no)
{
	packetNo = no;
}

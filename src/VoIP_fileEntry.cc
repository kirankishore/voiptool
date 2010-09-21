
#include "VoIP_fileEntry.h"

VoIP_fileEntry::VoIP_fileEntry(double t, int type, int s, char *filename, double pos)
{
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

double VoIP_fileEntry::getTime()
{
	return time;
}

double VoIP_fileEntry::getArrivalTime()
{
	return arrivalTime;
}

void VoIP_fileEntry::setArrivalTime(double t)
{
	arrivalTime = t;
}

bool VoIP_fileEntry::hasError()
{
	return error;
}

void VoIP_fileEntry::setError(bool e)
{
	error = e;
}

double VoIP_fileEntry::getPosInWav()
{
	return posInWav;
}

int VoIP_fileEntry::getPacketType()
{
	return packetType;
}

int VoIP_fileEntry::getPacketNo()
{
	return packetNo;
}

void VoIP_fileEntry::setPacketNo(int no)
{
	packetNo = no;
}


int VoIP_fileEntry::getSize()
{
	return size;
}

char *VoIP_fileEntry::getWaveFile()
{
	return wavefile;
}



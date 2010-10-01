#ifndef VOIP_FILEENTRY_H
#define VOIP_FILEENTRY_H

#include <omnetpp.h>

#include "VoIPPacket_m.h"

using namespace std;

// Just a container class - represents exactly one row.

class VoIP_fileEntry
{
	public:
		VoIP_fileEntry(simtime_t t, VoIPPacketType type, int size, char *filename, double pos); //Constructor
		~VoIP_fileEntry();

// packetType: z.B. Silence-packet
// wavefile: Just a pointer to the name, no extra memory is reserved for the name...

		simtime_t getTime() const { return time; }
		double getPosInWav() const { return posInWav; } //FIXME why double???
		VoIPPacketType getPacketType() const { return packetType; }
		int getSize() const { return size; }
		int getPacketNo() const { return packetNo; }
		void setPacketNo(int no);
		simtime_t getArrivalTime() const { return arrivalTime; }
		void setArrivalTime(simtime_t t);
		bool hasError() const { return error; }
		void setBitErrorRate(bool e);
		char *getWaveFile() const { return wavefile; }
	
	protected:
		simtime_t time;
		simtime_t arrivalTime;
		bool error;
		double posInWav; //position inside wavfile //FIXME why double???
		VoIPPacketType packetType;
		int size;
		int packetNo;
		char *wavefile;
};

#endif

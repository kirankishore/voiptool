#ifndef VOIP_FILEENTRY_H
#define VOIP_FILEENTRY_H

#include <omnetpp.h>

using namespace std;

// Just a container class - represents exactly one row.

class VoIP_fileEntry {

	public:
		VoIP_fileEntry(simtime_t t, int type, int size, char *filename, double pos); //Constructor
		~VoIP_fileEntry();

// packetType: z.B. Silence-packet
// wavefile: Just a pointer to the name, no extra memory is reserved for the name...

		simtime_t getTime();
		double getPosInWav(); //FIXME why double???
		int getPacketType();
		int getSize();
		int getPacketNo();
		void setPacketNo(int no);
		simtime_t getArrivalTime();
		void setArrivalTime(simtime_t t);
		bool hasError();
		void setBitErrorRate(bool e);
		char *getWaveFile();
	
	protected:
		simtime_t time;
		simtime_t arrivalTime;
		bool error;
		double posInWav; //position inside wavfile //FIXME why double???
		int packetType;
		int size;
		int packetNo;
		char *wavefile;
};

#endif

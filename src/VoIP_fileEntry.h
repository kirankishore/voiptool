#ifndef VOIP_FILEENTRY_H
#define VOIP_FILEENTRY_H

using namespace std;

// Just a container class - represents exactly one row.

class VoIP_fileEntry {

	public:
		VoIP_fileEntry(double t, int type, int size, char *filename, double pos); //Constructor
		~VoIP_fileEntry();

// packetType: z.B. Silence-packet
// wavefile: Just a pointer to the name, no extra memory is reserved for the name...

		double getTime();
		double getPosInWav();
		int getPacketType();
		int getSize();
		int getPacketNo();
		void setPacketNo(int no);
		double getArrivalTime();
		void setArrivalTime(double t);
		bool hasError();
		void setError(bool e);
		char *getWaveFile();
	
	protected:
		double time;
		double arrivalTime;
		bool error;
		double posInWav; //position inside wavfile
		int packetType;
		int size;
		int packetNo;
		char *wavefile;
};

#endif

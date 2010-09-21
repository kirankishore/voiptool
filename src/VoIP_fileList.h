#ifndef VOIP_FILELIST_H
#define VOIP_FILELIST_H

#include "VoIP_fileEntry.h"

enum type {
SILENCE = 9,
VO_IP = 0 
};

using namespace std;

class VoIP_fileList {
	public:
		VoIP_fileList(int no_pkt);  //Constructor: no_pkt - Number of packets
		~VoIP_fileList();
		int setNewPacket(double t, int type, int size, char *filename, double pos);
		VoIP_fileEntry *getPacket(int pktno);
		VoIP_fileEntry *getCurrentPacket(void);
		void next(void);
		void setCurrent(int number);
		int getCurrent(void);
		int getNumber(void);
		
	
	protected:
		int maxNo, current, last;
		VoIP_fileEntry **list;
};



#endif


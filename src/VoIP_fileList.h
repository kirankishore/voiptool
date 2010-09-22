#ifndef VOIP_FILELIST_H
#define VOIP_FILELIST_H

#include <omnetpp.h>

#include "VoIP_fileEntry.h"

using namespace std;

class VoIP_fileList
{
  public:
    VoIP_fileList(int no_pkt); //Constructor: no_pkt - Number of packets
    ~VoIP_fileList();
    int setNewPacket(double t, VoIP_fileEntry::EntryType type, int size, char *filename, double pos);
    VoIP_fileEntry *getPacket(int pktno) const;
    VoIP_fileEntry *getCurrentPacket(void) const;
    void next();
    void setCurrent(int number);
    int getCurrent() const { return current; }
    int getNumber() const { return last; }

protected:
    int maxNo;
    int current;
    int last;
    VoIP_fileEntry **list;
};

#endif

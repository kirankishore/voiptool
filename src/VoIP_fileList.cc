
#include "VoIP_fileList.h"
#include <stdio.h>

// packets are being added later, for now, set all entries to NULL
VoIP_fileList::VoIP_fileList(int no_pkt)
{
	list = new VoIP_fileEntry *[no_pkt];
	maxNo = no_pkt;
	for(int i=0; i<maxNo; i++) list[i] = NULL;
	current = 0;
	last = 0;
}

VoIP_fileList::~VoIP_fileList()
{
	for(int i=0; i<maxNo; i++)
	{
		if(list[i] != NULL)
		    delete list[i];
	}
	delete[] list;
}

int VoIP_fileList::setNewPacket(double t, VoIP_fileEntry::EntryType type, int size, char *filename, double pos)
{
	if(last >= this->maxNo)
	    return -1;  //maximum number of packets reached

	if(list[last] != NULL)
	    delete list[last];

	list[last] = new VoIP_fileEntry(t, type, size, filename, pos);
	last++;
	return 0;
}

VoIP_fileEntry *VoIP_fileList::getPacket(int pktno) const
{
	if(pktno >= this->maxNo)
	    return NULL;

	return list[pktno];
}

VoIP_fileEntry *VoIP_fileList::getCurrentPacket(void) const
{
	return list[current];
}

void VoIP_fileList::next()
{
	if(++current == maxNo)
	    current = 0;  //overflow protection
}

void VoIP_fileList::setCurrent(int number)
{
	if(number < this->maxNo)
	    current = number;
}

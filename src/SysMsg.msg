// **************************************************************************
//                               SysMsg.msg
//             Omnett++ Message definition file for system messages
//                             -------------------
//    begin                : Mon Jul 18 2005
//    copyright            : (C) 2005 by M. Bohge
//    email                : bohge@tkn.tu-berlin.de
// ***************************************************************************/

cplusplus {{
#include "SysMsg_AddOns.h"
}}


struct Container;


enum SYSMSGTYPES
{

    // src releated messages
    SRC_VOIP_TRG = 201; // triggers the source to send a voip packet
    SRC_FTP_TRG = 202; // triggers the source to send an ftp packet

    SINK_INIT_TRG = 301;  //Triggers initialization


}

enum MODTYPES
{

    BPSK = 1000; // use BPSK as modulation technique
    QPSK = 1001; // use QPSK as modulation technique
    EIGHTQAM = 1002; // use 8-QAM as modulation technique
    ONESIXQAM = 1003; // use 16-QAM as modulation technique
    SIXFOURQAM = 1004; // use 64-QAM as modulation technique
    TWOFIVESIXQAM = 1005; // use 256-QAM as modulation technique
}

message SysMsg
{
    int sender;
    int receiver;
    int id;
    int qosClass;
    int size;
    int qSize[];
    int qPackets[];
    double delay;
    Container container;
    double xPos;
    double yPos;
}


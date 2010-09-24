// **************************************************************************
//                               VoIPPacket.msg
//             Omnett++ Message definition file for VoIP packets
//                             -------------------
//    begin                : Mon Jul 18 2005
//    copyright            : (C) 2005 by M. Bohge
//    email                : bohge@tkn.tu-berlin.de
// ***************************************************************************/

enum VoIPPacketType
{
    VOICE = 1;
    SILENT = 2;
}

enum G726Options
{
    G726_SAMPLERATE = 8000;
}

packet VoIPPacket
{
    int type;
    int id;
}

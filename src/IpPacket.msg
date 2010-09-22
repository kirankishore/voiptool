// **************************************************************************
//                               IpPacket.msg
//             Omnett++ Message definition file for IP packets
//                             -------------------
//    begin                : Mon Jul 18 2005
//    copyright            : (C) 2005 by M. Bohge
//    email                : bohge@tkn.tu-berlin.de
// ***************************************************************************/

enum IPPACKETTYPES
{
    VOIP = 100;
    FTP = 101;
}


packet IpPacket
{
    int type;
    int id;
}

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
  FTP  = 101;
}


message IpPacket
{
  fields:
    int type;
    int id;
};

// ***************************************************************************
// *                                                                         *
// *   This program is free software; you can redistribute it and/or modify  *
// *   it under the terms of the GNU General Public License as published by  *
// *   the Free Software Foundation; either version 2 of the License, or     *
// *   (at your option) any later version.                                   *
// *                                                                         *
// ***************************************************************************/

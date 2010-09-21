// **************************************************************************
//                           VoIPGenerator.ned
//                    Omnett++ Network description file
//                             -------------------
//    begin                : Mon Jul 4 2005
//    copyright            : (C) 2005 by M. Renwanz
//    email                : renwanz@tkn.tu-berlin.de
// ***************************************************************************/

import "VoIPGenerator",
          "VoIPSink";

//define the channel
channel Wireless
   delay 0  //sec
   error 1e-4
   datarate 2000000 // bits/sec
endchannel


module SimpleNet
  parameters:
          voipHeaderSize         : numeric const,
          samplesPerPacket       : numeric const,
          codingRate             : numeric const,
          voipSilenceThreshold   : numeric const,
          computePesqValue       : numeric const,
          writeTracesToDisk      : numeric const,
          traceFileBasename      : string,
          filemode               : string,
          soundFileDir           : string,
          originalWavFileName    : string,
          degeneratedWavFileName : string;

  submodules:
    traffic: VoIPGenerator
      parameters:
        voipSilenceThreshold = voipSilenceThreshold,
        voipHeaderSize = voipHeaderSize,
        samplesPerPacket = samplesPerPacket,
        codingRate = codingRate,
        writeTracesToDisk = writeTracesToDisk,
        traceFileBasename = traceFileBasename,
        filemode = filemode,
        soundFileDir = soundFileDir;
      gatesizes:
      display: "i=block/source";

    voIPsink: VoIPSink
      parameters:
        samplesPerPacket = samplesPerPacket,
        computePesqValue = computePesqValue,
        codingRate = codingRate,
        originalWavFileName = originalWavFileName,
        degeneratedWavFileName = degeneratedWavFileName;
      gatesizes:
      display: "i=block/sink";

   connections:
     traffic.to_udp --> Wireless --> voIPsink.from_udp;
     traffic.from_udp <-- Wireless <-- voIPsink.to_udp;
endmodule

network simpleNet: SimpleNet
endnetwork

// ***************************************************************************
// *                                                                         *
// *   This program is free software; you can redistribute it and/or modify  *
// *   it under the terms of the GNU General Public License as published by  *
// *   the Free Software Foundation; either version 2 of the License, or     *
// *   (at your option) any later version.                                   *
// *                                                                         *
// ***************************************************************************/

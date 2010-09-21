// **************************************************************************
//                           VoIPGenerator.ned
//                    Omnett++ Network description file
//                             -------------------
//    begin                : Mon Jul 4 2005
//    copyright            : (C) 2005 by M. Renwanz
//    email                : renwanz@tkn.tu-berlin.de
// ***************************************************************************/

package voiptool.simulations;

import voiptool.VoIPSink;
import voiptool.VoIPGenerator;


//define the channel
channel Wireless extends ned.DatarateChannel
{
    parameters:
        delay = 0;  //sec
        ber = 1e-4;
        datarate = 2000000bps; // bits/sec
}


module SimpleNet
{
    parameters:
        int voipHeaderSize @unit(B);
        int samplesPerPacket;
        int codingRate;
        int voipSilenceThreshold;
        bool computePesqValue;
        bool writeTracesToDisk;
        string traceFileBasename;
        string filemode;
        string soundFileDir;
        string originalWavFileName;
        string degeneratedWavFileName;

    submodules:
        traffic: VoIPGenerator {
            parameters:
                voipSilenceThreshold = voipSilenceThreshold;
                voipHeaderSize = voipHeaderSize;
                samplesPerPacket = samplesPerPacket;
                codingRate = codingRate;
                writeTracesToDisk = writeTracesToDisk;
                traceFileBasename = traceFileBasename;
                filemode = filemode;
                soundFileDir = soundFileDir;
                @display("i=block/source");

            gates:
        }
        voIPsink: VoIPSink {
            parameters:
                samplesPerPacket = samplesPerPacket;
                computePesqValue = computePesqValue;
                codingRate = codingRate;
                originalWavFileName = originalWavFileName;
                degeneratedWavFileName = degeneratedWavFileName;
                @display("i=block/sink");

            gates:
        }
    connections:
        traffic.udpOut --> Wireless --> voIPsink.udpIn;
        traffic.udpIn <-- Wireless <-- voIPsink.udpOut;
}

network simpleNet extends SimpleNet
{
    parameters:
}


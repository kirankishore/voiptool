package voiptool;

import inet.applications.udpapp.UDPApp;


simple VoIPSink like UDPApp
{
    parameters:
        int samplesPerPacket;
        bool computePesqValue;
        double codingRate;
        int localPort;
        string originalWavFileName;
        string degeneratedWavFileName;
        string resultFile;
    gates:
        input udpIn;
        output udpOut;
}

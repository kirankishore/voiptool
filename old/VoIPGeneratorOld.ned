package voiptool;

import inet.applications.udpapp.UDPApp;

// define the traffic generator
simple VoIPGeneratorOld like UDPApp
{
    // the traffic parameters
    parameters:
        int voipHeaderSize @unit(B);
        int samplesPerPacket;
        int codingRate;
        int voipSilenceThreshold;
        bool writeTracesToDisk;
        int localPort;
        int destPort;
        string destAddress;
        string traceFileBasename;
        string filemode;
        string soundFileDir;

    gates:
        input udpIn;
        output udpOut;
}

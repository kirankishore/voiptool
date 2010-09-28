package voiptool;

import inet.applications.udpapp.UDPApp;

// define the traffic generator
simple VoIPGenerator like UDPApp
{
    // the traffic parameters
    parameters:
        int localPort;
        int destPort;
        string destAddress;
        double start @unit(s) = default(0);

        int voipHeaderSize @unit(B);
        int voipSilenceThreshold;
        int sampleRate @unit(Hz) = default(8000Hz);
        int sampleBits = default(16);	// bits/sample: 8u/16/32
        string codec = default("g726");	// used by ffmpeg::avcodec_find_encoder_by_name()
        int compressedBitRate @unit(bps) = default(40Kbps);
        double packetTimeLength @unit(s) = default(20ms);

        string soundFile;	// filename of input audiofile

        int repeatCount = default(1);
        string traceFileName = default(""); // filename to save output stream, OFF when empty

    gates:
        input udpIn;
        output udpOut;
}

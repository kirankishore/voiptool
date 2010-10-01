package voiptool;

import inet.applications.udpapp.UDPApp;


simple VoIPReceiver like UDPApp
{
    parameters:
        int localPort;
        double timeout @unit(s) = default(3s);
        string resultFile;
    gates:
        input udpIn;
        output udpOut;
}

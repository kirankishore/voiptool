//
// Copyright (C) 2005 M. Bohge (bohge@tkn.tu-berlin.de), M. Renwanz
// Copyright (C) 2010 Zoltan Bojthe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


package voiptool;

import inet.applications.udpapp.UDPApp;

// define the traffic generator
simple VoIPSourceApp like UDPApp
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

simple VoIPSink
  parameters:
     samplesPerPacket       : numeric const,
     computePesqValue       : numeric const,
     codingRate             : numeric const,
     localPort                : numeric const,
     originalWavFileName    : string,
     degeneratedWavFileName : string,
     resultFile             : string;
  gates:
      in: from_udp;
      out: to_udp;
endsimple;
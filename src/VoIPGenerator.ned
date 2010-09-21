// define the traffic generator
simple VoIPGenerator

  // the traffic parameters
  parameters:
      voipHeaderSize        : numeric const,
      samplesPerPacket      : numeric const,
      codingRate            : numeric const,
      voipSilenceThreshold  : numeric const,
      writeTracesToDisk     : numeric const,
      localPort                 : numeric const,
      destPort                   : numeric const,
      destAddress            : string,
      traceFileBasename     : string,
      filemode              : string,
      soundFileDir          : string;

  gates:
      in: from_udp;
      out: to_udp;
// 
endsimple;
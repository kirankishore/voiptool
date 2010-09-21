#!/bin/bash

# This path must be set to the folder of your INET framework installation
INET="/path/to/your/INET"

# Writing dynamic ini file

echo "[General]" > inetdyn.ini
echo "preload-ned-files = VoIPToolTest.ned VoIPGenerator.ned VoIPSink.ned @$INET/nedfiles.lst" >> inetdyn.ini
echo "" >> inetdyn.ini
echo "[Tkenv]" >> inetdyn.ini
echo "plugin-path=$INET/Etc/plugins" >> inetdyn.ini

# Writing executable file

echo "#!/bin/bash" > InetVoIPTest
echo "$INET/bin/INET -l libs/libavutil -l libs/libavformat -l libs/libavcodec \$*" >> InetVoIPTest
chmod a+x InetVoIPTest


./libs/link.sh
opp_makemake -N -f -s -x -I$INET/Applications/UDPApp -I$INET/Network/Contract -I$INET/Base -I$INET/Transport/Contract -I./include -L./libs -lm -lpthread libs/libavutil.so libs/libavformat.so libs/libavcodec.so

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


#ifndef VOIPTOOL_AUDIOOUTFILE_H
#define VOIPTOOL_AUDIOOUTFILE_H


#include <omnetpp.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};


class AudioOutFile
{
  public:
    AudioOutFile() : opened(false) {};
    ~AudioOutFile();

    bool open(const char *resultFile, int sampleRate, short int sampleBits);
    bool write(void *inbuf, int inbytes);
    bool close();
    bool isopened() { return opened; }

  protected:
    void addAudioStream(enum CodecID codec_id, int sampleRate, short int sampleBits);

  protected:
    bool opened;
    AVStream *audio_st;
    AVFormatContext *oc;
};


#endif // VOIPTOOL_AUDIOOUTFILE_H

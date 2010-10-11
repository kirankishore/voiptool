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


#include "AudioOutFile.h"

#include "INETEndians.h"

// FIXME check on WINDOWS!!
#define INT64_C(x) int64_t(x##ULL)
// FIXME check on WINDOWS!!

void AudioOutFile::addAudioStream(enum CodecID codec_id, int sampleRate, short int sampleBits)
{
    AVStream *st = av_new_stream(oc, 1);
    if (!st)
        throw cRuntimeError("Could not alloc stream\n");

    AVCodecContext *c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = CODEC_TYPE_AUDIO;

    /* put sample parameters */
    c->bit_rate = sampleRate * sampleBits;
    c->sample_rate = sampleRate;
    c->channels = 1;
    audio_st = st;
}

bool AudioOutFile::open(const char *resultFile, int sampleRate, short int sampleBits)
{
    if (opened)
        return false;

    opened = true;

    // auto detect the output format from the name. default is WAV
    AVOutputFormat *fmt = guess_format(NULL, resultFile, NULL);
    if (!fmt)
    {
        ev << "Could not deduce output format from file extension: using WAV.\n";
        fmt = guess_format("wav", NULL, NULL);
    }
    if (!fmt)
    {
        throw cRuntimeError("Could not find suitable output format for filename '%s'\n", resultFile);
    }

    // allocate the output media context
    oc = avformat_alloc_context();
    if (!oc)
        throw cRuntimeError("Memory error at avformat_alloc_context()\n");

    oc->oformat = fmt;
    snprintf(oc->filename, sizeof(oc->filename), "%s", resultFile);

    // add the audio stream using the default format codecs and initialize the codecs
    audio_st = NULL;
    if (fmt->audio_codec != CODEC_ID_NONE)
        addAudioStream(fmt->audio_codec, sampleRate, sampleBits);

    // set the output parameters (must be done even if no parameters).
    if (av_set_parameters(oc, NULL) < 0)
        throw cRuntimeError("Invalid output format parameters\n");

    dump_format(oc, 0, resultFile, 1);

    /* now that all the parameters are set, we can open the audio and
       video codecs and allocate the necessary encode buffers */
    if (audio_st)
    {
        AVCodecContext *c = audio_st->codec;

        /* find the audio encoder */
        AVCodec *avcodec = avcodec_find_encoder(c->codec_id);
        if (!avcodec)
            throw cRuntimeError("codec %d not found\n", c->codec_id);

        /* open it */
        if (avcodec_open(c, avcodec) < 0)
            throw cRuntimeError("could not open codec %d\n", c->codec_id);
    }

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE))
    {
        if (url_fopen(&oc->pb, resultFile, URL_WRONLY) < 0)
            throw cRuntimeError("Could not open '%s'\n", resultFile);
    }

    // write the stream header
    av_write_header(oc);

    return true;
}

bool AudioOutFile::write(void *decBuf, int pktBytes)
{
    if (!opened)
        return false;

    AVCodecContext *c = audio_st->codec;
    uint8_t outbuf[pktBytes];
    AVPacket pkt;

    av_init_packet(&pkt);

    // the 3rd parameter of avcodec_encode_audio() is the size of INPUT buffer!!!
    // It's wrong in the FFMPEG documentation/header file!!!
    pkt.size = avcodec_encode_audio(c, outbuf, pktBytes, (short int*)decBuf);
    if (c->coded_frame->pts != AV_NOPTS_VALUE)
        pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, audio_st->time_base);
    pkt.flags |= PKT_FLAG_KEY;
    pkt.stream_index = audio_st->index;
    pkt.data = outbuf;

    // write the compressed frame in the media file
    if (av_interleaved_write_frame(oc, &pkt) != 0)
        throw cRuntimeError("Error while writing audio frame\n");
    return true;
}

bool AudioOutFile::close()
{
    if (!opened)
        return false;

    opened = false;

    /* write the trailer, if any.  the trailer must be written
     * before you close the CodecContexts open when you wrote the
     * header; otherwise write_trailer may try to use memory that
     * was freed on av_codec_close() */
    av_write_trailer(oc);

    /* close each codec */
    if (audio_st)
        avcodec_close(audio_st->codec);

    /* free the streams */
    for(unsigned int i = 0; i < oc->nb_streams; i++)
    {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    if (!(oc->oformat->flags & AVFMT_NOFILE))
    {
        /* close the output file */
        url_fclose(oc->pb);
    }

    /* free the stream */
    av_free(oc);
    oc = NULL;
    return true;
}

AudioOutFile::~AudioOutFile()
{
    close();
}

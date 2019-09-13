#include "ffmpegcapture.h"

#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavutil/timestamp.h>
#include <libavutil/pixdesc.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <iostream>
#include <vector>
#include <fmt/printf.h>


using namespace std;

// a wrapper around a single output AVStream
class FFMPEGCapturePrivate
{
 public:
    FFMPEGCapturePrivate() = default;
    ~FFMPEGCapturePrivate();

    bool init(const std::string& fn);
    bool addStream(int w, int h, float fps);
    bool openVideo();
    bool start();
    bool writeVideoFrame(bool = false);
    void finish();

    bool isSupportedPixelFormat(enum AVPixelFormat) const;

 private:
    int writePacket();

    AVStream        *st       { nullptr };
    AVFrame         *frame    { nullptr };
    AVFrame         *tmpfr    { nullptr };
    AVCodecContext  *enc      { nullptr };
    AVFormatContext *oc       { nullptr };
    AVCodec         *vc       { nullptr };
    AVPacket        *pkt      { nullptr };
    SwsContext      *swsc     { nullptr };

    // pts of the next frame that will be generated
    int64_t         nextPts   { 0 };

    std::string     filename;
    float           fps       { 0 } ;
    bool            capturing { false };

    const Renderer  *renderer { nullptr };

 public:
#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)) // ffmpeg < 4.0
    static bool     registered;
#endif
    friend class FFMPEGCapture;
};

#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)) // ffmpeg < 4.0
bool FFMPEGCapturePrivate::registered = false;
#endif

bool FFMPEGCapturePrivate::init(const std::string& _filename)
{
    filename = _filename;

#if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 10, 100)) // ffmpeg < 4.0
    if (!FFMPEGCapturePrivate::registered)
    {
        av_register_all();
        FFMPEGCapturePrivate::registered = true;
    }
#endif

#ifndef DEBUG_VIDEO
    av_log_set_level(AV_LOG_FATAL);
#endif

    // allocate the output media context
    avformat_alloc_output_context2(&oc, nullptr, nullptr, filename.c_str());
    // use matroska as a fallback container
    if (oc == nullptr)
        avformat_alloc_output_context2(&oc, nullptr, "matroska", filename.c_str());

#ifdef DEBUG_VIDEO
    if (oc != nullptr)
        fmt::printf("Format codec: %s\n", oc->oformat->long_name);
#endif

    return oc != nullptr;
}

bool FFMPEGCapturePrivate::isSupportedPixelFormat(enum AVPixelFormat format) const
{
    const enum AVPixelFormat *p = vc->pix_fmts;
    if (p == nullptr)
        return false;

    for (; *p != -1; p++)
    {
        if (*p == format)
            return true;
    }

    return false;
}

#ifdef DEBUG_VIDEO
// av_ts2str and av_ts2timestr macros are not compatible with C++, so
// we undef them and provide own implementations
#ifdef av_ts2str
#undef av_ts2str
#endif
static std::string av_ts2str(int64_t ts)
{
    char s[AV_TS_MAX_STRING_SIZE];
    av_ts_make_string(s, ts);
    return s;
}

#ifdef av_ts2timestr
#undef av_ts2timestr
#endif
static std::string av_ts2timestr(int64_t ts, AVRational *tb)
{
    char s[AV_TS_MAX_STRING_SIZE];
    av_ts_make_time_string(s, ts, tb);
    return s;
}

static void log_packet(const AVFormatContext *oc, const AVPacket *pkt)
{
    AVRational *time_base = &oc->streams[pkt->stream_index]->time_base;

    fmt::printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}
#endif

int FFMPEGCapturePrivate::writePacket()
{
    // rescale output packet timestamp values from codec to stream timebase
    av_packet_rescale_ts(pkt, enc->time_base, st->time_base);
    pkt->stream_index = st->index;

    // Write the compressed frame to the media file.
#ifdef DEBUG_VIDEO
    log_packet(oc, pkt);
#endif
    return av_interleaved_write_frame(oc, pkt);
}

// add an output stream
bool FFMPEGCapturePrivate::addStream(int width, int height, float fps)
{
    this->fps = fps;

    // find the encoder
    vc = avcodec_find_encoder(oc->oformat->video_codec/*AV_CODEC_ID_FFVHUFF*/);
    if (vc == nullptr)
    {
        cout << "Video codec isn't found\n";
        return false;
    }

#define AVCODEC_DEBUG 1
#if AVCODEC_DEBUG
{
    cout << "codec: " << vc->name << ' ' << vc->long_name <<'\n';

    cout << "supported framerates:\n";
    const AVRational *f = vc->supported_framerates;
    if (f != nullptr)
    {
        for (; f->num != 0 && f->den != 0; f++)
            cout << f->num << ' ' << f->den << '\n';
    }
    else
    {
        cout << "any\n";
    }

    cout << "supported pixel formats:\n";
    const enum AVPixelFormat *p = vc->pix_fmts;
    if (p != nullptr)
    {
        for (; *p != -1; p++)
        {
            auto *d = av_pix_fmt_desc_get(*p);
            cout << d->name << '\n';
        }
    }
    else
    {
        cout << "unknown\n";
    }

    cout << "recognized profiles:\n";
    const AVProfile *r = vc->profiles;
    if (r != nullptr)
    {
        for (; r->profile != FF_PROFILE_UNKNOWN; r++)
            cout << r->profile << ' ' << r->name << '\n';
    }
    else
    {
        cout << "unknown\n";
    }
}
#endif

    st = avformat_new_stream(oc, nullptr);
    if (st == nullptr)
    {
        cout << "Unable to alloc a new stream\n";
        return false;
    }
    st->id = oc->nb_streams - 1;

    enc = avcodec_alloc_context3(vc);
    if (enc == nullptr)
    {
        cout << "Unable to alloc a new context\n";
        return false;
    }

#if 1
    enc->codec_id = oc->oformat->video_codec; // TODO: make selectable
#else
    enc->codec_id = oc->oformat->video_codec = AV_CODEC_ID_FFVHUFF;
#endif

    enc->bit_rate  = 400000; // TODO: make selectable
    // Resolution must be a multiple of two
    enc->width     = width;
    enc->height    = height;
    // timebase: This is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. For fixed-fps content,
    // timebase should be 1/framerate and timestamp increments should be
    // identical to 1.
    if (abs(fps - 29.97) < 1e-5)
        st->time_base = { 1001, 30000 };
    else if (abs(fps - 23.976) < 1e-5)
        st->time_base = { 1001, 24000 };
    else
        st->time_base = { 1, (int) fps };

    enc->time_base = st->time_base;
    enc->framerate = st->avg_frame_rate = { st->time_base.den, st->time_base.num };
    enc->gop_size  = 12; // emit one intra frame every twelve frames at most

    // find a best pixel format to convert to from AV_PIX_FMT_RGB24
    if (isSupportedPixelFormat(AV_PIX_FMT_YUV420P))
    {
        enc->pix_fmt = AV_PIX_FMT_YUV420P;
    }
    else
    {
        enc->pix_fmt = avcodec_find_best_pix_fmt_of_list(vc->pix_fmts, AV_PIX_FMT_RGB24, 0, nullptr);
        if (enc->pix_fmt == AV_PIX_FMT_NONE)
            avcodec_default_get_format(enc, &(enc->pix_fmt));
    }

    if (enc->codec_id == AV_CODEC_ID_MPEG1VIDEO)
    {
        // Need to avoid usage of macroblocks in which some coeffs overflow.
        // This does not happen with normal video, it just happens here as
        // the motion of the chroma plane does not match the luma plane.
        enc->mb_decision = 2;
    }

    // Some formats want stream headers to be separate.
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    return true;
}

bool FFMPEGCapturePrivate::start()
{
    // open the output file, if needed
    if ((oc->oformat->flags & AVFMT_NOFILE) == 0)
    {
        if (avio_open(&oc->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            cout << "Video file open error\n";
            return false;
        }
    }

    // Write the stream header, if any.
    if (avformat_write_header(oc, nullptr) < 0)
    {
        cout << "Failed to write header\n";
        return false;
    }

    av_dump_format(oc, 0, filename.c_str(), 1);

    if ((pkt = av_packet_alloc()) == nullptr)
    {
        cout << "Failed to allocate a packet\n";
        return false;
    }

    return true;
}

bool FFMPEGCapturePrivate::openVideo()
{

    // open the codec
    if (avcodec_open2(enc, vc, nullptr) < 0)
    {
        cout << "Failed to open the codec\n";
        return false;
    }

    // allocate and init a re-usable frame
    if ((frame = av_frame_alloc()) == nullptr)
    {
        cout << "Failed to allocate dest frame\n";
        return false;
    }

    frame->format = enc->pix_fmt;
    frame->width  = enc->width;
    frame->height = enc->height;

    // allocate the buffers for the frame data
    if (av_frame_get_buffer(frame, 32) < 0)
    {
        cout << "Failed to allocate dest frame buffer\n";
        return false;
    }

    if (enc->pix_fmt != AV_PIX_FMT_RGB24)
    {
        // as we only grab a RGB24 picture, we must convert it
        // to the codec pixel format if needed
        swsc = sws_getContext(enc->width, enc->height, AV_PIX_FMT_RGB24,
                              enc->width, enc->height, enc->pix_fmt,
                              SWS_BITEXACT, nullptr, nullptr, nullptr);
        if (swsc == nullptr)
        {
            cout << "Failed to allocate SWS context\n";
            return false;
        }

        // allocate and init a temporary frame
        if((tmpfr = av_frame_alloc()) == nullptr)
        {
            cout << "Failed to allocate temp frame\n";
            return false;
        }

        tmpfr->format = AV_PIX_FMT_RGB24;
        tmpfr->width  = enc->width;
        tmpfr->height = enc->height;

        // allocate the buffers for the frame data
        if (av_frame_get_buffer(tmpfr, 32) < 0)
        {
            cout << "Failed to allocate temp frame buffer\n";
            return false;
        }
    }

    // copy the stream parameters to the muxer
    if (avcodec_parameters_from_context(st->codecpar, enc) < 0)
    {
        cout << "Failed to copy the stream parameters to the muxer\n";
        return false;
    }

    return true;
}

static void captureImage(AVFrame *pict, int width, int height, const Renderer *r)
{
    int x, y, w, h;
    r->getScreenSize(&x, &y, &w, &h);

    x += (w - width) / 2;
    y += (h - height) / 2;
    r->captureFrame(x, y, width, height,
                    Renderer::PixelFormat::RGB,
                    pict->data[0]);

    // Read image is vertically flipped
    // TODO: this should go to Renderer::captureFrame()
    int realWidth = width * 3; // 3 bytes per pixel
    uint8_t tempLine[realWidth];
    uint8_t *fb = pict->data[0];
    for (int i = 0, p = realWidth * (height - 1); i < p; i += realWidth, p -= realWidth)
    {
        memcpy(tempLine, &fb[i],   realWidth);
        memcpy(&fb[i],   &fb[p],   realWidth);
        memcpy(&fb[p],   tempLine, realWidth);
    }
}

// encode one video frame and send it to the muxer
// return 1 when encoding is finished, 0 otherwise
bool FFMPEGCapturePrivate::writeVideoFrame(bool finalize)
{
    AVFrame *frame = finalize ? nullptr : this->frame;

    // check if we want to generate more frames
    if (!finalize)
    {
        // when we pass a frame to the encoder, it may keep a reference to it
        // internally; make sure we do not overwrite it here
        if (av_frame_make_writable(frame) < 0)
        {
            cout << "Failed to make the frame writable\n";
            return false;
        }

        if (enc->pix_fmt != AV_PIX_FMT_RGB24)
        {
            captureImage(tmpfr, enc->width, enc->height, renderer);
            // we need to compute the correct line width of our source
            // data. as we grab as RGB24, we multiply the width by 3.
            const int linesize = 3 * enc->width;
            sws_scale(swsc, tmpfr->data, &linesize, 0, enc->height,
                      frame->data, frame->linesize);
        }
        else
        {
            captureImage(frame, enc->width, enc->height, renderer);
        }

        frame->pts = nextPts++;
    }

    av_init_packet(pkt);

    // encode the image
    if (avcodec_send_frame(enc, frame) < 0)
    {
        cout << "Failed to send the frame\n";
        return false;
    }

    for (;;)
    {
        int ret = avcodec_receive_packet(enc, pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;

        if (ret >= 0)
        {
            ret = writePacket();
            av_packet_unref(pkt);
        }

        if (ret < 0)
        {
            cout << "Failed to receive/unref the packet\n";
            return false;
        }
    }

    return true;
}

void FFMPEGCapturePrivate::finish()
{
    writeVideoFrame(true);

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(oc);

    if (!(oc->oformat->flags & AVFMT_NOFILE))
        avio_closep(&oc->pb);
}

FFMPEGCapturePrivate::~FFMPEGCapturePrivate()
{
    avcodec_free_context(&enc);
    av_frame_free(&frame);
    if (tmpfr != nullptr)
        av_frame_free(&tmpfr);
    avformat_free_context(oc);
    av_packet_free(&pkt);
}


FFMPEGCapture::FFMPEGCapture(const Renderer *r) :
    MovieCapture(r)
{
    d = new FFMPEGCapturePrivate;
    d->renderer = r;
}

FFMPEGCapture::~FFMPEGCapture()
{
    delete d;
}

int FFMPEGCapture::getFrameCount() const
{
    return d->nextPts;
}

int FFMPEGCapture::getWidth() const
{
    return d->enc->width;
}

int FFMPEGCapture::getHeight() const
{
    return d->enc->height;
}

float FFMPEGCapture::getFrameRate() const
{
    return d->fps;
}

bool FFMPEGCapture::start(const std::string& filename, int width, int height, float fps)
{
    if (!d->init(filename) ||
        !d->addStream(width, height, fps) ||
        !d->openVideo() ||
        !d->start())
    {
        return false;
    }

    d->capturing = true; // XXX

    return true;
}

bool FFMPEGCapture::end()
{
    if (!d->capturing)
        return false;

    d->finish();

    d->capturing = false;

    return true;
}

bool FFMPEGCapture::captureFrame()
{
    return d->capturing && d->writeVideoFrame();
}

#include "ffmpegdecode.h"

FFmpegDecode::FFmpegDecode(const QString & url)
    : url_(url)
    , fmt_ctx_(nullptr)
    , video_dec_ctx_(nullptr)
    , audio_dec_ctx_(nullptr)
    , pkt_(nullptr)
    , frame_(nullptr)
    , frame_audio_(nullptr)
    , video_idx_(UNKNOWN_INDEX)
    , audio_idx_(UNKNOWN_INDEX)
    , duration_(0)
{
    video_time_base_.num = 0;
    video_time_base_.den = 0;
    audio_time_base_.num = 0;
    audio_time_base_.den = 0;
}

FFmpegDecode::~FFmpegDecode()
{

}

bool FFmpegDecode::start(int video_index, int audio_index)
{
    error_info_.clear();
    /* open input file, and allocate format context */
    std::string tmp(url_.toLocal8Bit().data());
    const char *file_path = tmp.c_str();

    if (avformat_open_input(&fmt_ctx_, file_path, nullptr, nullptr) < 0) {
        error_info_ = "[FFmpegDecode::start] Could not open source file: " + url_;
        return false;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
        error_info_ = "[FFmpegDecode::start] Could not find stream information";
        return false;
    }
    duration_ = fmt_ctx_->duration * 1000 / AV_TIME_BASE;

    /* dump input information to stderr */
    //av_dump_format(fmt_ctx_, 0, file_path, 0);

    int ret = 0;
    if (UNKNOWN_INDEX != video_index) {
        video_idx_ = video_index;
        video_time_base_ = fmt_ctx_->streams[video_idx_]->time_base;
        if (!open_codec(fmt_ctx_->streams[video_idx_], &video_dec_ctx_)) {
            video_idx_ = UNKNOWN_INDEX;
            qDebug() << "[FFmpegDecode::start] open video codec err: " << error_info_;
        }
    } else {
        ret = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (ret < 0) {
            error_info_ = "[FFmpegDecode::start] Could not find video stream";
        } else {
            video_idx_ = ret;
            video_time_base_ = fmt_ctx_->streams[video_idx_]->time_base;
            if (!open_codec(fmt_ctx_->streams[video_idx_], &video_dec_ctx_)) {
                video_idx_ = UNKNOWN_INDEX;
                qDebug() << "[FFmpegDecode::start] open video codec err: " << error_info_;
            }
        }
    }

    if (UNKNOWN_INDEX != audio_index) {
        audio_idx_ = audio_index;
        audio_time_base_ = fmt_ctx_->streams[audio_idx_]->time_base;
        if (!open_codec(fmt_ctx_->streams[audio_idx_], &audio_dec_ctx_)) {
            audio_idx_ = UNKNOWN_INDEX;
            qDebug() << "[FFmpegDecode::start] open audio codec err: " << error_info_;
        }
    } else {
        ret = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (ret < 0) {
            if (!error_info_.isEmpty())
                error_info_ += " and audio stream";
            else
                error_info_ = "[FFmpegDecode::start] Could not find audio stream";
        } else {
            audio_idx_ = ret;
            audio_time_base_ = fmt_ctx_->streams[audio_idx_]->time_base;
            if (!open_codec(fmt_ctx_->streams[audio_idx_], &audio_dec_ctx_)) {
                audio_idx_ = UNKNOWN_INDEX;
                qDebug() << "[FFmpegDecode::start] open audio codec err: " << error_info_;
            }
        }
    }
    if (UNKNOWN_INDEX == video_idx_ && UNKNOWN_INDEX == audio_idx_)
        return false;

    pkt_ = av_packet_alloc();
    if (!pkt_) {
        error_info_ = "[FFmpegDecode::start] Could not allocate packet";
        return false;
    }

    frame_ = av_frame_alloc();
    if (!frame_) {
        error_info_ = "[FFmpegDecode::start] Could not allocate video frame";
        return false;
    }

    frame_audio_ = av_frame_alloc();
    if (!frame_audio_) {
        error_info_ = "[FFmpegDecode::start] Could not allocate audio frame";
        return false;
    }
    parse_summary_info(fmt_ctx_, summary_info_);
    return true;
}

void FFmpegDecode::stop()
{
    avcodec_free_context(&video_dec_ctx_);
    avcodec_free_context(&audio_dec_ctx_);
    avformat_close_input(&fmt_ctx_);
    av_packet_free(&pkt_);
    av_frame_free(&frame_);
    av_frame_free(&frame_audio_);
}

bool FFmpegDecode::get_packet(VideoPacketInfo &pkt_info)
{
    if (video_infos_.size() > 0) {
        pkt_info = video_infos_.first();
        video_infos_.removeFirst();
        return true;
    } else if (audio_infos_.size() > 0){
        pkt_info = audio_infos_.first();
        audio_infos_.removeFirst();
        return true;
    }

    int ret = av_read_frame(fmt_ctx_, pkt_);
    if (ret >= 0) {
        if (pkt_->stream_index != video_idx_ && pkt_->stream_index != audio_idx_) {
            pkt_info.media_type = UNKNOWN;
            return true;
        }
        if (pkt_->stream_index == video_idx_) {
            if (decode_video(pkt_)) {
                pkt_info = video_infos_.first();
                video_infos_.removeFirst();
            } else {
                pkt_info.media_type = UNKNOWN;
            }
        } else if (pkt_->stream_index == audio_idx_) {
            if (decode_audio(pkt_)) {
                pkt_info = audio_infos_.first();
                audio_infos_.removeFirst();
            } else {
                pkt_info.media_type = UNKNOWN;
            }
        }

        av_packet_unref(pkt_);
        return true;
    }
    return false;
}

bool FFmpegDecode::flush_packet(VideoPacketInfo &pkt_info)
{
    if (video_infos_.size() > 0) {
        pkt_info = video_infos_.first();
        video_infos_.removeFirst();
        return true;
    } else if (audio_infos_.size() > 0) {
        pkt_info = audio_infos_.first();
        audio_infos_.removeFirst();
        return true;
    }

    if (decode_video(nullptr)) {
        pkt_info = video_infos_.first();
        video_infos_.removeFirst();
        return true;
    } else if (decode_audio(nullptr)) {
        pkt_info = audio_infos_.first();
        audio_infos_.removeFirst();
        return true;
    }
    return false;
}

QString FFmpegDecode::get_error_info() const
{
    return error_info_;
}

long long FFmpegDecode::get_duration() const
{
    return duration_;
}

QString FFmpegDecode::get_summary_info() const
{
    return summary_info_;
}

bool FFmpegDecode::open_codec(AVStream *st, AVCodecContext **dec_ctx)
{
    int ret;
    const AVCodec *dec = NULL;
    QString media_type = av_get_media_type_string(st->codecpar->codec_type);

    /* find decoder for the stream */
    dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec) {
        error_info_ = "[FFmpegDecode::open_codec] Failed to find " + media_type + " codec";
        return false;
    }

    /* Allocate a codec context for the decoder */
    *dec_ctx = avcodec_alloc_context3(dec);
    if (!*dec_ctx) {
        error_info_ = "[FFmpegDecode::open_codec] Failed to allocate the " + media_type + " codec context";
        return false;
    }

    /* Copy codec parameters from input stream to output codec context */
    if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
        error_info_ = "[FFmpegDecode::open_codec] Failed to copy " + media_type + " codec parameters to decoder context";
        return false;
    }
    Q_UNUSED(ret);

    /* Init the decoders */
    if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
        error_info_ = "[FFmpegDecode::open_codec] Failed to open " + media_type + " codec";
        return false;
    }
    Q_UNUSED(ret);
    return true;
}

bool FFmpegDecode::decode_video(AVPacket *pkt)
{
    int ret = 0;
    bool got_frame = false;

    // submit the packet to the decoder
    ret = avcodec_send_packet(video_dec_ctx_, pkt);
    if (ret < 0) {
        char buf[256] = {0};
        av_strerror(ret, buf, 255);
        QString err_str(buf);
        error_info_ = "[FFmpegDecode::decode_video] Error submitting a packet for decoding: " + err_str;
        return false;
    }

    // get all the available frames from the decoder
    while (ret >= 0) {
        ret = avcodec_receive_frame(video_dec_ctx_, frame_);
        if (ret < 0) {
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return got_frame;

            char buf[256] = {0};
            av_strerror(ret, buf, 255);
            QString err_str(buf);
            error_info_ = "[FFmpegDecode::decode_video] Error during decoding: " + err_str;
            return false;
        }

        VideoPacketInfo info;
        info.media_type = VIDEO;
        info.frame_pict_type = frame_->pict_type;
        info.pkt_size = frame_->pkt_size;
        info.key_frame = frame_->flags & AV_FRAME_FLAG_KEY;

        if (0 != video_time_base_.den && AV_NOPTS_VALUE != frame_->pkt_dts) {
            info.pkt_dts = 1000.0 * frame_->pkt_dts * video_time_base_.num / video_time_base_.den;
        } else {
            info.pkt_dts = AV_NOPTS_VALUE;
        }

        if (0 != video_time_base_.den && AV_NOPTS_VALUE != frame_->pts) {
            info.frame_pts = 1000.0 * frame_->pts * video_time_base_.num / video_time_base_.den;
        } else {
            info.frame_pts = AV_NOPTS_VALUE;
        }
        video_infos_.append(info);
        got_frame = true;

        av_frame_unref(frame_);
    }

    return got_frame;
}

bool FFmpegDecode::decode_audio(AVPacket *pkt)
{
    int ret = 0;
    bool got_frame = false;

    // submit the packet to the decoder
    ret = avcodec_send_packet(audio_dec_ctx_, pkt);
    if (ret < 0) {
        char buf[256] = {0};
        av_strerror(ret, buf, 255);
        QString err_str(buf);
        error_info_ = "[FFmpegDecode::decode_audio] Error submitting a packet for decoding: " + err_str;
        return false;
    }

    // get all the available frames from the decoder
    while (ret >= 0) {
        ret = avcodec_receive_frame(audio_dec_ctx_, frame_audio_);
        if (ret < 0) {
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return got_frame;

            char buf[256] = {0};
            av_strerror(ret, buf, 255);
            QString err_str(buf);
            error_info_ = "[FFmpegDecode::decode_audio] Error during decoding: " + err_str;
            return false;
        }

        VideoPacketInfo info;
        info.media_type = AUDIO;
        info.frame_pict_type  = frame_audio_->pict_type;
        info.pkt_size = frame_audio_->pkt_size;
        info.key_frame = frame_audio_->flags & AV_FRAME_FLAG_KEY;

        if (0 != audio_time_base_.den && AV_NOPTS_VALUE != frame_audio_->pkt_dts) {
            info.pkt_dts = 1000.0 * frame_audio_->pkt_dts * audio_time_base_.num / audio_time_base_.den;
        } else {
            info.pkt_dts = AV_NOPTS_VALUE;
        }

        if (0 != audio_time_base_.den && AV_NOPTS_VALUE != frame_audio_->pts) {
            info.frame_pts = 1000.0 * frame_audio_->pts * audio_time_base_.num / audio_time_base_.den;
        } else {
            info.frame_pts = AV_NOPTS_VALUE;
        }
        audio_infos_.append(info);
        got_frame = true;

        av_frame_unref(frame_audio_);
    }

    return got_frame;
}

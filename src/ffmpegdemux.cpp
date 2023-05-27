#include "ffmpegdemux.h"

FFmpegDemux::FFmpegDemux(const QString & url)
    : url_(url)
    , fmt_ctx_(nullptr)
    , pkt_(nullptr)
    , video_idx_(UNKNOWN_INDEX)
    , audio_idx_(UNKNOWN_INDEX)
    , duration_(0)
{
    video_time_base_.num = 0;
    video_time_base_.den = 0;
    audio_time_base_.num = 0;
    audio_time_base_.den = 0;
}

FFmpegDemux::~FFmpegDemux()
{

}

bool FFmpegDemux::start(int video_index, int audio_index)
{
    error_info_.clear();
    /* open input file, and allocate format context */
    std::string tmp(url_.toLocal8Bit().data());
    const char *file_path = tmp.c_str();

    if (avformat_open_input(&fmt_ctx_, file_path, nullptr, nullptr) < 0) {
        error_info_ = "[FFmpegDemux::start] Could not open source file: " + url_;
        return false;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
        error_info_ = "[FFmpegDemux::start] Could not find stream information";
        return false;
    }
    duration_ = fmt_ctx_->duration * 1000 / AV_TIME_BASE;

    /* dump input information to stderr */
    //av_dump_format(fmt_ctx_, 0, file_path, 0);

    int ret = 0;
    if (UNKNOWN_INDEX != video_index) {
        video_idx_ = video_index;
        video_time_base_ = fmt_ctx_->streams[video_idx_]->time_base;
    } else {
        ret = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (ret < 0) {
            error_info_ = "[FFmpegDemux::start] Could not find video stream";
        } else {
            video_idx_ = ret;
            video_time_base_ = fmt_ctx_->streams[video_idx_]->time_base;
        }
    }

    if (UNKNOWN_INDEX != audio_index) {
        audio_idx_ = audio_index;
        audio_time_base_ = fmt_ctx_->streams[audio_idx_]->time_base;
    } else {
        ret = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (ret < 0) {
            if (!error_info_.isEmpty())
                error_info_ += " and audio stream";
            else
                error_info_ = "[FFmpegDemux::start] Could not find audio stream";
        } else {
            audio_idx_ = ret;
            audio_time_base_ = fmt_ctx_->streams[audio_idx_]->time_base;
        }
    }
    if (UNKNOWN_INDEX == video_idx_ && UNKNOWN_INDEX == audio_idx_)
        return false;

    pkt_ = av_packet_alloc();
    if (!pkt_) {
        error_info_ = "[FFmpegDemux::start] Could not allocate packet";
        return false;
    }

    parse_summary_info(fmt_ctx_, summary_info_);
    return true;
}

void FFmpegDemux::stop()
{
    avformat_close_input(&fmt_ctx_);
    av_packet_free(&pkt_);
}

bool FFmpegDemux::get_packet(VideoPacketInfo &pkt_info)
{
    int ret = av_read_frame(fmt_ctx_, pkt_);

    int num  = 0;
    int den = 0;
    if (ret >= 0) {
        if (pkt_->stream_index != video_idx_ && pkt_->stream_index != audio_idx_) {
            pkt_info.media_type = UNKNOWN;
            return true;
        }

        if (pkt_->stream_index == video_idx_) {
            pkt_info.media_type = VIDEO;
            num = video_time_base_.num;
            den = video_time_base_.den;
        } else if (pkt_->stream_index == audio_idx_) {
            pkt_info.media_type = AUDIO;
            num = audio_time_base_.num;
            den = audio_time_base_.den;
        }
        pkt_info.pkt_size = pkt_->size;

        if (0 != den) {
            if (pkt_->dts != AV_NOPTS_VALUE)
                pkt_info.pkt_dts = 1000.0 * pkt_->dts * num / den;

            if (pkt_->pts != AV_NOPTS_VALUE)
                pkt_info.pkt_pts = 1000.0 * pkt_->pts * num / den;
        }

        av_packet_unref(pkt_);
        return true;
    } else {
        return false;
    }
}

QString FFmpegDemux::get_error_info() const
{
    return error_info_;
}

long long FFmpegDemux::get_duration() const
{
    return duration_;
}

QString FFmpegDemux::get_summary_info() const
{
    return summary_info_;
}

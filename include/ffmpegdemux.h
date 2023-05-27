#ifndef FFMPEGDEMUX_H
#define FFMPEGDEMUX_H

#include <QString>
#include <QDebug>
#include "ffmpegcommon.h"

class FFmpegDemux
{
public:
    FFmpegDemux(const QString & url);
    ~FFmpegDemux();

    bool start(int video_index, int audio_index);
    void stop();

    bool get_packet(VideoPacketInfo & pkt_info);
    QString get_error_info() const;
    long long get_duration() const;
    QString get_summary_info() const;

private:
    QString url_;
    AVFormatContext *fmt_ctx_;
    AVPacket *pkt_;

    int video_idx_;
    int audio_idx_;
    AVRational video_time_base_;
    AVRational audio_time_base_;

    QString error_info_;
    long long duration_;
    QString summary_info_;
};

#endif // FFMPEGDEMUX_H

#ifndef FFMPEGDECODE_H
#define FFMPEGDECODE_H

#include <QString>
#include <QDebug>
#include "ffmpegcommon.h"

class FFmpegDecode
{
public:
    FFmpegDecode(const QString & url);
    ~FFmpegDecode();

    bool start(int video_index, int audio_index);
    void stop();

    bool get_packet(VideoPacketInfo &pkt_info);
    bool flush_packet(VideoPacketInfo &pkt_info);

    QString get_error_info() const;
    long long get_duration() const;
    QString get_summary_info() const;

private:
    bool open_codec(AVStream *st, AVCodecContext **dec_ctx);
    bool decode_video(AVPacket *pkt);

    bool decode_audio(AVPacket *pkt);

private:
    QString url_;
    AVFormatContext *fmt_ctx_;
    AVCodecContext *video_dec_ctx_;
    AVCodecContext *audio_dec_ctx_;
    AVPacket *pkt_;
    AVFrame *frame_;
    AVFrame *frame_audio_;

    int video_idx_;
    int audio_idx_;
    AVRational video_time_base_;
    AVRational audio_time_base_;

    QString error_info_;
    QList<VideoPacketInfo> video_infos_;
    QList<VideoPacketInfo> audio_infos_;
    long long duration_;
    QString summary_info_;
};

#endif // FFMPEGDECODE_H

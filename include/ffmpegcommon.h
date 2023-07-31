#ifndef FFMPEGCOMMON_H
#define FFMPEGCOMMON_H

#include <QMetaType>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <iostream>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

enum MediaType {
    UNKNOWN,
    VIDEO,
    AUDIO
};

enum StreamIndex {
    UNKNOWN_INDEX = -1
};

struct VideoPacketInfo {
    VideoPacketInfo()
        : media_type(UNKNOWN)
        , pkt_dts(AV_NOPTS_VALUE)
        , pkt_pts(AV_NOPTS_VALUE)
        , pkt_size(-1)
        , frame_pts(AV_NOPTS_VALUE)
        , frame_pict_type(AV_PICTURE_TYPE_NONE)
        , key_frame(0)
        , diff_dts(0)
        , diff_pts(0)
    {}
    int media_type; // 1:video 2:audio

    // packet info
    double pkt_dts;
    double pkt_pts;
    int pkt_size;

    // frame info
    double frame_pts;
    int frame_pict_type;
    int key_frame;

    // diff infos
    double diff_dts;
    double diff_pts;
};

// need on windows
Q_DECLARE_METATYPE(VideoPacketInfo);

QString size_human(quint64 size);

void parse_summary_info(AVFormatContext *ic, QString &summary_info);

QString analyse(const QString &url, QList<QString> &video_tracks, QList<QString> &audio_tracks);

std::string QString2string(const QString & qstr);
QString string2QString(const std::string & str);

#endif // FFMPEGCOMMON_H

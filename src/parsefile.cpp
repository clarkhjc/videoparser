#include "parsefile.h"

ParseFile::ParseFile(const QString &file_path, const QString &parse_mode,
    int video_index, int audio_index, QObject *parent)
    : QThread(parent)
    , file_path_(file_path)
    , parse_mode_(parse_mode)
    , video_index_(video_index)
    , audio_index_(audio_index)
    , duration_(0)
    , packet_dts_(0)
{
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &ParseFile::on_timer);
    timer_->start(1000);
}

void ParseFile::stop()
{
    if (timer_->isActive())
        timer_->stop();

    requestInterruption();
    wait();
}

void ParseFile::on_timer()
{
    qDebug() << "packet_dts_: " << packet_dts_ << "duration: " << duration_;
    if (duration_) {
        double percent = 100.0 * packet_dts_ / duration_;
        QString str_percent = QString::number(percent, 'f', 2) + "%";
        emit sig_progress(str_percent);
    }
}

void ParseFile::run()
{
    if (parse_mode_.isEmpty() || parse_mode_ == "packet") {
        parse_file_packets();
    } else {
        parse_file_frames();
    }
}

void ParseFile::parse_file_packets()
{
    QString err_info;
    bool is_interrupt = false;

    int row_v = 0, row_a = 0;
    double last_video_dts = 0, last_video_pts = 0;
    double last_audio_dts = 0, last_audio_pts = 0;

    FFmpegDemux ff_demux(file_path_);
    bool ret = ff_demux.start(video_index_, audio_index_);
    if (ret) {
        summary_info_ = ff_demux.get_summary_info();
        duration_ = ff_demux.get_duration();
        VideoPacketInfo info;
        while (ff_demux.get_packet(info)) {
            if (VIDEO == info.media_type) {
                double diff_dts = info.pkt_dts - last_video_dts;
                double diff_pts = info.pkt_pts - last_video_pts;
                if (0 == row_v++) {
                    diff_dts = 0;
                    diff_pts = 0;
                }
                last_video_dts = info.pkt_dts;
                last_video_pts = info.pkt_pts;

                info.diff_dts = diff_dts;
                info.diff_pts = diff_pts;
                infos_.append(info);

                packet_dts_ = info.pkt_dts;
            } else if (AUDIO == info.media_type) {
                double diff_dts = info.pkt_dts - last_audio_dts;
                double diff_pts = info.pkt_pts - last_audio_pts;
                if (0 == row_a++) {
                    diff_dts = 0;
                    diff_pts = 0;
                }
                last_audio_dts = info.pkt_dts;
                last_audio_pts = info.pkt_pts;

                info.diff_dts = diff_dts;
                info.diff_pts = diff_pts;
                infos_.append(info);
            }

            if (QThread::currentThread()->isInterruptionRequested()) {
                is_interrupt = true;
                break;
            }
        }
    } else {
        err_info = ff_demux.get_error_info();
    }
    ff_demux.stop();

    // send signal
    QList<VideoPacketInfo> empty_info;
    if (is_interrupt) {
        emit sig_result("interrupt", parse_mode_, file_path_, empty_info, summary_info_);
    } else if (!err_info.isEmpty()) {
        emit sig_result(err_info, parse_mode_, file_path_, empty_info, summary_info_);
    } else {
        emit sig_result("ok", parse_mode_, file_path_, infos_, summary_info_);
    }
}

void ParseFile::parse_file_frames()
{
    QString err_info;
    bool is_interrupt = false;

    FFmpegDecode ff_decode(file_path_);
    bool ret = ff_decode.start(video_index_, audio_index_);
    if (ret) {
        summary_info_ = ff_decode.get_summary_info();
        duration_ = ff_decode.get_duration();
        VideoPacketInfo info;
        while (ff_decode.get_packet(info) || ff_decode.flush_packet(info)) {
            if (VIDEO == info.media_type) {
                infos_.append(info);

                packet_dts_ = info.pkt_dts;
            } else if (AUDIO == info.media_type) {
                infos_.append(info);
            }

            if (QThread::currentThread()->isInterruptionRequested()) {
                is_interrupt = true;
                break;
            }
        }
    } else {
        err_info = ff_decode.get_error_info();
    }
    ff_decode.stop();

    // send signal
    QList<VideoPacketInfo> empty_info;
    if (is_interrupt) {
        emit sig_result("interrupt", parse_mode_, file_path_, empty_info, summary_info_);
    } else if (!err_info.isEmpty()) {
        emit sig_result(err_info, parse_mode_, file_path_, empty_info, summary_info_);
    } else {
        emit sig_result("ok", parse_mode_, file_path_, infos_, summary_info_);
    }
}

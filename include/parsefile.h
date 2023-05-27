#ifndef _PARSE_FILE_H_
#define _PARSE_FILE_H_

#include <QDebug>
#include <QObject>
#include <QThread>
#include <QProcess>
#include <QTimer>

#include "ffmpegdemux.h"
#include "ffmpegdecode.h"

class ParseFile : public QThread
{
    Q_OBJECT

public:
    ParseFile(const QString &file_path, const QString &parse_mode,
              int video_index, int audio_index,
              QObject *parent = nullptr);
    ~ParseFile() = default;

    void stop();

public slots:
    void on_timer(void);

protected:
    void run() override;

private:
    void parse_file_packets();
    void parse_file_frames();
    void get_summary_info();

signals:
    void sig_progress(const QString progress);
    void sig_result(const QString res, const QString parse_mode,
                    const QString file_path, QList<VideoPacketInfo> &infos, const QString summary);

private:
    QString file_path_;
    QString parse_mode_;
    int video_index_;
    int audio_index_;
    QList<VideoPacketInfo> infos_;

    long long duration_;
    long long packet_dts_;
    QTimer *timer_;
    QString summary_info_;
};
#endif

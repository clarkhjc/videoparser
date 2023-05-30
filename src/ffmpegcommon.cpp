#include "ffmpegcommon.h"

QString size_human(quint64 size)
{
    float num = size;
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while(num >= 1024.0 && i.hasNext())
    {
     unit = i.next();
     num /= 1024.0;
    }
    return QString().setNum(num,'f',2)+" "+unit;
}

static void print_fps(QString &info, double d, const char *postfix)
{
    uint64_t v = lrintf(d * 100);
    if (!v)
        info += QString("%1").arg(d, 1, 'f', 4) + QString(postfix);
    else if (v % 100)
        info += QString("%1").arg(d, 3, 'f', 2) + QString(postfix);
    else if (v % (100 * 1000))
        info += QString("%1").arg(d, 1, 'f', 0) + QString(postfix);
    else
        info += QString("%1").arg(d / 1000, 1, 'f', 0) + "k " + QString(postfix);
}

void parse_summary_info(AVFormatContext *ic, QString &summary_info)
{
    QString prefix_spaces = "&nbsp;&nbsp;&nbsp;&nbsp;";
#ifdef Q_OS_WIN
    prefix_spaces = "&nbsp;&nbsp;";
#endif

    QFileInfo file_info(ic->url);
    summary_info = "<h2>basic information:</h2><br />";
    summary_info += prefix_spaces + "<b>File path:</b> " + QString(ic->url) + "<br /><br />";
    summary_info += prefix_spaces + "<b>File format:</b> " + QString(ic->iformat->name) + "<br /><br />";

    QString file_size = size_human(file_info.size());
    summary_info += prefix_spaces + "<b>File size:</b> " + file_size + "<br /><br />";
    if (ic) {
        summary_info += prefix_spaces + "<b>Duration:</b> ";
        if (ic->duration != AV_NOPTS_VALUE) {
            int64_t hours, mins, secs, us;
            int64_t duration = ic->duration + (ic->duration <= INT64_MAX - 5000 ? 5000 : 0);
            secs  = duration / AV_TIME_BASE;
            us    = duration % AV_TIME_BASE;
            mins  = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;

            summary_info += QString("%1").arg(hours, 2, 10, QLatin1Char('0')) + ":";
            summary_info += QString("%1").arg(mins, 2, 10, QLatin1Char('0'))  + ":";
            summary_info += QString("%1").arg(secs, 2, 10, QLatin1Char('0'))  + ".";
            summary_info += QString::number((100 * us) / AV_TIME_BASE);
        } else {
            summary_info += "N/A";
        }

        if (ic->start_time != AV_NOPTS_VALUE) {
            int secs, us;
            summary_info += " start: ";
            secs = llabs(ic->start_time / AV_TIME_BASE);
            us   = llabs(ic->start_time % AV_TIME_BASE);

            QString pre_str = ic->start_time >= 0 ? "" : "-";
            summary_info += pre_str;
            summary_info += QString::number(secs) + ".";
            QString us_str = QString("%1").arg((int)av_rescale(us, 1000000, AV_TIME_BASE), 6, 10, QLatin1Char('0'));
            summary_info += us_str;
            summary_info += " ";
        }
        summary_info += "bitrate:";
        if (ic->bit_rate)
            summary_info += QString::number(ic->bit_rate / 1000) + " kb/s";
        else
            summary_info += "N/A";

        summary_info += "<h2>stream information:</h2>";
        for (unsigned int i = 0; i < ic->nb_streams; i++) {
            char buf[256];
            int flags = ic->iformat->flags;
            const AVStream *st = ic->streams[i];
            const AVDictionaryEntry *lang = av_dict_get(st->metadata, "language", NULL, 0);
            AVCodecContext *avctx;
            int ret;

            avctx = avcodec_alloc_context3(NULL);
            if (!avctx)
                return;

            ret = avcodec_parameters_to_context(avctx, st->codecpar);
            if (ret < 0) {
                avcodec_free_context(&avctx);
                return;
            }

            avcodec_string(buf, sizeof(buf), avctx, 0);
            avcodec_free_context(&avctx);

            summary_info += prefix_spaces +"Stream #" + QString::number(i);
            if (flags & AVFMT_SHOW_IDS)
                summary_info += "[0x" + QString().setNum(st->id, 16) + "]";
            if (lang) {
                summary_info += "(" + QString(lang->value) + ")";
            }
            summary_info += ": ";
            summary_info += QString(buf);

            if (st->sample_aspect_ratio.num &&
                av_cmp_q(st->sample_aspect_ratio, st->codecpar->sample_aspect_ratio)) {
                AVRational display_aspect_ratio;
                av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
                          st->codecpar->width  * (int64_t)st->sample_aspect_ratio.num,
                          st->codecpar->height * (int64_t)st->sample_aspect_ratio.den,
                          1024 * 1024);
                summary_info += ", SAR " + QString::number(st->sample_aspect_ratio.num) + ":" + QString::number(st->sample_aspect_ratio.den)
                        + " DAR " + QString::number(display_aspect_ratio.num) + ":" + QString::number(display_aspect_ratio.den);
            }
            if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                int fps = st->avg_frame_rate.den && st->avg_frame_rate.num;
                int tbr = st->r_frame_rate.den && st->r_frame_rate.num;
                int tbn = st->time_base.den && st->time_base.num;

                summary_info += ", ";
                if (fps)
                    print_fps(summary_info, av_q2d(st->avg_frame_rate), tbr || tbn ? "fps, " : "fps");
                if (tbr)
                    print_fps(summary_info, av_q2d(st->r_frame_rate), tbn ? "tbr, " : "tbr");
                if (tbn)
                    print_fps(summary_info, 1 / av_q2d(st->time_base), "tbn");
            }
            summary_info += "<br />";
            summary_info += "<br />";
        }
    }
}

QString analyse(const QString &url, QList<QString> &video_tracks, QList<QString> &audio_tracks)
{
    QString error_info;

    /* open input file, and allocate format context */
    std::string tmp(url.toLocal8Bit().data());
    const char *file_path = tmp.c_str();

    AVFormatContext *fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, file_path, nullptr, nullptr) < 0) {
        error_info = "[analyse] Could not open source file: " + url;
        return error_info;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        error_info = "[analyse] Could not find stream information";
        return error_info;
    }

    /* get all streams information*/
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        AVStream *st = fmt_ctx->streams[i];

        char buf[256];
        AVCodecContext *avctx = nullptr;
        avctx = avcodec_alloc_context3(NULL);
        avcodec_parameters_to_context(avctx, st->codecpar);
        avcodec_string(buf, sizeof(buf), avctx, 0);
        avcodec_free_context(&avctx);

        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_tracks.push_back(QString::number(st->index) + ":" + QString(buf));
        } else if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_tracks.push_back(QString::number(st->index) + ":" + QString(buf));
        }
    }

    avformat_close_input(&fmt_ctx);
    return error_info;
}

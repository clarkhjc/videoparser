#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QElapsedTimer>
#include "parsefile.h"
#include "usersettings.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void analyse_file();
    void parse_file();
    void select_file();
    void stop_parse();

    void receive_parse_progress(const QString progress);
    void receive_parse_result(const QString res, const QString parse_mode,
                              const QString file_path, QList<VideoPacketInfo> &infos, const QString summary);

private:
    void set_table_style();
    void set_menu_bar();
    void set_tool_bar();

private:
    void clear_packet_tables();
    void clear_frame_tables();
    void add_row_for_video(int row, const VideoPacketInfo & info);
    void add_row_for_audio(int row, const VideoPacketInfo & info);

    void add_row_for_video_frame(int row, const VideoPacketInfo & info);
    void add_row_for_audio_frame(int row, const VideoPacketInfo & info);

    void update_recent_action_list(const QString &file_path);

private:
    Ui::MainWindow *ui;
    int max_recent_nums_;
    QMenu *recent_files_menu_;
    QList<QAction*> recent_actions_;

    int last_video_packet_cnts_;
    int last_audio_packet_cnts_;

    int last_video_frame_cnts_;
    int last_audio_frame_cnts_;

    QAction *packet_mode_;
    QAction *frame_mode_;

    ParseFile *parse_file_;
};

#endif // MAINWINDOW_H

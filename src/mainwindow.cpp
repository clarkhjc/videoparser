#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    max_recent_nums_(4),
    recent_files_menu_(nullptr),
    last_video_packet_cnts_(0),
    last_audio_packet_cnts_(0),
    last_video_frame_cnts_(0),
    last_audio_frame_cnts_(0),
    parse_file_(nullptr)
{
    ui->setupUi(this);
#ifdef Q_OS_MAC
    setWindowIcon(QIcon(":/img/logo.icns"));
#else
    setWindowIcon(QIcon(":/img/logo.png"));
#endif
    setWindowTitle(tr("Video Parser"));

    packet_mode_ = new QAction(tr("packet"), this);
    frame_mode_ = new QAction(tr("frame"), this);

    set_menu_bar();
    set_tool_bar();
    set_table_style();

    update_recent_action_list("");

    ui->btn_analyse->setToolTip(tr("analyse video and audio tracks"));
    ui->btn_start->setToolTip(tr("extract timestamps and show"));
    ui->btn_analyse->setStyleSheet("QPushButton {background-color: green;}");
    ui->btn_start->setStyleSheet("QPushButton {background-color: green;}");
    connect(ui->btn_analyse, &QPushButton::clicked, this, &MainWindow::analyse_file);
    connect(ui->btn_start, &QPushButton::clicked, this, &MainWindow::parse_file);

    ui->lb_progress->setVisible(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::analyse_file()
{
    ui->cb_video_tracks->clear();
    ui->cb_audio_tracks->clear();

    QList<QString> video_tracks;
    QList<QString> audio_tracks;
    QString err_info = analyse(ui->le_url->text(), video_tracks, audio_tracks);
    if (err_info.isEmpty()) {
        for (auto & item : video_tracks) {
            ui->cb_video_tracks->addItem(item);
        }
        for (auto & item : audio_tracks) {
            ui->cb_audio_tracks->addItem(item);
        }
    } else {
        QMessageBox::warning(nullptr, "warning", err_info);
    }
}

void MainWindow::parse_file()
{
    clear_packet_tables();
    clear_frame_tables();

    QString parse_mode = get_user_settings().get_parse_mode();

    int video_index = UNKNOWN_INDEX, audio_index = UNKNOWN_INDEX;
    QString video_text = ui->cb_video_tracks->currentText();
    QString audio_text = ui->cb_audio_tracks->currentText();

    QStringList video_text_list;
    QStringList audio_text_list;
    video_text_list = video_text.split(":", Qt::SkipEmptyParts);
    audio_text_list = audio_text.split(":", Qt::SkipEmptyParts);

    video_index = video_text_list.size() > 0 ? video_text_list[0].toInt(): -1;
    audio_index = audio_text_list.size() > 0 ? audio_text_list[0].toInt(): -1;

    if (parse_file_) {
        delete parse_file_;
        parse_file_ = nullptr;
    }
    parse_file_ = new ParseFile(ui->le_url->text(), parse_mode, video_index, audio_index);
    connect(parse_file_, &ParseFile::sig_progress, this, &MainWindow::receive_parse_progress);
    connect(parse_file_, &ParseFile::sig_result, this, &MainWindow::receive_parse_result);
    parse_file_->start();

    ui->lb_progress->setVisible(true);
    ui->btn_start->setText("Stop");
    ui->btn_start->setStyleSheet("QPushButton {background-color: red;}");
    disconnect(ui->btn_start, &QPushButton::clicked, this, &MainWindow::parse_file);
    connect(ui->btn_start, &QPushButton::clicked, this, &MainWindow::stop_parse);
}

void MainWindow::select_file()
{
    QStringList files;
    QFileDialog dialog(this, tr("open file"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setModal(true);
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    if (dialog.exec())
        files = dialog.selectedFiles();

    QString file_path;
    if (files.size())
        file_path = files.first();

    if (!file_path.isEmpty()) {
        ui->le_url->setText(file_path);
    }
}

void MainWindow::stop_parse()
{
    if (parse_file_)
        parse_file_->stop();
}

void MainWindow::receive_parse_progress(const QString progress)
{
    ui->lb_progress->setText(progress);
}

void MainWindow::receive_parse_result(const QString res, const QString parse_mode,
                                      QString file_path, QList<VideoPacketInfo> &infos, const QString summary)
{
    if (res == "ok") {
        if (parse_mode.isEmpty() || parse_mode == "packet") {
            int row_v = 0, row_a = 0;
            for (auto &info : infos) {
                if (info.media_type == VIDEO) {
                    add_row_for_video(row_v++, info);
                } else if (info.media_type == AUDIO) {
                    add_row_for_audio(row_a++, info);
                }
            }
        } else {
            int row_v = 0, row_a = 0;
            for (auto &info : infos) {
                if (info.media_type == VIDEO) {
                    add_row_for_video_frame(row_v++, info);
                } else if (info.media_type == AUDIO) {
                    add_row_for_audio_frame(row_a++, info);
                }
            }
        }
        ui->textBrowser->setText(summary);
        update_recent_action_list(file_path);
    } else if (res == "interrupt") {
        QMessageBox::warning(nullptr, "warning", "The user canceled the operation");
    } else {
        QMessageBox::warning(nullptr, "warning", res);
    }

    ui->btn_start->setText(tr("Start"));
    ui->btn_start->setStyleSheet("QPushButton {background-color: green;}");
    disconnect(ui->btn_start, &QPushButton::clicked, this, &MainWindow::stop_parse);
    connect(ui->btn_start, &QPushButton::clicked, this, &MainWindow::parse_file);

    ui->lb_progress->setVisible(false);
    ui->lb_progress->setText("0%");
    stop_parse();
}

void MainWindow::update_recent_action_list(const QString &file_path)
{
    qDebug() << "[update_recent_action_list] start: " << file_path;
    QStringList recent_file_paths = get_user_settings().get_recent_files();

    if (!file_path.isEmpty()) {
        recent_file_paths.removeAll(file_path);
        recent_file_paths.prepend(file_path);
    }
    while (recent_file_paths.size() > max_recent_nums_)
           recent_file_paths.removeLast();
    get_user_settings().set_recent_files(recent_file_paths);

    int end = 0;
    if (recent_file_paths.size() <= max_recent_nums_)
        end = recent_file_paths.size();
    else
        end = max_recent_nums_;

    for (auto i = 0; i < end; ++i) {
        QString stripped_name = QFileInfo(recent_file_paths.at(i)).fileName();
        qDebug() << "[update_recent_action_list] stripped_name: " << stripped_name;
        recent_actions_.at(i)->setText(stripped_name);
        recent_actions_.at(i)->setData(recent_file_paths.at(i));
        recent_actions_.at(i)->setVisible(true);
    }
    for (auto i = end; i < max_recent_nums_; ++i) {
        recent_actions_.at(i)->setVisible(false);
    }

    if (!file_path.isEmpty()) {
        ui->le_url->setText(file_path);
    }

    qDebug() << "[update_recent_action_list] end";
}

void MainWindow::set_table_style()
{
    // set packet tables
    ui->tw_video_packet->setColumnCount(5);
    ui->tw_video_packet->setHorizontalHeaderLabels(QStringList() << "Packet num" << "Size" << "DTS(ms)" << "DTS Diff(ms)" << "PTS(ms)");
    ui->tw_video_packet->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tw_video_packet->setAlternatingRowColors(true);
    ui->tw_video_packet->setPalette(QPalette(QColor("#9dd3a8")));

    ui->tw_audio_packet->setColumnCount(5);
    ui->tw_audio_packet->setHorizontalHeaderLabels(QStringList() << "Packet num" << "Size" << "DTS(ms)" << "DTS Diff(ms)" << "PTS(ms)");
    ui->tw_audio_packet->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tw_audio_packet->setAlternatingRowColors(true);
    ui->tw_audio_packet->setPalette(QPalette(QColor("#348498")));

    // set frame tables
    ui->tw_video_frame->setColumnCount(4);
    ui->tw_video_frame->setHorizontalHeaderLabels(QStringList() << "Frame num" << "Size" << "PTS(ms)" << "PICT TYPE");
    ui->tw_video_frame->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tw_video_frame->setAlternatingRowColors(true);
    ui->tw_video_frame->setPalette(QPalette(QColor("#9dd3a8")));

    ui->tw_audio_frame->setColumnCount(4);
    ui->tw_audio_frame->setHorizontalHeaderLabels(QStringList() << "Frame num" << "Size" << "PTS(ms)" << "PICT TYPE");
    ui->tw_audio_frame->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tw_audio_frame->setAlternatingRowColors(true);
    ui->tw_audio_frame->setPalette(QPalette(QColor("#348498")));

    QString parse_mode = get_user_settings().get_parse_mode();
    if (parse_mode.isEmpty() || parse_mode == "packet") {
        ui->tw_video_packet->setVisible(true);
        ui->tw_audio_packet->setVisible(true);

        ui->tw_video_frame->setVisible(false);
        ui->tw_audio_frame->setVisible(false);
    } else {
        ui->tw_video_packet->setVisible(false);
        ui->tw_audio_packet->setVisible(false);

        ui->tw_video_frame->setVisible(true);
        ui->tw_audio_frame->setVisible(true);
    }
}

void MainWindow::set_menu_bar()
{
    QMenuBar *menu_bar = new QMenuBar(this);

    /*
     * QMenuBar
     * ****QMenu(File)
     * ********QAction(Open)
     * ********QMenu(Recent files)
     * ************QAction(file1)
     * ************QAction(file2)
     * ************QAction(file3)
     * ************QAction(file4)
     * ****QMenu(Set)
     * ********QMenu(Parse mode)
     * ************QAction(packet mode)
     * ************QAction(frame mode)
     * ****QMenu(Help)
     * ********QAction(关于QT)
    */

    QMenu *file_menu = new QMenu(tr("File"), this);

    QAction *open_action = new QAction(tr("Open"));
    open_action->setShortcut(QKeySequence::Open);
    file_menu->addAction(open_action);
    connect(open_action, &QAction::triggered, this, &MainWindow::select_file);

    recent_files_menu_ = new QMenu(tr("Recent files"), this);
    for (auto i = 0; i < max_recent_nums_; ++i) {
        QAction *recent_action = new QAction(this);
        recent_action->setVisible(false);
        connect(recent_action, &QAction::triggered, this, [=]() {
            QString file_path = recent_action->data().toString();
            ui->le_url->setText(file_path);
        });
        recent_actions_.append(recent_action);
    }
    file_menu->addMenu(recent_files_menu_);
    for (auto i = 0; i < max_recent_nums_; ++i) {
        recent_files_menu_->addAction(recent_actions_.at(i));
    }

    QMenu *edit_menu = new QMenu(tr("Edit"), this);
    QAction *cut_action = new QAction(tr("cut"));
    cut_action->setShortcut(QKeySequence::Cut);
    QAction *copy_action = new QAction(tr("copy"));
    copy_action->setShortcut(QKeySequence::Copy);
    QAction *paste_action = new QAction(tr("paste"));
    paste_action->setShortcut(QKeySequence::Paste);
    edit_menu->addAction(cut_action);
    edit_menu->addAction(copy_action);
    edit_menu->addAction(paste_action);

    QMenu *set_menu = new QMenu(tr("Set"), this);
    QMenu *parse_mode_menu = new QMenu(tr("Parse mode"), this);

    connect(packet_mode_, &QAction::triggered, this, [=]() {
        get_user_settings().set_parse_mode("packet");
        packet_mode_->setIcon(QIcon(":/img/checked.png"));
        frame_mode_->setIcon(QIcon(""));

        clear_packet_tables();
        ui->textBrowser->clear();
        ui->tw_video_packet->setVisible(true);
        ui->tw_audio_packet->setVisible(true);
        ui->tw_video_frame->setVisible(false);
        ui->tw_audio_frame->setVisible(false);
    });

    connect(frame_mode_, &QAction::triggered, this, [=]() {
        get_user_settings().set_parse_mode("frame");
        frame_mode_->setIcon(QIcon(":/img/checked.png"));
        packet_mode_->setIcon(QIcon(""));

        clear_frame_tables();
        ui->textBrowser->clear();
        ui->tw_video_packet->setVisible(false);
        ui->tw_audio_packet->setVisible(false);
        ui->tw_video_frame->setVisible(true);
        ui->tw_audio_frame->setVisible(true);
    });

    QString parse_mode = get_user_settings().get_parse_mode();
    if (parse_mode.isEmpty() || parse_mode == "packet") {
        packet_mode_->setIcon(QIcon(":/img/checked.png"));
        frame_mode_->setIcon(QIcon(""));
    } else {
        frame_mode_->setIcon(QIcon(":/img/checked.png"));
        packet_mode_->setIcon(QIcon(""));
    }
    parse_mode_menu->addAction(packet_mode_);
    parse_mode_menu->addAction(frame_mode_);
    set_menu->addMenu(parse_mode_menu);

    QMenu *help_menu = new QMenu(tr("Help"), this);
    QAction *about_qt_action = new QAction(tr("Qt info"), this);
    connect(about_qt_action, &QAction::triggered, this, [=]() {
        QMessageBox::aboutQt(nullptr, "");
    });
    help_menu->addAction(about_qt_action);

    menu_bar->addMenu(file_menu);
    menu_bar->addMenu(edit_menu);
    menu_bar->addMenu(set_menu);
    menu_bar->addMenu(help_menu);

    setMenuBar(menu_bar);
}

void MainWindow::set_tool_bar()
{
    QToolBar *tool_bar = new QToolBar(this);
    addToolBar(Qt::TopToolBarArea, tool_bar);

    tool_bar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    tool_bar->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);

    tool_bar->setFloatable(false);
    tool_bar->setMovable(false);

    QAction *open_action = new QAction(QIcon(":/img/add.png"), "");
    open_action->setToolTip(tr("select a video or audio file"));
    tool_bar->addAction(open_action);
    connect(open_action, &QAction::triggered, this, &MainWindow::select_file);
}

void MainWindow::clear_frame_tables()
{
    last_video_frame_cnts_ = ui->tw_video_frame->rowCount();
    last_audio_frame_cnts_ = ui->tw_audio_frame->rowCount();
    qDebug() << "[clear_frame_tables] video table rows: " << last_video_frame_cnts_;
    qDebug() << "[clear_frame_tables] audio table rows: " << last_audio_frame_cnts_;

    QElapsedTimer time;
    time.start();
    ui->tw_video_frame->clearContents();
    ui->tw_audio_frame->clearContents();
    qDebug() << "[clear_frame_tables] cost: " << time.elapsed() << " ms";
}

void MainWindow::clear_packet_tables()
{
    last_video_packet_cnts_ = ui->tw_video_packet->rowCount();
    last_audio_packet_cnts_ = ui->tw_audio_packet->rowCount();
    qDebug() << "[clear_packet_tables] video table rows: " << last_video_packet_cnts_;
    qDebug() << "[clear_packet_tables] audio table rows: " << last_audio_packet_cnts_;

    QElapsedTimer time;
    time.start();
    ui->tw_video_packet->clearContents();
    ui->tw_audio_packet->clearContents();
    qDebug() << "[clear_packet_tables] cost: " << time.elapsed() << " ms";
}

void MainWindow::add_row_for_video(int row, const VideoPacketInfo &info)
{
    if (row >= last_video_packet_cnts_)
        ui->tw_video_packet->insertRow(row);

    QTableWidgetItem *item0 = new QTableWidgetItem(QString::number(row));
    item0->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_video_packet->setItem(row, 0, item0);

    QTableWidgetItem *item1 = new QTableWidgetItem(QString::number(info.pkt_size));
    item1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_video_packet->setItem(row, 1, item1);

    QTableWidgetItem *item2 = new QTableWidgetItem(QString::number(info.pkt_dts));
    item2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_video_packet->setItem(row, 2, item2);

    QTableWidgetItem *item3 = new QTableWidgetItem(QString::number(info.diff_dts));
    item3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_video_packet->setItem(row, 3, item3);

    QTableWidgetItem *item4 = new QTableWidgetItem(QString::number(info.pkt_pts));
    item4->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_video_packet->setItem(row, 4, item4);

    //QTableWidgetItem *item5 = new QTableWidgetItem(QString::number(info.diff_pts));
    //item4->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    //ui->tw_video_packet->setItem(row, 5, item5);
}

void MainWindow::add_row_for_audio(int row, const VideoPacketInfo &info)
{
    if (row >= last_audio_packet_cnts_)
        ui->tw_audio_packet->insertRow(row);

    QTableWidgetItem *item0 = new QTableWidgetItem(QString::number(row));
    item0->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_packet->setItem(row, 0, item0);

    QTableWidgetItem *item1 = new QTableWidgetItem(QString::number(info.pkt_size));
    item1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_packet->setItem(row, 1, item1);

    QTableWidgetItem *item2 = new QTableWidgetItem(QString::number(info.pkt_dts));
    item2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_packet->setItem(row, 2, item2);

    QTableWidgetItem *item3 = new QTableWidgetItem(QString::number(info.diff_dts));
    item3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_packet->setItem(row, 3, item3);

    QTableWidgetItem *item4 = new QTableWidgetItem(QString::number(info.pkt_pts));
    item4->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_packet->setItem(row, 4, item4);

    QTableWidgetItem *item5 = new QTableWidgetItem(QString::number(info.diff_pts));
    item5->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_packet->setItem(row, 5, item5);
}

void MainWindow::add_row_for_video_frame(int row, const VideoPacketInfo &info)
{
    if (row >= last_video_frame_cnts_)
        ui->tw_video_frame->insertRow(row);

    QTableWidgetItem *item0 = new QTableWidgetItem(QString::number(row));
    item0->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_video_frame->setItem(row, 0, item0);

    QTableWidgetItem *item1 = new QTableWidgetItem(QString::number(info.pkt_size));
    item1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_video_frame->setItem(row, 1, item1);

    /*
    QString str_pkt_dts = "N/A";
    if (info.pkt_dts != AV_NOPTS_VALUE)
        str_pkt_dts = QString::number(info.pkt_dts);
    QTableWidgetItem *item2 = new QTableWidgetItem(str_pkt_dts);
    item2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_video_frame->setItem(row, 2, item2);
    */

    QString str_frame_pts = "N/A";
    if (info.frame_pts != AV_NOPTS_VALUE)
        str_frame_pts = QString::number(info.frame_pts);
    QTableWidgetItem *item2 = new QTableWidgetItem(str_frame_pts);
    item2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_video_frame->setItem(row, 2, item2);

    Qt::GlobalColor back_color = Qt::gray;
    QString type = "unknown";
    if (info.key_frame) {
        type = "key frame";
        back_color = Qt::darkRed;
    } else {
        if (info.frame_pict_type == AV_PICTURE_TYPE_I) {
            type = "I";
            back_color = Qt::red;
        } else if (info.frame_pict_type == AV_PICTURE_TYPE_P) {
            type = "P";
            back_color = Qt::darkGreen;
        } else if (info.frame_pict_type == AV_PICTURE_TYPE_B) {
            type = "B";
            back_color = Qt::darkBlue;
        } else if (info.frame_pict_type == AV_PICTURE_TYPE_S) {
            type = "S";
        } else if (info.frame_pict_type == AV_PICTURE_TYPE_SI) {
            type = "SI";
        } else if (info.frame_pict_type == AV_PICTURE_TYPE_SP) {
            type = "SP";
        } else if (info.frame_pict_type == AV_PICTURE_TYPE_BI) {
            type = "BI";
        }
    }
    QTableWidgetItem *item3 = new QTableWidgetItem(type);
    item3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    QBrush back_brush;
    back_brush.setColor(back_color);
    back_brush.setStyle(Qt::SolidPattern);
    item3->setBackground(back_brush);
    ui->tw_video_frame->setItem(row, 3, item3);
}

void MainWindow::add_row_for_audio_frame(int row, const VideoPacketInfo &info)
{
    if (row >= last_audio_frame_cnts_)
        ui->tw_audio_frame->insertRow(row);

    QTableWidgetItem *item0 = new QTableWidgetItem(QString::number(row));
    item0->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_frame->setItem(row, 0, item0);

    QTableWidgetItem *item1 = new QTableWidgetItem(QString::number(info.pkt_size));
    item1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_frame->setItem(row, 1, item1);

    /*
    QString str_pkt_dts = "N/A";
    if (info.pkt_dts != AV_NOPTS_VALUE)
        str_pkt_dts = QString::number(info.pkt_dts);
    QTableWidgetItem *item2 = new QTableWidgetItem(str_pkt_dts);
    item2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_frame->setItem(row, 2, item2);
    */

    QString str_frame_pts = "N/A";
    if (info.frame_pts != AV_NOPTS_VALUE)
        str_frame_pts = QString::number(info.frame_pts);
    QTableWidgetItem *item2 = new QTableWidgetItem(str_frame_pts);
    item2->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_frame->setItem(row, 2, item2);

    QTableWidgetItem *item3 = new QTableWidgetItem(tr("key frame"));
    QBrush back_brush;
    back_brush.setColor(Qt::darkRed);
    back_brush.setStyle(Qt::SolidPattern);
    item3->setBackground(back_brush);
    item3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tw_audio_frame->setItem(row, 3, item3);
}

#include "mainwindow.h"
#include <QApplication>
#include <QMutex>
#include <QDateTime>
#include <QDebug>
#include <QDir>

//Release 版本默认不包含context这些信息:文件名、函数名、行数，需要在.pro项目文件加入以下代码，加入后最好重新构建项目使之生效：
//DEFINES += QT_MESSAGELOGCONTEXT

//在.pro文件定义以下的宏，可以屏蔽相应的日志输出
//DEFINES += QT_NO_WARNING_OUTPUT
//DEFINES += QT_NO_DEBUG_OUTPUT
//DEFINES += QT_NO_INFO_OUTPUT

static QMutex g_mutex;
static QFile g_file;
static QTextStream g_textStream;

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString text;
    switch (type)
    {
    case QtDebugMsg:
        text = QString("[Debug]");
        break;
    case QtInfoMsg:
        text = QString("[Info]");
        break;
    case QtWarningMsg:
        text = QString("[Warning]");
        break;
    case QtCriticalMsg:
        text = QString("[Critical]");
        break;
    case QtFatalMsg:
        text = QString("[Fatal]");
    }
    text.append(QString("[%1]").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    text.append(QString("[%1: Line: %2]").arg(QString(context.file)).arg(context.line));
    text.append(QString("[Function: %1] ").arg(QString(context.function)));

    g_mutex.lock();
    g_textStream << text << QString("Message: %1").arg(msg) << Qt::endl;
    g_mutex.unlock();
}

void logInit()
{
    QDir dir("log");
    if (!dir.exists()) {
        QDir dir;
        dir.mkdir("log");
    }
    qDebug() << dir.absolutePath();

    QString currentDate = QDateTime::currentDateTime().toString("yyyyMMdd");
    QString logName = "log/log-" + currentDate + ".txt";

    g_file.setFileName(logName);
    g_file.open(QIODevice::WriteOnly | QIODevice::Append);
    g_textStream.setDevice(&g_file);
    g_textStream.setGenerateByteOrderMark(true);
    g_textStream.setEncoding(QStringConverter::Utf8);
    qInstallMessageHandler(outputMessage);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    logInit();

    qDebug("app is running...");

    QFile f(":/qdarkstyle/dark/darkstyle.qss");
    if (!f.exists())   {
        qDebug() << "Unable to set stylesheet, file not found\n";
    } else {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        qApp->setStyleSheet(ts.readAll());
        f.close();
    }

    // 默认使用原生菜单栏 可以通过下面设置禁用原生菜单栏
    //QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);

    // need on windows
    qRegisterMetaType<QList<VideoPacketInfo>>("QList<VideoPacketInfo>");
    qRegisterMetaType<QList<VideoPacketInfo>>("QList<VideoPacketInfo>&");

    MainWindow w;
    w.show();

    return a.exec();
}

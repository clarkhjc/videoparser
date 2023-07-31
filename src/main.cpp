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

static QString logFileName;
static QMutex mutex;
static QFile file;
static QTextStream textStream;

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

    mutex.lock();
    textStream << text << QString("Message: %1").arg(msg) << endl;
    mutex.unlock();
}

void logInit(QString logfileName)
{
    logFileName = logfileName;
    QFileInfo qfi(logFileName);
    if (qfi.size() > 20*1024*1024) {
        QDateTime qdt;
        qint64 timestamp = qdt.currentMSecsSinceEpoch();
        QFile::rename(logFileName, logFileName + QString::number(timestamp));
    }
    {
        QMutexLocker locker(&mutex);
        QFile file(logFileName);
        file.open(QIODevice::ReadOnly);
        QTextStream textStream(&file);
        QString temp;
        QStringList tempList;
        QRegExp regExp(".*(20\\d\\d-\\d\\d-\\d\\d).*");
        while (false == textStream.atEnd()) {
            temp = textStream.readLine();
            if (temp.isEmpty()) {
                continue;
            }
            if (temp.indexOf(regExp) >= 0) {
                QDate date = QDate::fromString(regExp.cap(1), "yyyy-MM-dd");
                QDate currentDate = QDate::currentDate();
                if (date.daysTo(currentDate) < date.day()) {
                    tempList << temp;
                    tempList << textStream.readLine();
                }
            }
        }
        file.close();
        file.open(QIODevice::Truncate | QIODevice::WriteOnly);
        textStream.setDevice(&file);
        for (auto iterator = tempList.begin(); iterator != tempList.end(); iterator++) {
            textStream << *iterator << endl;
        }
        file.close();
    }
    file.setFileName(logFileName);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    textStream.setDevice(&file);
    qInstallMessageHandler(outputMessage);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString currentDate = QDateTime::currentDateTime().toString("yyyyMMdd");
    QString logName = "log-" + currentDate + ".txt";
    QString logFileName = "log/" + logName;
    logInit(logFileName);

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

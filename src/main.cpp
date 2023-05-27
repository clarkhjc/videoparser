#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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

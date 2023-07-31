#-------------------------------------------------
#
# Project created by QtCreator 2023-05-16T14:10:58
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = videoparser
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_MESSAGELOGCONTEXT

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11
CONFIG += sdk_no_version_check

INCLUDEPATH += -I ./include/

macx {
ICON = img/logo.icns
    # ffmpeg
    INCLUDEPATH += -I /usr/local/include/
    LIBS += /usr/local/lib/libavdevice.a /usr/local/lib/libavutil.a
    LIBS += /usr/local/lib/libavformat.a /usr/local/lib/libavfilter.a
    LIBS += /usr/local/lib/libavcodec.a /usr/local/lib/libswscale.a /usr/local/lib/libswresample.a

    # ffmpeg deps
    LIBS += -lz -lbz2 -liconv

    # macos sdk
    LIBS += -framework CoreFoundation
    LIBS += -framework Security
    LIBS += -framework VideoToolbox
    LIBS += -framework AudioToolbox
    LIBS += -framework CoreVideo
    LIBS += -framework CoreMedia
}

win32 {

    #ffmpeg
    INCLUDEPATH += -I D:\\QT_project\\videoparser\\deps\\include\\

    LIBS += D:\QT_project\videoparser\deps\lib\x64\libavdeviced.lib
    LIBS += D:\QT_project\videoparser\deps\lib\x64\libavutild.lib
    LIBS += D:\QT_project\videoparser\deps\lib\x64\libavformatd.lib
    LIBS += D:\QT_project\videoparser\deps\lib\x64\libavfilterd.lib
    LIBS += D:\QT_project\videoparser\deps\lib\x64\libavcodecd.lib
    LIBS += D:\QT_project\videoparser\deps\lib\x64\libswscaled.lib
    LIBS += D:\QT_project\videoparser\deps\lib\x64\libswresampled.lib

    LIBS += -lole32 -luser32
}

SOURCES += \
        src/ffmpegcommon.cpp \
        src/ffmpegdecode.cpp \
        src/ffmpegdemux.cpp \
        src/main.cpp \
        src/mainwindow.cpp \
        src/parsefile.cpp \
        src/qlineeditwithdrag.cpp \
        src/usersettings.cpp

HEADERS += \
        include/ffmpegcommon.h \
        include/ffmpegdecode.h \
        include/ffmpegdemux.h \
        include/mainwindow.h \
        include/parsefile.h \
        include/qlineeditwithdrag.h \
        include/usersettings.h

FORMS += \
        ui/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    qdarkstyle/dark/darkstyle.qrc \
    qdarkstyle/light/lightstyle.qrc \
    res.qrc

DISTFILES += \
    README.md


#include "usersettings.h"

UserSettings::UserSettings()
{

}

UserSettings::~UserSettings()
{

}

QString UserSettings::get_parse_mode()
{
    QString parse_mode;
#ifdef Q_OS_WIN
    QSettings registry(QSettings::NativeFormat, // Format
                       QSettings::UserScope,    // Scope
                       "Clark",                 // Organization
                       "videoparser"            // Application
                       );
    parse_mode = registry.value("parseMode").toString();
#else
    QSettings settings;
    parse_mode = settings.value("parseMode").toString();
#endif
    return parse_mode;
}

void UserSettings::set_parse_mode(const QString &mode)
{
#ifdef Q_OS_WIN
    QSettings registry(QSettings::NativeFormat, // Format
                       QSettings::UserScope,    // Scope
                       "Clark",                 // Organization
                       "videoparser"            // Application
                       );
    registry.setValue("parseMode", mode);
#else
    QSettings settings;
    settings.setValue("parseMode", mode);
#endif
}

QStringList UserSettings::get_recent_files()
{
    QStringList files;
#ifdef Q_OS_WIN
    QSettings registry(QSettings::NativeFormat, // Format
                       QSettings::UserScope,    // Scope
                       "Clark",                 // Organization
                       "videoparser"            // Application
                       );
    files = registry.value("recentFiles").toStringList();
#else
    QSettings settings;
    files = settings.value("recentFiles").toStringList();
#endif
    return files;
}

void UserSettings::set_recent_files(const QStringList &files)
{
#ifdef Q_OS_WIN
    QSettings registry(QSettings::NativeFormat, // Format
                       QSettings::UserScope,    // Scope
                       "Clark",                 // Organization
                       "videoparser"            // Application
                       );
    registry.setValue("recentFiles", files);
#else
    QSettings settings;
    settings.setValue("recentFiles", files);
#endif
}

UserSettings &get_user_settings() {
    static UserSettings user_settings;
    return user_settings;
}

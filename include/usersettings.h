#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include <QSettings>
#include <QString>

class UserSettings {
public:
    UserSettings();
    ~UserSettings();

    QString get_parse_mode();
    void set_parse_mode(const QString &mode);

    QStringList get_recent_files();
    void set_recent_files(const QStringList &files);
};

UserSettings &get_user_settings();

#endif // USERSETTINGS_H

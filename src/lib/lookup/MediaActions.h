#ifndef MEDIAACTIONS_H
#define MEDIAACTIONS_H

#include <QString>
#include <QHash>
struct MediaActions
{
    const QString TogglePause = "TogglePause";
    const QString Next = "Next";
    const QString Back = "Back";
    const QString FullScreen = "FullScreen";
    const QString VolumeUp = "VolumeUp";
    const QString VolumeDown = "VolumeDown";
    const QString Mute = "Mute";
    const QString Stop = "Stop";
    const QHash<QString, QString> Values {
        {TogglePause, "Toggle pause"},
        {Next, "Next video"},
        {Back, "Previous video"},
        {FullScreen, "Toggle fullscreen"},
        {VolumeUp, "Volume up"},
        {VolumeDown, "Volume down"},
        {Mute, "Toggle mute"},
        {Stop, "Stop video"}
    };
};
#endif // MEDIAACTIONS_H

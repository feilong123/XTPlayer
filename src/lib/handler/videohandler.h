#ifndef VIDEOHANDLER_H
#define VIDEOHANDLER_H
#include <QWidget>
#include <QtAV>
#include <QtAVWidgets>
#include <QHBoxLayout>
#include "lib/handler/settingshandler.h"
#include "lib/handler/loghandler.h"

using namespace QtAV;

class VideoHandler : public QWidget
{
    Q_OBJECT

private slots:
    void on_media_positionChanged(qint64 position);
    void on_media_statusChanged(MediaStatus status);
    void on_media_start();
    void on_media_stop();

signals:
    void doubleClicked(QMouseEvent* e);
    void rightClicked(QMouseEvent* e);
    void keyPressed(QKeyEvent* k);
    void mouseEnter(QEvent* e);
    void positionChanged(int position);
    void mediaStatusChanged(MediaStatus status);
    void started();
    void stopped();
    void togglePaused(bool paused);

public:;
    VideoHandler(QWidget* parent = 0);
    virtual ~VideoHandler();
    bool isPlaying();
    void play();
    void stop();
    void togglePause();
    void pause();
    void setFile(QString file);
    QString file();
    void load();
    bool isMute();
    bool isPaused();
    void toggleMute();
    void setVolume(int value);
    AVPlayer::State state();
    void seek(qint64 position);
    void setPosition(qint64 position);
    void setRepeat(int max = 0);
    void setSpeed(qreal speed);
    qint64 position();
    qint64 duration();
    QHBoxLayout* layout();
    void showPreview(int position, qint64 time);


private:
    QHBoxLayout* _widgetLayout;
    AVPlayer* _player;
    VideoOutput* _videoRenderer;
    VideoPreviewWidget* _videoPreviewWidget;
    QString _currentFile;
    QMutex _mutex;

    void mouseDoubleClickEvent(QMouseEvent * e) override;
    void mousePressEvent(QMouseEvent * e) override;
    void keyPressEvent(QKeyEvent * e) override;
    void enterEvent(QEvent * e) override;


};
#endif // VIDEOHANDLER_H
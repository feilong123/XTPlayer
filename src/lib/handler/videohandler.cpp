#include "videohandler.h"
VideoHandler::VideoHandler(QWidget *parent) : QWidget(parent),
    _player(0), _videoRenderer(0)
{
    _parent = parent;
    QtAV::Widgets::registerRenderers();
    _player = new AVPlayer(this);
    setVideoRenderer(SettingsHandler::getSelectedVideoRenderer());
    setDecoderPriority();
    QVariantHash opt;
    QVariantHash cuda_opt;
    cuda_opt["surfaces"] = 20; //key is property name, case sensitive
    cuda_opt["copyMode"] = "DirectCopy"; // default is "DirectCopy"
    cuda_opt["flags"] = "+genpt";
    cuda_opt["deinterlace"] = "Adaptive";
    opt["CUDA"] = cuda_opt; //key is decoder name, case sensitive

//    QVariantHash va_opt;
//    va_opt["display"] = "X11"; //"GLX", "X11", "DRM"
//    va_opt["copyMode"] = "ZeroCopy"; // "ZeroCopy", "OptimizedCopy", "GenericCopy". Default is "ZeroCopy" if possible
//    opt["VAAPI"] = va_opt; //key is decoder name, case sensitive

//    QVariantHash avfmt_opt;
//    avfmt_opt["probesize"] = 4096;
//    avfmt_opt["user_agent"] = "xxx";
//    opt["avformat"] = avfmt_opt;
    auto test = QtAV::avcodecOptions().split('\n');
    auto test2 = QtAV::avformatOptions().split("\n");;
    _player->setOptionsForVideoCodec(opt);

    _player->setBufferMode(QtAV::BufferMode::BufferBytes);

    _player->audio()->setVolume(SettingsHandler::getPlayerVolume() * 0.001f);
    _player->setSeekType(QtAV::SeekType::KeyFrameSeek);

    connect(_player, &AVPlayer::positionChanged, this, &VideoHandler::on_media_positionChanged, Qt::QueuedConnection);
    connect(_player, &AVPlayer::mediaStatusChanged, this, &VideoHandler::on_media_statusChanged, Qt::QueuedConnection);
    connect(_player, &AVPlayer::started, this, &VideoHandler::on_media_start, Qt::QueuedConnection);
    connect(_player, &AVPlayer::stopped, this, &VideoHandler::on_media_stop, Qt::QueuedConnection);

    setMinimumHeight(SettingsHandler::getThumbSize());
    setMinimumWidth(SettingsHandler::getThumbSize());
}

QStringList idsToNames(QList<VideoDecoderId> ids) {
    QStringList decs;
    foreach (int id, ids) {
        decs.append(QString::fromLatin1(VideoDecoder::name(id)));
    }
    return decs;
}

QList<VideoDecoderId> idsFromNames(const QStringList& names) {
    QList<VideoDecoderId> decs;
    foreach (QString name, names) {
        if (name.isEmpty())
            continue;
        VideoDecoderId id = VideoDecoder::id(name.toLatin1().constData());
        if (id == 0)
            continue;
        decs.append(id);
    }
    return decs;
}

void VideoHandler::createLayout(VideoRenderer* videoRenderer)
{
    setContentsMargins(contentsMargins().left(), contentsMargins().top(), contentsMargins().right(), 0);
    LogHandler::Debug("Create player");
    if(_mediaGrid)
        delete _mediaGrid;
    _mediaGrid = new QGridLayout(this);
    setContentsMargins(contentsMargins().left(), contentsMargins().top(), contentsMargins().right(), 0);

    _mediaGrid->setMargin(0);
    _mediaGrid->setContentsMargins(0,0,0,0);
    setLayout(_mediaGrid);

    //audioSyncFilter = new AudioSyncFilter(this);
    //installFilter(audioSyncFilter);

    if(_videoLoadingMovie)
        delete _videoLoadingMovie;
    if(_videoLoadingLabel)
        delete _videoLoadingLabel;
    _videoLoadingMovie = new QMovie("://images/Eclipse-1s-loading-200px.gif");
    _videoLoadingMovie->setProperty("cssClass", "mediaLoadingSpinner");
    _videoLoadingLabel = new QLabel(this);
    _videoLoadingLabel->setMovie(_videoLoadingMovie);
    _videoLoadingLabel->setAttribute(Qt::WA_TransparentForMouseEvents );
    _videoLoadingLabel->setMaximumSize(200,200);
    //_videoLoadingLabel->setStyleSheet("* {background: ffffff}");
    _videoLoadingLabel->setProperty("cssClass", "mediaLoadingSpinner");
    _videoLoadingLabel->setAlignment(Qt::AlignCenter);
    setLoading(false);

    _videoRenderer = videoRenderer;
    _videoRenderer->widget()->setGeometry(_parent->geometry());
    _mediaGrid->addWidget(_videoRenderer->widget(), 0, 0, 3, 5);
    _mediaGrid->addWidget(_videoLoadingLabel, 1, 2);

}
VideoHandler::~VideoHandler()
{
    delete _mediaGrid;
    delete _player;
    delete _videoRenderer;
}

QString VideoHandler::file()
{
    return _currentFile;
}

void VideoHandler::mouseDoubleClickEvent(QMouseEvent * e)
{
    emit doubleClicked(e);
}
void VideoHandler::mousePressEvent(QMouseEvent * e)
{
    if(e->button() == Qt::MouseButton::RightButton)
        emit rightClicked(e);
}
void VideoHandler::keyPressEvent(QKeyEvent * e)
{
    emit keyPressed(e);
}

void VideoHandler::enterEvent(QEvent * e)
{
    emit mouseEnter(e);
}

void VideoHandler::on_media_positionChanged(qint64 position)
{
    //const QMutexLocker locker(&_mutex);
    emit positionChanged(position);
}

void VideoHandler::on_media_statusChanged(MediaStatus status)
{
    emit mediaStatusChanged(status);
}

void VideoHandler::on_media_start()
{
    emit started();
}

void VideoHandler::on_media_stop()
{
    emit stopped();
}

bool VideoHandler::isPlaying()
{
    return _player ? _player->isPlaying() : false;
}

void VideoHandler::play()
{
    _player->play();
    //transcode(_currentFile);
    emit playing();
}

void VideoHandler::stop()
{
    _player->stop();
    emit stopping();
}

void VideoHandler::togglePause()
{
    _player->togglePause();
    emit togglePaused(isPaused());
}

void VideoHandler::pause()
{
    _player->pause();
    emit togglePaused(isPaused());
}

void VideoHandler::setFile(QString file)
{
    _currentFile = file;
    _player->setFile(file);
}

void VideoHandler::load()
{
    _player->load();
}

bool VideoHandler::isPaused()
{
    return _player->isPaused();
}

bool VideoHandler::isMute()
{
    return _player->audio()->isMute();
}

void VideoHandler::toggleMute()
{
    if (!isMute())
    {
        volumeBeforeMute = _player->audio()->volume();
        _player->audio()->setMute(true);
    }
    else
    {
        _player->audio()->setMute(false);
        _player->audio()->setVolume(volumeBeforeMute);
    }
}

void VideoHandler::setVolume(int value)
{
    _player->audio()->setVolume(value * 0.001f);
}
void VideoHandler::setRepeat(int max)
{
    _player->setRepeat(max);
}
AVPlayer::State VideoHandler::state()
{
    return _player->state();
}

void VideoHandler::setPosition(qint64 position)
{
    //const QMutexLocker locker(&_mutex);
    disconnect(_player, &AVPlayer::positionChanged, this, &VideoHandler::on_media_positionChanged);
    _player->setPosition(position);
    connect(_player, &AVPlayer::positionChanged, this, &VideoHandler::on_media_positionChanged, Qt::QueuedConnection);
}

void VideoHandler::seek(qint64 position)
{
    _player->seek(position);
}

void VideoHandler::setSpeed(qreal speed)
{
    _player->setSpeed(speed);
}

qint64 VideoHandler::position()
{
    return _player->position();
}

qint64 VideoHandler::duration()
{
    return _player->duration();
}

QGridLayout* VideoHandler::layout()
{
    return _mediaGrid;
}

void VideoHandler::setLoading(bool loading)
{
    on_setLoading(loading);
}

void VideoHandler::on_setLoading(bool loading)
{
    if(loading && _videoLoadingMovie->state() != QMovie::MovieState::Running)
    {
        _videoLoadingLabel->show();
        _videoLoadingMovie->start();
    }
    else if(!loading && _videoLoadingMovie->state() == QMovie::MovieState::Running)
    {
        _videoLoadingLabel->hide();
        _videoLoadingMovie->stop();
    }
}

void VideoHandler::setDecoderPriority()
{
    bool wasPlaying = false;
    if(isPlaying())
    {
        wasPlaying = true;
        stop();
    }
    QStringList stringList;
    QList<DecoderModel> models = SettingsHandler::getDecoderPriority();
    foreach (auto model, models)
        if(model.Enabled)
            stringList.append(model.Name);
    _player->setVideoDecoderPriority(stringList);
    if(wasPlaying)
        play();
}
bool VideoHandler::setVideoRenderer(XVideoRenderer renderer)
{
    bool wasPlaying = false;
    if(isPlaying())
    {
        wasPlaying = true;
        stop();
    }

    QtAV::VideoRendererId vid = QtAVVideoRendererIdMap.value(renderer);
    VideoRenderer *videoRenderer = VideoRenderer::create(vid);
    if (!videoRenderer || !videoRenderer->isAvailable() || !videoRenderer->widget())
    {
//        LogHandler::Debug(" '" + XVideoRendererReverseMap.value(renderer) + "' failed, trying default");
//        _videoRenderer = new VideoOutput(this);
//        if (!_videoRenderer || !_videoRenderer->isAvailable() || !_videoRenderer->widget())
//        {
            LogHandler::Dialog("QtAV Video renderer '" + XVideoRendererReverseMap.value(renderer) + "' is not availabe on your platform!", XLogLevel::Critical);
            return false;
//        }
    }

    videoRenderer->widget()->setParent(this);
    videoRenderer->widget()->setMouseTracking(true); //mouseMoveEvent without press.

    QWidget* r = 0;
    if (_videoRenderer)
    {
        r = _videoRenderer->widget();
        if(r)
        {
            _mediaGrid->removeWidget(r);
            if (r->testAttribute(Qt::WA_DeleteOnClose)) {
                r->close();
            } else {
                r->close();
                delete r;
            }
            r = 0;
        }
    }

    createLayout(videoRenderer);

//    if (mpPlayer->renderer()->id() == VideoRendererId_GLWidget) {
//        mpVideoEQ->setEngines(QVector<VideoEQConfigPage::Engine>() << VideoEQConfigPage::SWScale << VideoEQConfigPage::GLSL);
//        mpVideoEQ->setEngine(VideoEQConfigPage::GLSL);
//    } else if (mpPlayer->renderer()->id() == VideoRendererId_XV) {
//        mpVideoEQ->setEngines(QVector<VideoEQConfigPage::Engine>() << VideoEQConfigPage::XV);
//        mpVideoEQ->setEngine(VideoEQConfigPage::XV);
//    } else {
//        mpVideoEQ->setEngines(QVector<VideoEQConfigPage::Engine>() << VideoEQConfigPage::SWScale);
//        mpVideoEQ->setEngine(VideoEQConfigPage::SWScale);
//    }

    if (vid == VideoRendererId_GLWidget || vid == VideoRendererId_GLWidget2 || vid == VideoRendererId_OpenGLWidget || vid == VideoRendererId_XV)
    {
        _videoRenderer->forcePreferredPixelFormat(true);
//        //_videoRenderer->forcePreferredPixelFormat(false);
//        //qApp->setAttribute(Qt::AA_UseOpenGLES);
//        _videoRenderer->forcePreferredPixelFormat(true);
//        _videoRenderer->setPreferredPixelFormat(VideoFormat::Format_RGB32);
    }
    else//SWScale
    {
        _videoRenderer->forcePreferredPixelFormat(false);
        _videoRenderer->setPreferredPixelFormat(VideoFormat::Format_RGB32);
    }
    _player->setRenderer(_videoRenderer);
//    _player->renderer()->resizeRenderer(_player->renderer()->rendererSize());
//    _player->renderer()->widget()->resize(_player->renderer()->rendererSize());
//    if(wasPlaying)
//        play();
    return true;
}

void VideoHandler::installFilter(AudioFilter* filter)
{
    _player->installFilter(filter);
}

void VideoHandler::clearFilters()
{
    qDeleteAll(_player->videoFilters());
}
#include <QtAV/AVTranscoder.h>
QString VideoHandler::transcode(QString file)
{
    QFileInfo fileInfo(file);
    QString outFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + fileInfo.fileName();
    QString hwdev;
    const bool an = false;//a.arguments().contains(QLatin1String("-an"));
    qint64 ss = 0;
    QString fmt;
    QString cv = QString::fromLatin1("libx264");
    QString ca = QString::fromLatin1("aac");
    QVariantHash muxopt, avfopt;
    avfopt[QString::fromLatin1("segment_time")] = 10;
    avfopt[QString::fromLatin1("segment_list_size")] = 10;
    avfopt[QString::fromLatin1("segment_list")] = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + "index.m3u8";
    avfopt[QString::fromLatin1("segment_format")] = QString::fromLatin1("mpegts");
    muxopt[QString::fromLatin1("avformat")] = avfopt;
    //AVPlayer player;
    _player->setFile(file);
    _player->setFrameRate(10000.0); // as fast as possible. FIXME: why 1000 may block source player?
    _player->audio()->setBackends(QStringList() << QString::fromLatin1("null"));
    AVTranscoder avt;
    if (ss > 0)
        avt.setStartTime(ss);
    avt.setMediaSource(_player);
    avt.setOutputMedia(outFile);
    avt.setOutputOptions(muxopt);
    if (!fmt.isEmpty())
        avt.setOutputFormat(fmt); // segment, image2
    if (!avt.createVideoEncoder()) {
        qWarning("Failed to create video encoder");
        return nullptr;
    }
    VideoEncoder *venc = avt.videoEncoder();
    venc->setCodecName(cv); // "png"
    venc->setBitRate(1024*1024);
    if (!hwdev.isEmpty())
        venc->setProperty("hwdevice", hwdev);
    if (fmt == QLatin1String("image2"))
        venc->setPixelFormat(VideoFormat::Format_RGBA32);
    if (an) {
        avt.sourcePlayer()->setAudioStream(-1);
    } else {
        if (!avt.createAudioEncoder()) {
            qWarning("Failed to create audio encoder");
            return nullptr;
        }
        AudioEncoder *aenc = avt.audioEncoder();
        aenc->setCodecName(ca);
    }
    //QObject::connect(&avt, &AVTranscoder::started(), qApp, SLOT(quit()));
    avt.setAsync(true);
    avt.start(); //start transcoder first
    _player->play();
    return outFile;
}

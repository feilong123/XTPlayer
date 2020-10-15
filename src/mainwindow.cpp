#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QStringList arguments, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    SettingsHandler::Load();
    if(arguments.length() > 0)
    {
        foreach(QString arg, arguments)
        {
            if(arg.toLower().startsWith("-debug"))
                LogHandler::UserDebug(true);
            else if(arg.toLower().startsWith("-reset"))
                SettingsHandler::Default();
        }
    }
    QFile file(SettingsHandler::getSelectedTheme());
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    funscriptHandler = new FunscriptHandler(_axisNames.TcXUpDownL0);

    textToSpeech = new QTextToSpeech(this);
    auto availableVoices = textToSpeech->availableVoices();

    const QVoice voice = boolinq::from(availableVoices).firstOrDefault([](const QVoice &x) { return x.gender() == QVoice::Female; });
    textToSpeech->setVoice(voice);

    deoConnectionStatusLabel = new QLabel(this);
    deoRetryConnectionButton = new QPushButton(this);
    deoRetryConnectionButton->hide();
    deoRetryConnectionButton->setText("DeoVR Retry");
    ui->statusbar->addPermanentWidget(deoConnectionStatusLabel);
    ui->statusbar->addPermanentWidget(deoRetryConnectionButton);

    whirligigConnectionStatusLabel = new QLabel(this);
    whirligigRetryConnectionButton = new QPushButton(this);
    whirligigRetryConnectionButton->hide();
    whirligigRetryConnectionButton->setText("Whirligig Retry");
    ui->statusbar->addPermanentWidget(whirligigConnectionStatusLabel);
    ui->statusbar->addPermanentWidget(whirligigRetryConnectionButton);

    connectionStatusLabel = new QLabel(this);
    retryConnectionButton = new QPushButton(this);
    retryConnectionButton->hide();
    retryConnectionButton->setText("TCode Retry");
    ui->statusbar->addPermanentWidget(connectionStatusLabel);
    ui->statusbar->addPermanentWidget(retryConnectionButton);

    gamepadConnectionStatusLabel = new QLabel(this);
    ui->statusbar->addPermanentWidget(gamepadConnectionStatusLabel);

    videoHandler = new VideoHandler(this);
    ui->MediaGrid->addWidget(videoHandler);

    ui->videoLoadingLabel->hide();
    _movie = new QMovie(QApplication::applicationDirPath() + "/themes/loading.gif");
    ui->videoLoadingLabel->setMovie(_movie);

    ui->SeekSlider->setDisabled(true);
    ui->SeekSlider->SetRange(0, 100);
    ui->SeekSlider->setOption(RangeSlider::Option::RightHandle);
    ui->SeekSlider->setUpperValue(0);

    ui->VolumeSlider->setDisabled(false);
    ui->VolumeSlider->SetRange(0, 30);
    ui->VolumeSlider->setOption(RangeSlider::Option::RightHandle);
    ui->VolumeSlider->setUpperValue(SettingsHandler::getPlayerVolume());
    setVolumeIcon(SettingsHandler::getPlayerVolume());

    ui->LibraryList->setUniformItemSizes(true);
    ui->LibraryList->setContextMenuPolicy(Qt::CustomContextMenu);

    thumbCaptureTime = 35000;
    _currentMaxThumbSize = {175,175};
    libraryViewGroup = new QActionGroup(this);
    libraryViewGroup->addAction(ui->actionList);
    libraryViewGroup->addAction(ui->actionThumbnail);

    videoPreviewWidget = new VideoPreviewWidget(this);
    videoPreviewWidget->resize(150, 90);
    videoPreviewWidget->hide();

    QMenu* submenuSize = ui->menuView->addMenu( "Size" );
    submenuSize->setObjectName("sizeMenu");
    libraryThumbSizeGroup = new QActionGroup(submenuSize);
    action75_Size = submenuSize->addAction( "75" );
    action75_Size->setCheckable(true);
    action100_Size = submenuSize->addAction( "100" );
    action100_Size->setCheckable(true);
    action125_Size = submenuSize->addAction( "125" );
    action125_Size->setCheckable(true);
    action150_Size = submenuSize->addAction( "150" );
    action150_Size->setCheckable(true);
    action175_Size = submenuSize->addAction( "175" );
    action175_Size->setCheckable(true);
    libraryThumbSizeGroup->addAction(action75_Size);
    libraryThumbSizeGroup->addAction(action100_Size);
    libraryThumbSizeGroup->addAction(action125_Size);
    libraryThumbSizeGroup->addAction(action150_Size);
    libraryThumbSizeGroup->addAction(action175_Size);
    if (SettingsHandler::getLibraryView() == LibraryView::List)
    {
        ui->actionList->setChecked(true);
        on_actionList_triggered();
    }
    else
    {
        ui->actionThumbnail->setChecked(true);
        on_actionThumbnail_triggered();
    }

    on_load_library(SettingsHandler::getSelectedLibrary());

    _xSettings = new SettingsDialog(this);
    tcodeHandler = new TCodeHandler();

    connect(_xSettings, &SettingsDialog::deoDeviceConnectionChange, this, &MainWindow::on_deo_device_connectionChanged);
    connect(_xSettings, &SettingsDialog::deoDeviceError, this, &MainWindow::on_deo_device_error);

    connect(_xSettings, &SettingsDialog::whirligigDeviceConnectionChange, this, &MainWindow::on_whirligig_device_connectionChanged);
    connect(_xSettings, &SettingsDialog::whirligigDeviceError, this, &MainWindow::on_whirligig_device_error);
    connect(_xSettings, &SettingsDialog::deviceConnectionChange, this, &MainWindow::on_device_connectionChanged);
    connect(_xSettings, &SettingsDialog::deviceError, this, &MainWindow::on_device_error);
    connect(_xSettings->getDeoHandler(), &DeoHandler::messageRecieved, this, &MainWindow::onVRMessageRecieved);
    connect(_xSettings->getWhirligigHandler(), &WhirligigHandler::messageRecieved, this, &MainWindow::onVRMessageRecieved);
    connect(_xSettings, &SettingsDialog::gamepadConnectionChange, this, &MainWindow::on_gamepad_connectionChanged);
    connect(_xSettings->getGamepadHandler(), &GamepadHandler::emitTCode, this, &MainWindow::on_gamepad_sendTCode);
    connect(_xSettings->getGamepadHandler(), &GamepadHandler::emitAction, this, &MainWindow::on_gamepad_sendAction);

    _xSettings->init(videoHandler);

    connect(action75_Size, &QAction::triggered, this, &MainWindow::on_action75_triggered);
    connect(action100_Size, &QAction::triggered, this, &MainWindow::on_action100_triggered);
    connect(action125_Size, &QAction::triggered, this, &MainWindow::on_action125_triggered);
    connect(action150_Size, &QAction::triggered, this, &MainWindow::on_action150_triggered);
    connect(action175_Size, &QAction::triggered, this, &MainWindow::on_action175_triggered);

    connect(videoHandler, &VideoHandler::positionChanged, this, &MainWindow::on_media_positionChanged);
    connect(videoHandler, &VideoHandler::mediaStatusChanged, this, &MainWindow::on_media_statusChanged);
    connect(videoHandler, &VideoHandler::started, this, &MainWindow::on_media_start);
    connect(videoHandler, &VideoHandler::stopped, this, &MainWindow::on_media_stop);
    connect(videoHandler, &VideoHandler::togglePaused, this, &MainWindow::onVideoHandler_togglePaused);

    connect(ui->SeekSlider, &RangeSlider::upperValueMove, this, &MainWindow::on_seekSlider_sliderMoved);
    connect(ui->SeekSlider, &RangeSlider::onHover, this, &MainWindow::on_seekslider_hover );
    connect(ui->SeekSlider, &RangeSlider::onLeave, this, &MainWindow::on_seekslider_leave );
    connect(ui->VolumeSlider, &RangeSlider::upperValueMove, this, &MainWindow::on_VolumeSlider_valueChanged);
    //connect(player, static_cast<void(AVPlayer::*)(AVPlayer::Error )>(&AVPlayer::error), this, &MainWindow::on_media_error);

    connect(videoHandler, &VideoHandler::doubleClicked, this, &MainWindow::media_double_click_event);
    connect(videoHandler, &VideoHandler::rightClicked, this, &MainWindow::media_single_click_event);
    connect(this, &MainWindow::keyPressed, this, &MainWindow::on_key_press);
    connect(this, &MainWindow::change, this, &MainWindow::on_mainwindow_change);
    //connect(videoHandler, &VideoHandler::mouseEnter, this, &MainWindow::on_video_mouse_enter);

    connect(ui->LibraryList, &QListWidget::customContextMenuRequested, this, &MainWindow::onLibraryList_ContextMenuRequested);


    connect(retryConnectionButton, &QPushButton::clicked, _xSettings, &SettingsDialog::initDeviceRetry);
    connect(deoRetryConnectionButton, &QPushButton::clicked, _xSettings, &SettingsDialog::initDeoRetry);
    connect(QApplication::instance(), &QCoreApplication::aboutToQuit, this, &MainWindow::dispose);

    setFocus();
    _defaultAppSize = this->size();

}
MainWindow::~MainWindow()
{

}
void MainWindow::dispose()
{
    SettingsHandler::Save();
    _xSettings->dispose();
    if (videoHandler->isPlaying())
    {
        videoHandler->stop();
    }
    if(funscriptFuture.isRunning())
    {
        funscriptFuture.cancel();
        funscriptFuture.waitForFinished();
    }
    if(funscriptVRSyncFuture.isRunning())
    {
        funscriptVRSyncFuture.cancel();
        funscriptVRSyncFuture.waitForFinished();
    }
    qDeleteAll(funscriptHandlers);
    delete tcodeHandler;
    delete videoHandler;
    delete connectionStatusLabel;
    delete retryConnectionButton;
    delete videoPreviewWidget;
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // This function repeatedly call for those QObjects
    // which have installed eventFilter (Step 2)

    if (obj == (QObject*)playerControlsPlaceHolder) {
        if (event->type() == QEvent::Enter)
        {
            showControls();
        }
        else if(event->type() == QEvent::Leave)
        {
            hideControls();
        }
        return true;
    }
    else
    {
        // pass the event on to the parent class
        return QWidget::eventFilter(obj, event);
    }
}
void MainWindow::on_key_press(QKeyEvent * event)
{
    MediaActions actions;
    switch(event->key())
    {
        case Qt::Key_Space:
        case Qt::Key_Enter:
        case Qt::Key_MediaPause:
        case Qt::Key_MediaTogglePlayPause:
            mediaAction(actions.TogglePause);
            break;
        case Qt::Key_1:
        case Qt::Key_F11:
            mediaAction(actions.FullScreen);
            break;
        case Qt::Key_M:
        case Qt::Key_2:
            mediaAction(actions.Mute);
            break;
        case Qt::Key_MediaStop:
        case Qt::Key_Escape:
            mediaAction(actions.Stop);
            break;
        case Qt::Key_MediaNext:
        case Qt::Key_E:
            mediaAction(actions.Next);
            break;
        case Qt::Key_MediaPrevious:
        case Qt::Key_Q:
            mediaAction(actions.Back);
            break;
        case Qt::Key_VolumeUp:
        case Qt::Key_W:
            mediaAction(actions.VolumeUp);
            break;
        case Qt::Key_VolumeDown:
        case Qt::Key_S:
            mediaAction(actions.VolumeDown);
            break;
        case Qt::Key_L:
        case Qt::Key_C:
            mediaAction(actions.Loop);
            break;
        case Qt::Key_A:
            mediaAction(actions.Rewind);
            break;
        case Qt::Key_D:
            mediaAction(actions.FastForward);
            break;
        case Qt::Key_X:
            mediaAction(actions.IncreaseXRange);
            break;
        case Qt::Key_Z:
            mediaAction(actions.DecreaseXRange);
            break;
        case Qt::Key_F:
            mediaAction(actions.DecreaseXUpperRange);
            break;
        case Qt::Key_R:
            mediaAction(actions.IncreaseXUpperRange);
            break;
        case Qt::Key_G:
            mediaAction(actions.DecreaseXLowerRange);
            break;
        case Qt::Key_T:
            mediaAction(actions.IncreaseXLowerRange);
            break;
        case Qt::Key_Control:
            mediaAction(actions.ResetLiveXRange);
            break;
        case Qt::Key_I:
            mediaAction(actions.ToggleFunscriptInvert);
            break;
        case Qt::Key_V:
            mediaAction(actions.ToggleAxisMultiplier);
            break;
    }
}
void MainWindow::mediaAction(QString action)
{
    MediaActions actions;
    if (action == actions.TogglePause)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying())
        {
            videoHandler->togglePause();
        }
        else if(SettingsHandler::getDeoEnabled() && _xSettings->getDeoHandler()->isConnected())
        {
            //_xSettings->getDeoHandler()->togglePause();
        }
        else
        {
            on_PlayBtn_clicked();
        }
    }
    else if(action == actions.FullScreen)
    {
        MainWindow::toggleFullScreen();
    }
    else if(action == actions.Mute)
    {
        MainWindow::on_MuteBtn_toggled(!videoHandler->isMute());
    }
    else if(action == actions.Stop)
    {
        MainWindow::on_StopBtn_clicked();
    }
     else if(action == actions.Next)
    {
        MainWindow::skipForward();
    }
    else if(action == actions.Back)
    {
        MainWindow::skipBack();
    }
    else if(action == actions.VolumeUp)
    {
        int currentVolume = ui->VolumeSlider->GetUpperValue();
        int maxVolume = ui->VolumeSlider->GetMaximum();
        int newVolume = currentVolume - maxVolume <= 5 ? currentVolume + 5 : maxVolume;
        ui->VolumeSlider->setUpperValue(newVolume);
        on_VolumeSlider_valueChanged(newVolume);
    }
    else if(action == actions.VolumeDown)
    {
        int currentVolume = ui->VolumeSlider->GetUpperValue();
        int minVolume = ui->VolumeSlider->GetMinimum();
        int newVolume = currentVolume - minVolume  >= 5 ? currentVolume - 5 : minVolume;
        ui->VolumeSlider->setUpperValue(newVolume);
        on_VolumeSlider_valueChanged(newVolume);
    }
    else if(action == actions.Loop)
    {
        toggleLoop();
    }
    else if(action == actions.Rewind)
    {
        if (videoHandler->isPlaying())
        {
            rewind();
        }
    }
    else if(action == actions.FastForward)
    {
        if (videoHandler->isPlaying())
        {
            fastForward();
        }
    }
    else if(action == actions.TCodeSpeedUp)
    {
        int newGamepadSpeed = SettingsHandler::getLiveGamepadSpeed() + SettingsHandler::getGamepadSpeedIncrement();
        SettingsHandler::setLiveGamepadSpeed(newGamepadSpeed);
        if(!SettingsHandler::getDisableSpeechToText())
            textToSpeech->say("Raise speed "+ QString::number(newGamepadSpeed));
    }
    else if(action == actions.TCodeSpeedDown)
    {
        int newGamepadSpeed = SettingsHandler::getLiveGamepadSpeed() - SettingsHandler::getGamepadSpeedIncrement();
        if (newGamepadSpeed > 0)
        {
            SettingsHandler::setLiveGamepadSpeed(newGamepadSpeed);
            if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("Lower speed "+ QString::number(newGamepadSpeed));
        } else if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("Lower speed at minimum");
    }
    else if(action == actions.IncreaseXLowerRange)
    {
        AxisNames axisNames;
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveRange = SettingsHandler::getLiveXRangeMin() + SettingsHandler::getXRangeStep();
        int xRangeMax = SettingsHandler::getLiveXRangeMax();
        if(newLiveRange < xRangeMax - xRangeStep)
        {
            SettingsHandler::setLiveXRangeMin(newLiveRange);
            if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("Raise X min to "+ QString::number(newLiveRange));
        }
        else
        {
            SettingsHandler::setLiveXRangeMin(xRangeMax - xRangeStep);
            if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("X Min limit reached");
        }
    }
    else if(action == actions.DecreaseXLowerRange)
    {
        AxisNames axisNames;
        int newLiveRange = SettingsHandler::getLiveXRangeMin() - SettingsHandler::getXRangeStep();
        int axisMin = SettingsHandler::getAxis(axisNames.TcXUpDownL0).Min;
        if(newLiveRange > axisMin)
        {
            SettingsHandler::setLiveXRangeMin(newLiveRange);
            if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("Lower X min to "+ QString::number(newLiveRange));
        }
        else
        {
            SettingsHandler::setLiveXRangeMin(axisMin);
            if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("Low X min limit reached");
        }
    }
    else if(action == actions.IncreaseXUpperRange)
    {
        AxisNames axisNames;
        int newLiveRange = SettingsHandler::getLiveXRangeMax() + SettingsHandler::getXRangeStep();
        int axisMax = SettingsHandler::getAxis(axisNames.TcXUpDownL0).Max;
        if(newLiveRange < axisMax)
        {
            SettingsHandler::setLiveXRangeMax(newLiveRange);
            if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("Raise X max to "+ QString::number(newLiveRange));
        }
        else
        {
            SettingsHandler::setLiveXRangeMax(axisMax);
            if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("High X max limit reached");
        }
    }
    else if(action == actions.DecreaseXUpperRange)
    {
        AxisNames axisNames;
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveRange = SettingsHandler::getLiveXRangeMax() - xRangeStep;
        int xRangeMin = SettingsHandler::getLiveXRangeMin();
        if(newLiveRange > xRangeMin + xRangeStep)
        {
            SettingsHandler::setLiveXRangeMax(newLiveRange);
            if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("Lower X max to "+ QString::number(newLiveRange));
        }
        else
        {
            SettingsHandler::setLiveXRangeMax(xRangeMin + xRangeStep);
            if(!SettingsHandler::getDisableSpeechToText())
                textToSpeech->say("Low X max limit reached");
        }
    }
    else if (action == actions.IncreaseXRange)
    {
        AxisNames axisNames;
        int xRangeMax = SettingsHandler::getLiveXRangeMax();
        int xRangeMin = SettingsHandler::getLiveXRangeMin();
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveMaxRange = xRangeMax + xRangeStep;
        int limitXMax = 0;
        int axisMax = SettingsHandler::getAxis(axisNames.TcXUpDownL0).Max;
        if(newLiveMaxRange < axisMax)
        {
            SettingsHandler::setLiveXRangeMax(newLiveMaxRange);
        }
        else
        {
            limitXMax = axisMax;
            SettingsHandler::setLiveXRangeMax(axisMax);
        }

        int newLiveMinRange = xRangeMin - xRangeStep;
        int axisMin = SettingsHandler::getAxis(axisNames.TcXUpDownL0).Min;
        int limitXMin = 0;
        if(newLiveMinRange > axisMin)
        {
            SettingsHandler::setLiveXRangeMin(newLiveMinRange);
        }
        else
        {
            limitXMin = axisMin;
            SettingsHandler::setLiveXRangeMin(axisMin);
        }

        if(!SettingsHandler::getDisableSpeechToText())
        {
            if (limitXMax && limitXMin)
                textToSpeech->say("Increase X at limit");
            else if (limitXMax)
                textToSpeech->say("Increase X, "+ QString::number(newLiveMinRange) + ", max at limit");
            else if (limitXMin)
                textToSpeech->say("Increase X, "+ QString::number(newLiveMaxRange) + ", min at limit");
            else
                textToSpeech->say("Increase X, "+ QString::number(newLiveMaxRange) + ", "+ QString::number(newLiveMinRange));
        }

    }
    else if (action == actions.DecreaseXRange)
    {
        AxisNames axisNames;
        int xRangeMax = SettingsHandler::getLiveXRangeMax();
        int xRangeMin = SettingsHandler::getLiveXRangeMin();
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveMaxRange = xRangeMax - xRangeStep;
        int limitXMax = 0;
        if(newLiveMaxRange > xRangeMin + xRangeStep)
        {
            SettingsHandler::setLiveXRangeMax(newLiveMaxRange);
        }
        else
        {
            limitXMax = xRangeMin + xRangeStep;
            SettingsHandler::setLiveXRangeMax(xRangeMin + xRangeStep);
        }

        int newLiveMinRange = xRangeMin + xRangeStep;
        int limitXMin = 0;
        if(newLiveMinRange < xRangeMax - xRangeStep)
        {
            SettingsHandler::setLiveXRangeMin(newLiveMinRange);
        }
        else
        {
            limitXMin = xRangeMax - xRangeStep;
            SettingsHandler::setLiveXRangeMin(xRangeMax - xRangeStep);
        }
        if(!SettingsHandler::getDisableSpeechToText())
        {
            if (limitXMax && limitXMin)
                textToSpeech->say("Decrease X at limit");
            else if (limitXMax)
                textToSpeech->say("Decrease X, "+ QString::number(newLiveMinRange) + ", max at limit");
            else if (limitXMin)
                textToSpeech->say("Decrease X, "+ QString::number(newLiveMaxRange) + ", min at limit");
            else
                textToSpeech->say("Decrease X, "+ QString::number(newLiveMaxRange) + ", "+ QString::number(newLiveMinRange));
        }
    }
    else if (action == actions.ResetLiveXRange)
    {
        if(!SettingsHandler::getDisableSpeechToText())
            textToSpeech->say("Resetting X range");
        SettingsHandler::resetLiveXRange();
    }
    else if (action == actions.ToggleAxisMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierEnabled();
        textToSpeech->say(multiplier ? "Disable multiplier" : "Enable multiplier");
        SettingsHandler::setLiveMultiplierEnabled(!multiplier);
    }
    else if (action == actions.ToggleFunscriptInvert)
    {
        bool inverted = FunscriptHandler::getInverted();
        textToSpeech->say(inverted ? "Funscript normal" : "Funscript inverted");
        FunscriptHandler::setInverted(!inverted);
    }
}

void MainWindow::onLibraryList_ContextMenuRequested(const QPoint &pos)
{
    // Handle global position
    QPoint globalPos = ui->LibraryList->mapToGlobal(pos);

    // Create menu and insert some actions
    QMenu myMenu;

    myMenu.addAction("Play", this, &MainWindow::playFileFromContextMenu);
    myMenu.addAction("Play with funscript...", this, &MainWindow::playFileWithCustomScript);
    myMenu.addAction("Regenerate thumbnail", this, &MainWindow::regenerateThumbNail);
    myMenu.addAction("Set thumbnail from current", this, &MainWindow::setThumbNailFromCurrent);

    // Show context menu at handling position
    myMenu.exec(globalPos);
}

void MainWindow::changeDeoFunscript()
{
    VRPacket playingPacket = _xSettings->getDeoHandler()->getCurrentPacket();
    if (playingPacket.path != nullptr)
    {
        QFileInfo videoFile(playingPacket.path);
        funscriptFileSelectorOpen = true;
        QString funscriptPath = QFileDialog::getOpenFileName(this, "Choose script for video: " + videoFile.fileName(), SettingsHandler::getSelectedLibrary(), "Script Files (*.funscript)");
        funscriptFileSelectorOpen = false;
        if (!funscriptPath.isEmpty())
        {
            SettingsHandler::setDeoDnlaFunscript(playingPacket.path, funscriptPath);
            funscriptHandler->setLoaded(false);
        }
    }
    else
    {
        LogHandler::Dialog("No packet for current video or no video playing", XLogLevel::Information);
    }
}

void MainWindow::on_load_library(QString path)
{
    if (!path.isNull() && !path.isEmpty())
    {
        QString thumbPath = QCoreApplication::applicationDirPath() + "/thumbs/";
        QDir thumbDir(thumbPath);
        if (!thumbDir.exists())
        {
            thumbDir.mkdir(thumbPath);
        }
        QDir directory(path);
        if (directory.exists())
        {
            ui->LibraryList->clear();
            libraryItems.clear();
            QDirIterator library(path, QStringList()
                        << "*.mp4"
                        << "*.avi"
                        << "*.mpg"
                        << "*.wmv"
                        << "*.mkv"
                        << "*.webm"
                        << "*.mp2"
                        << "*.mpeg"
                        << "*.mpv"
                        << "*.ogg"
                        << "*.m4p"
                        << "*.m4v"
                        << "*.mov"
                        << "*.qt"
                        << "*.flv"
                        << "*.swf"
                        << "*.avchd", QDir::Files, QDirIterator::Subdirectories);
            while (library.hasNext())
            {
                QFileInfo fileinfo(library.next());
                QString videoPath = fileinfo.filePath();
                QString videoPathTemp = fileinfo.filePath();
                QString fileName = fileinfo.fileName();
                QString fileNameTemp = fileinfo.fileName();
                QString fileNameNoExtension = fileNameTemp.remove(fileNameTemp.lastIndexOf('.'), fileNameTemp.length() -  1);
                QString scriptFile = fileNameNoExtension + ".funscript";
                QString scriptPath;
                QString scriptNoExtension = videoPathTemp.remove(videoPathTemp.lastIndexOf('.'), videoPathTemp.length() -  1);
                if (SettingsHandler::getSelectedFunscriptLibrary() == Q_NULLPTR)
                {
                    scriptPath = scriptNoExtension + ".funscript";
                }
                else
                {
                    scriptNoExtension = SettingsHandler::getSelectedFunscriptLibrary() + QDir::separator() + fileNameNoExtension;
                    scriptPath = SettingsHandler::getSelectedFunscriptLibrary() + QDir::separator() + scriptFile;
                }
                if (!funscriptHandler->exists(scriptPath))
                {
                    scriptPath = nullptr;
                }

                //QFileInfo scriptZip(scriptNoExtension + ".zip");
                QString zipFile;
//                if(scriptZip.exists())
//                    zipFile = scriptNoExtension + ".zip";

                QString thumbFile =  thumbPath + fileName + ".jpg";
                LibraryListItem item
                {
                    videoPath, // path
                    fileName, // name
                    fileNameNoExtension, //nameNoExtension
                    scriptPath, // script
                    scriptNoExtension,
                    thumbFile,
                    zipFile,
                    QDate::currentDate(),
                    0
                };
                QVariant listItem;
                listItem.setValue(item);
                QListWidgetItem* qListWidgetItem = new QListWidgetItem;
                if (!funscriptHandler->exists(scriptPath))
                {
                    qListWidgetItem->setToolTip(videoPath + "\nNo script file of the same name found.\nRight click and Play with funscript.");
                    qListWidgetItem->setForeground(QColorConstants::Gray);
                }
                else
                {
                    qListWidgetItem->setToolTip(videoPath);
                }

                QFileInfo thumbInfo(thumbFile);
                QString thumbString = thumbFile;
                if (!thumbInfo.exists())
                {
                    thumbString = QApplication::applicationDirPath() + "/themes/loading.png";
                }
                QIcon thumb;
                QPixmap bgPixmap(thumbString);
                QPixmap scaled = bgPixmap.scaled(_currentMaxThumbSize, Qt::AspectRatioMode::KeepAspectRatio);
                thumb.addPixmap(scaled);
                qListWidgetItem->setIcon(thumb);
                qListWidgetItem->setSizeHint({SettingsHandler::getThumbSize(), SettingsHandler::getThumbSize()});
                qListWidgetItem->setText(fileinfo.fileName());
                qListWidgetItem->setData(Qt::UserRole, listItem);
                ui->LibraryList->addItem(qListWidgetItem);
                libraryItems.push_back(qListWidgetItem);
            }
            stopThumbProcess = true;
            saveNewThumbs();
        }
        else
        {
           LogHandler::Dialog("Library path '" + path + "' does not exist anymore!", XLogLevel::Critical);
           on_actionSelect_library_triggered();
        }
    }
    else
    {
        on_actionSelect_library_triggered();
    }
}
int thumbNailSearchIterator = 0;
void MainWindow::saveNewThumbs()
{
    if (stopThumbProcess)
    {
        stopThumbProcess = false;
        thumbNailSearchIterator = 0;
    }
    if (thumbNailSearchIterator < libraryItems.count())
    {
        QListWidgetItem* listWidgetItem = libraryItems.at(thumbNailSearchIterator);
        LibraryListItem item = getLibraryListItemFromQListItem(listWidgetItem);
        thumbNailSearchIterator++;
        QFileInfo thumbInfo(item.thumbFile);
        if (!thumbInfo.exists())
        {
            saveThumb(item.path, item.thumbFile, listWidgetItem);
        }
        else
        {
            saveNewThumbs();
        }
    }
    else
    {
        stopThumbProcess = false;
        thumbNailSearchIterator = 0;
    }


}
void MainWindow::saveThumb(const QString& videoFile, const QString& thumbFile, QListWidgetItem* qListWidgetItem, qint64 position)
{
    QIcon thumb;
    QPixmap bgPixmap(QApplication::applicationDirPath() + "/themes/loading.png");
    QPixmap scaled = bgPixmap.scaled(_currentMaxThumbSize, Qt::AspectRatioMode::KeepAspectRatio);
    thumb.addPixmap(scaled);
    qListWidgetItem->setIcon(thumb);
    VideoFrameExtractor* extractor = new VideoFrameExtractor;

    connect(extractor,
           &QtAV::VideoFrameExtractor::frameExtracted,
           extractor,
           [this, extractor, videoFile, thumbFile, qListWidgetItem](const QtAV::VideoFrame& frame)
            {

               const auto& img = frame.toImage();
               QString thumbFileTemp = thumbFile;

               if (!img.save(thumbFile, nullptr, 15))
               {
                   //LogHandler::Debug("Error saving thumbnail: " + thumbFile + " for video: " + videoFile);
                   thumbFileTemp = "://images/icons/error.png";
               }
               QIcon thumb;
               QPixmap bgPixmap(thumbFileTemp);
               QPixmap scaled = bgPixmap.scaled(_currentMaxThumbSize, Qt::AspectRatioMode::KeepAspectRatio);
               thumb.addPixmap(scaled);
               qListWidgetItem->setIcon(thumb);
               if(thumbNailSearchIterator > 0)
               {
                   saveNewThumbs();
               }
               extractor->deleteLater();
            });
    connect(extractor,
           &QtAV::VideoFrameExtractor::error,
           extractor,
           [this, extractor, videoFile, qListWidgetItem](const QString &errorMessage)
            {

               //LogHandler::Debug("Error accessing video file: " + videoFile + " Error: " + errorMessage);

               QIcon thumb;
               QPixmap bgPixmap("://images/icons/error.png");
               QPixmap scaled = bgPixmap.scaled(_currentMaxThumbSize);
               thumb.addPixmap(scaled);
               qListWidgetItem->setIcon(thumb);
               if(thumbNailSearchIterator > 0)
               {
                   saveNewThumbs();
               }
               extractor->deleteLater();
            });

    extractor->setAsync(true);
    extractor->setSource(videoFile);

    AVPlayer* thumbNailPlayer = new AVPlayer;

    thumbNailPlayer->setAsyncLoad(true);
    thumbNailPlayer->setFile(videoFile);
    thumbNailPlayer->load();
    connect(thumbNailPlayer,
           &AVPlayer::loaded,
           thumbNailPlayer,
           [thumbNailPlayer, extractor, position]()
            {
               //LogHandler::Debug("Loaded video for thumb duration: "+ QString::number(thumbNailPlayer->duration()));
               qint64 randomPosition = position > 0 ? position : XMath::rand(1, thumbNailPlayer->duration());
               //LogHandler::Debug("randomPosition: " + QString::number(randomPosition));
               extractor->setPosition(randomPosition);
               thumbNailPlayer->deleteLater();
            });

    connect(thumbNailPlayer,
           &AVPlayer::error,
           thumbNailPlayer,
           [this, thumbNailPlayer, extractor, position]()
            {

               //LogHandler::Debug("Video load error");
               qint64 randomPosition = XMath::rand(1, position > 0 ? position : thumbCaptureTime);
               //LogHandler::Debug("randomPosition: " + QString::number(randomPosition));
               extractor->setPosition(randomPosition);
               thumbNailPlayer->deleteLater();
            });
}

void MainWindow::on_libray_path_select(QString path)
{
    this->on_load_library(path);
}

void MainWindow::on_actionSelect_library_triggered()
{
    QString selectedLibrary = QFileDialog::getExistingDirectory(this, tr("Choose media library"), ".", QFileDialog::ReadOnly);
    if (selectedLibrary != Q_NULLPTR) {
        on_libray_path_select(selectedLibrary);

        SettingsHandler::setSelectedLibrary(selectedLibrary);
    }
}

void MainWindow::on_LibraryList_itemClicked(QListWidgetItem *item)
{
//    if(!videoHandler->isPlaying())
//    {
//        loadVideo(item->data(Qt::UserRole).value<LibraryListItem>());
//    }
}

void MainWindow::on_LibraryList_itemDoubleClicked(QListWidgetItem *item)
{
    playVideo(item->data(Qt::UserRole).value<LibraryListItem>());
}

void MainWindow::regenerateThumbNail()
{
    QListWidgetItem* selectedItem = ui->LibraryList->selectedItems().first();
    LibraryListItem selectedFileListItem = getLibraryListItemFromQListItem(selectedItem);
    saveThumb(selectedFileListItem.path, selectedFileListItem.thumbFile, selectedItem);
}

void MainWindow::setThumbNailFromCurrent()
{
    QListWidgetItem* selectedItem = ui->LibraryList->selectedItems().first();
    LibraryListItem selectedFileListItem = getLibraryListItemFromQListItem(selectedItem);
    saveThumb(selectedFileListItem.path, selectedFileListItem.thumbFile, selectedItem, videoHandler->position());
}

void MainWindow::playFileFromContextMenu()
{
    LibraryListItem selectedFileListItem = getLibraryListItemFromQListItem(ui->LibraryList->selectedItems().first());
    playVideo(selectedFileListItem);
}

void MainWindow::playFileWithCustomScript()
{
    QString selectedScript = QFileDialog::getOpenFileName(this, tr("Choose script"), SettingsHandler::getSelectedLibrary(), tr("Script Files (*.funscript)"));
    if (selectedScript != Q_NULLPTR)
    {
        LibraryListItem selectedFileListItem = getLibraryListItemFromQListItem(ui->LibraryList->selectedItems().first());
        playVideo(selectedFileListItem, selectedScript);
    }
}

void MainWindow::playVideo(LibraryListItem selectedFileListItem, QString customScript)
{
    QFile file(selectedFileListItem.path);
    if (file.exists())
    {
        ui->loopToggleButton->setChecked(false);
        //on_loopToggleButton_toggled(false);
        ui->videoLoadingLabel->show();
        _movie->start();
        videoHandler->stop();
        if (videoHandler->file() != selectedFileListItem.path)
        {
            videoHandler->setFile(selectedFileListItem.path);
            videoPreviewWidget->setFile(selectedFileListItem.path);
            videoHandler->load();
            if(!selectedFileListItem.zipFile.isEmpty())
            {
//                asmZip zipFile(selectedFileListItem.zipFile);

//                if ( zipFile.isValid() )
//                {
//                   zipFile.listFiles();
//                   QByteArray  data;
//                   AxisNames axisNames;
//                   auto err = zipFile.extractFile( "x.funscript", data );

//                   if (err != asmZip::NO_ERR)
//                        funscriptHandler->load(data);
//                }
            }
            else
            {
                QString scriptFile = customScript == nullptr ? selectedFileListItem.script : customScript;
                funscriptHandler->load(scriptFile);
            }

            qDeleteAll(funscriptHandlers);
            funscriptHandlers.clear();
            _xSettings->getSelectedDeviceHandler()->sendTCode(tcodeHandler->getHome());

            AxisNames axisNames;
            foreach(auto axisName, axisNames.BasicAxis)
            {
                if(axisName.first == axisNames.TcXUpDownL0)
                    continue;
                if(!selectedFileListItem.zipFile.isEmpty())
                {
//                    asmZip zipFile(selectedFileListItem.zipFile);

//                    if ( zipFile.isValid() )
//                    {
//                       zipFile.listFiles();
//                       QByteArray  data;

//                       auto err = zipFile.extractFile(axisName.second.trackName + ".funscript", data);
//                       if (err != asmZip::NO_ERR)
//                       {
//                           FunscriptHandler* otherFunscript = new FunscriptHandler(axisName.first);
//                           otherFunscript->load(data);
//                           funscriptHandlers.append(otherFunscript);
//                       }
//                    }
                }
                else
                {
                    auto scriptFileTemp = customScript == nullptr ? selectedFileListItem.script : customScript;
                    auto funscriptNoExtension = scriptFileTemp.remove(scriptFileTemp.lastIndexOf('.'), scriptFileTemp.length() -  1);
                    QFileInfo fileInfo(funscriptNoExtension + "." + axisName.second.trackName + ".funscript");
                    if(fileInfo.exists())
                    {
                        FunscriptHandler* otherFunscript = new FunscriptHandler(axisName.first);
                        otherFunscript->load(fileInfo.absoluteFilePath());
                        funscriptHandlers.append(otherFunscript);
                    }
                }
            }
        }
        //QUrl url = QUrl::fromLocalFile(selectedFileListItem.path);
        videoHandler->play();
        playingVideoListIndex = ui->LibraryList->currentRow();
        if(!funscriptHandler->isLoaded())
        {
            LogHandler::Dialog("Error loading script " + customScript + "!\nTry right clicking on the video in the list\nand loading with another script.", XLogLevel::Warning);

        }
    }
    else
    {
        LogHandler::Dialog("File '" + selectedFileListItem.path + "' does not exist!", XLogLevel::Critical);
    }
}

void MainWindow::on_mainwindow_change(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        QWindowStateChangeEvent* stateEvent = (QWindowStateChangeEvent*)event;
        if(stateEvent->oldState() == Qt::WindowState::WindowMaximized && !_isFullScreen && !_isMaximized)
        {
            this->resize(_defaultAppSize);
            QMainWindow::move(_appPos.x() < 10 ? 10 : _appPos.x(), _appPos.y() < 10 ? 10 : _appPos.y());
        }
        else if(stateEvent->oldState() == Qt::WindowState::WindowMaximized && !_isFullScreen)
        {
            _isMaximized = false;
        }
    }
}

void MainWindow::toggleFullScreen()
{
    if(!_isFullScreen)
    {
        QScreen *screen = this->window()->windowHandle()->screen();
        QSize screenSize = screen->size();
        _videoSize = videoHandler->size();
        _appSize = this->size();
        _appPos = this->pos();
        _isMaximized = this->isMaximized();
        _isFullScreen = true;
        QMainWindow::setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        ui->MediaGrid->removeWidget(videoHandler);
        ui->ControlsGrid->removeWidget(ui->playerControlsFrame);

        placeHolderControlsGrid = new QGridLayout(this);
        placeHolderControlsGrid->setContentsMargins(0,0,0,0);
        placeHolderControlsGrid->setSpacing(0);
        placeHolderControlsGrid->addWidget(ui->playerControlsFrame);

        playerControlsPlaceHolder = new QFrame(this);
        playerControlsPlaceHolder->setLayout(placeHolderControlsGrid);
        playerControlsPlaceHolder->setContentsMargins(0,0,0,0);
        playerControlsPlaceHolder->installEventFilter(this);
        playerControlsPlaceHolder->move(QPoint(0, screenSize.height() - ui->playerControlsFrame->height()));
        playerControlsPlaceHolder->setFixedWidth(screenSize.width());
        playerControlsPlaceHolder->setFixedHeight(ui->playerControlsFrame->height());

        ui->fullScreenGrid->addWidget(videoHandler, 0, 0, 2, 1);
        ui->fullScreenGrid->addWidget(playerControlsPlaceHolder, 1, 0);
        ui->playerControlsFrame->setProperty("cssClass", "fullScreenControls");
        ui->playerControlsFrame->style()->unpolish(ui->playerControlsFrame);
        ui->playerControlsFrame->style()->polish(ui->playerControlsFrame);
        ui->mainStackedWidget->setCurrentIndex(1);
        QMainWindow::resize(screenSize);
        QMainWindow::centralWidget()->layout()->setMargin(0);
        QMainWindow::showFullScreen();
        videoHandler->layout()->setMargin(0);
        ui->mainStackedWidget->move(QPoint(0, 0));
        videoHandler->move(QPoint(0, 0));
        //videoHandler->resize(QSize(screenSize.width()+1, screenSize.height()+1));
        ui->playerControlsFrame->hide();
        ui->menubar->hide();
        ui->statusbar->hide();
        videoHandler->resize(screenSize);
        QMainWindow::setFocus();
    }
    else
    {

        ui->fullScreenGrid->removeWidget(videoHandler);
        playerControlsPlaceHolder->layout()->removeWidget(ui->playerControlsFrame);
        videoHandler->resize(_videoSize);
        ui->MediaGrid->addWidget(videoHandler);
        ui->fullScreenGrid->removeWidget(playerControlsPlaceHolder);
        ui->playerControlsFrame->setWindowFlags(Qt::Widget);
        ui->playerControlsFrame->setMinimumSize(QSize(700, 0));
        ui->playerControlsFrame->setMaximumSize(QSize(16777215, 16777215));
        ui->ControlsGrid->addWidget(ui->playerControlsFrame);
        ui->playerControlsFrame->setProperty("cssClass", "windowedControls");
        ui->playerControlsFrame->style()->unpolish(ui->playerControlsFrame);
        ui->playerControlsFrame->style()->polish(ui->playerControlsFrame);
        videoHandler->layout()->setMargin(9);
        QMainWindow::centralWidget()->layout()->setMargin(9);

        ui->mainStackedWidget->setCurrentIndex(0);

        QMainWindow::setWindowFlags(Qt::WindowFlags());
        if(_isMaximized)
        {
            QMainWindow::showMaximized();
        }
        else
        {
            QMainWindow::resize(_appSize);
            QMainWindow::move(_appPos.x() < 100 ? 100 : _appPos.x(), _appPos.y() < 100 ? 100 : _appPos.y());
            QMainWindow::showNormal();
        }
        _isFullScreen = false;
        ui->menubar->show();
        ui->statusbar->show();
        ui->playerControlsFrame->show();
        delete placeHolderControlsGrid;
        delete playerControlsPlaceHolder;
    }
}

void MainWindow::toggleLoop()
{
    if(!ui->loopToggleButton->isChecked() && !autoLoopOn)
    {
        autoLoopOn = true;
        ui->loopToggleButton->setChecked(true);
        qint64 currentVideoPositionPercentage = XMath::mapRange(videoHandler->position(),  (qint64)0, videoHandler->duration(), (qint64)0, (qint64)100);
        ui->SeekSlider->setLowerValue(currentVideoPositionPercentage);
    }
    else if (ui->loopToggleButton->isChecked() && autoLoopOn)
    {
        autoLoopOn = false;
        int lowerValue = ui->SeekSlider->GetLowerValue();
        qint64 currentVideoPositionPercentage = XMath::mapRange(videoHandler->position(),  (qint64)0, videoHandler->duration(), (qint64)0, (qint64)100);
        ui->SeekSlider->setUpperValue(currentVideoPositionPercentage > lowerValue + ui->SeekSlider->GetMinimumRange()
                                      ? currentVideoPositionPercentage : currentVideoPositionPercentage + lowerValue + ui->SeekSlider->GetMinimumRange());
    }
    else
    {
        ui->loopToggleButton->setChecked(false);
    }
}

void MainWindow::hideControls()
{
    if (_isFullScreen)
    {
        ui->playerControlsFrame->hide();
    }
}

void MainWindow::showControls()
{
    if (_isFullScreen)
    {
        ui->playerControlsFrame->show();
    }
}


void MainWindow::on_VolumeSlider_valueChanged(int value)
{
    if(!videoHandler->isMute())
    {
        videoHandler->setVolume(value);
        SettingsHandler::setPlayerVolume(value);
    }
    setVolumeIcon(value);
    ui->VolumeSlider->setToolTip(QString::number(value));
}

void MainWindow::on_PlayBtn_clicked()
{
    if (ui->LibraryList->count() > 0)
    {
        if(ui->LibraryList->selectedItems().length() == 0)
        {
            ui->LibraryList->setCurrentRow(0);
        }
        LibraryListItem selectedFileListItem = getLibraryListItemFromQListItem(ui->LibraryList->selectedItems().first());
        if(selectedFileListItem.path != videoHandler->file() || !videoHandler->isPlaying())
        {
            playVideo(selectedFileListItem);
        }
        else if(ui->LibraryList->selectedItems().length() == 0)
        {
            playingVideoListIndex = 0;
            ui->LibraryList->setCurrentRow(0);
            playVideo(ui->LibraryList->selectedItems()[0]->data(Qt::UserRole).value<LibraryListItem>());
        }
        else if(videoHandler->isPaused() || videoHandler->isPlaying())
        {
            videoHandler->togglePause();
        }
    }
}

void MainWindow::on_StopBtn_clicked()
{
    if(videoHandler->isPlaying())
    {
        videoHandler->stop();
    }
    QIcon icon("://images/icons/play.svg");
    ui->PlayBtn->setIcon(icon);
}

void MainWindow::onVideoHandler_togglePaused(bool paused)
{
    QIcon icon(paused ? "://images/icons/play.svg" : "://images/icons/pause.svg" );
    ui->PlayBtn->setIcon(icon);
}

void MainWindow::on_MuteBtn_toggled(bool checked)
{
    if (checked)
    {
        voulumeBeforeMute = ui->VolumeSlider->GetUpperValue();
        setVolumeIcon(0);
        ui->VolumeSlider->setUpperValue(0);
    }
    else
    {
        setVolumeIcon(voulumeBeforeMute);
        ui->VolumeSlider->setUpperValue(voulumeBeforeMute);
    }
    videoHandler->toggleMute();
}

void MainWindow::setVolumeIcon(int volume)
{
    if (volume > 15)
        ui->MuteBtn->setIcon(QIcon(":/images/icons/speakerLoud.svg"));
    else if (volume > 0)
        ui->MuteBtn->setIcon(QIcon(":/images/icons/speakerMid.svg"));
    else
        ui->MuteBtn->setIcon(QIcon(":/images/icons/speakerMute.svg"));
}

void MainWindow::on_fullScreenBtn_clicked()
{
    MainWindow::toggleFullScreen();
}

void MainWindow::on_seekslider_hover(int position, int sliderValue)
{
//    if(!isFullScreen())
//    {
        qint64 sliderValueTime = XMath::mapRange(static_cast<qint64>(sliderValue), (qint64)0, (qint64)100, (qint64)0, videoHandler->duration());
    //    if (!videoPreviewWidget)
    //        videoPreviewWidget = new VideoPreviewWidget();
        videoPreviewWidget->setTimestamp(sliderValueTime);
        videoPreviewWidget->preview();
    //    LogHandler::Debug("mousePosition: "+QString::number(sliderValue));
    //    LogHandler::Debug("time: "+QString::number(sliderValueTime));
    //    LogHandler::Debug("position: "+QString::number(position));
        QPoint gpos;
        if(_isFullScreen)
        {
            gpos = mapToGlobal(playerControlsPlaceHolder->pos() + ui->SeekSlider->pos() + QPoint(position, 0));
        }
        else
        {
            gpos = mapToGlobal(ui->playerControlsFrame->pos() + ui->SeekSlider->pos() + QPoint(position, 0));
        }
    //    LogHandler::Debug("gpos x: "+QString::number(gpos.x()));
    //    LogHandler::Debug("gpos y: "+QString::number(gpos.y()));
        QToolTip::showText(gpos, QTime(0, 0, 0).addMSecs(sliderValueTime).toString(QString::fromLatin1("HH:mm:ss")));
    //    if (!Config::instance().previewEnabled())
    //        return;

       //const int w = Config::instance().previewWidth();
        //const int h = Config::instance().previewHeight();
        videoPreviewWidget->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        videoPreviewWidget->resize(175, 100);
        videoPreviewWidget->move(gpos - QPoint(175/2, 100));
        videoPreviewWidget->show();
        //videoPreviewWidget->raise();
        //videoPreviewWidget->activateWindow();
    //}
}

void MainWindow::on_seekslider_leave()
{
    if (!videoPreviewWidget)
    {
        return;
    }
    if (videoPreviewWidget->isVisible())
    {
        videoPreviewWidget->close();
    }
//    delete videoPreviewWidget;
//    videoPreviewWidget = NULL;
}

void MainWindow::on_seekSlider_sliderMoved(int position)
{
    if (!ui->loopToggleButton->isChecked())
    {
        qint64 playerPosition = XMath::mapRange(static_cast<qint64>(position), (qint64)0, (qint64)100, (qint64)0, videoHandler->duration());
        ui->SeekSlider->setToolTip(QTime(0, 0, 0).addMSecs(position).toString(QString::fromLatin1("HH:mm:ss")));
        videoHandler->setPosition(playerPosition);
    }
}


void MainWindow::on_SeekSlider_valueChanged(int position)
{

}

void MainWindow::onLoopRange_valueChanged(int position)
{
    int endLoop = ui->SeekSlider->GetUpperValue();
    int startLoop = ui->SeekSlider->GetLowerValue();
    qint64 duration = videoHandler->duration();

    qint64 currentVideoPositionPercentage = XMath::mapRange(videoHandler->position(),  (qint64)0, duration, (qint64)0, (qint64)100);
    qint64 destinationVideoPosition = XMath::mapRange((qint64)position, (qint64)0, (qint64)100,  (qint64)0, duration);

    QString timeCurrent = mSecondFormat(destinationVideoPosition);
    ui->SeekSlider->setToolTip(timeCurrent);

    if(currentVideoPositionPercentage < startLoop)
    {
        videoHandler->setPosition(destinationVideoPosition);
    }
    else if (currentVideoPositionPercentage >= endLoop)
    {
        qint64 startLoopVideoPosition = XMath::mapRange((qint64)startLoop, (qint64)0, (qint64)100,  (qint64)0, duration);
        if(startLoopVideoPosition <= 0)
            startLoopVideoPosition = 50;
        if (videoHandler->position() != startLoopVideoPosition)
            videoHandler->setPosition(startLoopVideoPosition);
    }
}

void MainWindow::on_media_positionChanged(qint64 position)
{
    qint64 duration = videoHandler->duration();
    qint64 videoToSliderPosition = XMath::mapRange(position,  (qint64)0, duration, (qint64)0, (qint64)100);
    if (!ui->loopToggleButton->isChecked())
    {
        if (duration > 0)
        {
            ui->SeekSlider->setUpperValue(static_cast<int>(videoToSliderPosition));
        }
    }
    else
    {
        int endLoop = ui->SeekSlider->GetUpperValue();
        qint64 endLoopToVideoPosition = XMath::mapRange((qint64)endLoop, (qint64)0, (qint64)100,  (qint64)0, duration);
        if (position >= endLoopToVideoPosition)
        {
            int startLoop = ui->SeekSlider->GetLowerValue();
            qint64 startLoopVideoPosition = XMath::mapRange((qint64)startLoop, (qint64)0, (qint64)100,  (qint64)0, duration);
            if(startLoopVideoPosition <= 0)
                startLoopVideoPosition = 50;
            if (videoHandler->position() != startLoopVideoPosition)
                videoHandler->setPosition(startLoopVideoPosition);
        }
    }
    ui->lblCurrentDuration->setText(mSecondFormat(position).append("/").append(mSecondFormat(duration)));
    //    QString timeCurrent = QTime(0, 0, 0).addMSecs(position).toString(QString::fromLatin1("HH:mm:ss"));
    //    QString timeDuration = QTime(0, 0, 0).addMSecs(duration).toString(QString::fromLatin1("HH:mm:ss"));
    //    QString timeStamp = timeCurrent.append("/").append(timeDuration);
    //    ui->lblCurrentDuration->setText(timeStamp);
}

QString MainWindow::mSecondFormat(int mSecs)
{
    int seconds = mSecs / 1000;
    mSecs %= 1000;

    int minutes = seconds / 60;
    seconds %= 60;

    int hours = minutes / 60;
    minutes %= 60;
    QString hr = QString::number(hours);
    QString mn = QString::number(minutes);
    QString sc = QString::number(seconds);

    return (hr.length() == 1 ? "0" + hr : hr ) + ":" + (mn.length() == 1 ? "0" + mn : mn ) + ":" + (sc.length() == 1 ? "0" + sc : sc);
}

void MainWindow::on_media_start()
{
    toggleMediaControlStatus();
    if(SettingsHandler::getDeoEnabled())
        _xSettings->getDeoHandler()->dispose();
    if(SettingsHandler::getWhirligigEnabled())
        _xSettings->getWhirligigHandler()->dispose();
    if(funscriptFuture.isRunning())
    {
        funscriptFuture.cancel();
        funscriptFuture.waitForFinished();
    }
    if (funscriptHandler->isLoaded())
    {
        funscriptFuture = QtConcurrent::run(syncFunscript, videoHandler, _xSettings, tcodeHandler, funscriptHandler, funscriptHandlers);
    }
}

void MainWindow::on_media_stop()
{
    toggleMediaControlStatus();
    if(funscriptFuture.isRunning())
    {
        funscriptFuture.cancel();
    }

}
void MainWindow::toggleMediaControlStatus()
{
    if(videoHandler->isPlaying())
    {
        ui->videoLoadingLabel->hide();
        _movie->stop();
        ui->SeekSlider->setDisabled(false);
        QIcon icon("://images/icons/pause.svg" );
        ui->PlayBtn->setIcon(icon);
    }
    else
    {
        ui->videoLoadingLabel->hide();
        _movie->stop();
        ui->SeekSlider->setUpperValue(0);
        ui->SeekSlider->setDisabled(true);
        QIcon icon("://images/icons/play.svg" );
        ui->PlayBtn->setIcon(icon);
        ui->lblCurrentDuration->setText("00:00:00/00:00:00");
    }
}
void MainWindow::onVRMessageRecieved(VRPacket packet)
{
        //LogHandler::Debug("VR path: "+packet.path);
//LogHandler::Debug("VR duration: "+QString::number(packet.duration));
        //LogHandler::Debug("VR currentTime: "+QString::number(packet.currentTime));
//        LogHandler::Debug("VR playbackSpeed: "+QString::number(packet.playbackSpeed));
//        LogHandler::Debug("VR playing: "+QString::number(packet.playing));

    if(!packet.path.isEmpty() && packet.path != vrScriptSelectedCanceledPath)
        vrScriptSelectorCanceled = false;

    if (!vrScriptSelectorCanceled && !packet.path.isEmpty() && !funscriptFileSelectorOpen && packet.duration > 0)
    {
        QString videoPath = packet.path;
        QFileInfo videoFile(videoPath);
        QString funscriptPath = SettingsHandler::getDeoDnlaFunscript(videoPath);
        QFileInfo funscriptFile(funscriptPath);
        if (funscriptPath == nullptr || !funscriptFile.exists())
        {
            funscriptHandler->setLoaded(false);
            //Check the deo device local video directory for funscript.
            int indexOfSuffix = packet.path.lastIndexOf(".");
            QString packetPath = packet.path;
            QString localFunscriptPath = packetPath.replace(indexOfSuffix, packet.path.length() - indexOfSuffix, ".funscript");
            QFile localFile(localFunscriptPath);
            LogHandler::Debug("Searching local path: "+localFunscriptPath);
            if(localFile.exists())
            {
                funscriptPath = localFunscriptPath;
            }
            else
            {
                //Check the user selected library location.
                QFileInfo fileinfo(packet.path);
                QString libraryScriptFile = fileinfo.fileName().remove(fileinfo.fileName().lastIndexOf('.'), fileinfo.fileName().length() -  1) + ".funscript";
                QString libraryScriptPath = SettingsHandler::getSelectedLibrary() + QDir::separator() + libraryScriptFile;
                QFile libraryFile(libraryScriptPath);
                if(libraryFile.exists())
                {
                    funscriptPath = libraryScriptPath;
                }
            }
            //If the above locations fail ask the user to select a file manually.
            if (funscriptPath == nullptr)
            {
                funscriptFileSelectorOpen = true;
                if (!SettingsHandler::getDisableSpeechToText())
                    textToSpeech->say("Script for video playing in Deeo VR not found. Please check your computer to select a script.");
                funscriptPath = QFileDialog::getOpenFileName(this, "Choose script for video: " + videoFile.fileName(), SettingsHandler::getSelectedLibrary(), "Script Files (*.funscript)");
                funscriptFileSelectorOpen = false;
                //LogHandler::Debug("funscriptPath: "+funscriptPath);
                if(funscriptPath.isEmpty())
                {
                    vrScriptSelectorCanceled = true;
                    vrScriptSelectedCanceledPath = packet.path;
                }

            }
            //Store the location of the file so the above check doesnt happen again.
            SettingsHandler::setDeoDnlaFunscript(videoPath, funscriptPath);
        }
    }
}

void syncVRFunscript(VRDeviceHandler* vrPlayer, VideoHandler* xPlayer, SettingsDialog* xSettings, TCodeHandler* tcodeHandler, FunscriptHandler* funscriptHandler)
{
    if(xPlayer->isPlaying())
    {
        xPlayer->stop();
        funscriptHandler->setLoaded(false);
    }
    AxisNames axisNames;
    DeviceHandler* device = xSettings->getSelectedDeviceHandler();
    QList<FunscriptHandler*> funscriptHandlers;
    std::shared_ptr<FunscriptAction> actionPosition;
    QMap<QString, std::shared_ptr<FunscriptAction>> otherActions;
    VRPacket currentVRPacket = vrPlayer->getCurrentPacket();
    QString currentVideo;
    qint64 timeTracker = 0;
    qint64 lastVRTime = 0;
    QElapsedTimer mSecTimer;
    qint64 timer1 = 0;
    qint64 timer2 = 0;
    //qint64 elapsedTracker = 0;
    //QElapsedTimer timer;
    //timer.start();
    mSecTimer.start();
    while (vrPlayer->isConnected() && !xPlayer->isPlaying())
    {
        //timer.start();
        if(xSettings->isConnected() && funscriptHandler->isLoaded() && !currentVRPacket.path.isNull() && currentVRPacket.duration > 0 && currentVRPacket.playing)
        {
            //execute once every millisecond
            if (timer2 - timer1 >= 1)
            {
//                LogHandler::Debug("timer "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
//                LogHandler::Debug("timer1: "+QString::number(timer1));
//                LogHandler::Debug("timer2: "+QString::number(timer2));
//                LogHandler::Debug("timer2 - timer1 "+QString::number(timer2-timer1));
//                LogHandler::Debug("Out timeTracker: "+QString::number(timeTracker));
                timer1 = timer2;
                qint64 currentTime = currentVRPacket.currentTime;
                //LogHandler::Debug("VR time reset: "+QString::number(currentTime));
                bool hasRewind = lastVRTime > currentTime;
                if (currentTime > timeTracker + 100 || hasRewind)
                {
                    lastVRTime = currentTime;
                    LogHandler::Debug("current time reset: " + QString::number(currentTime));
                    LogHandler::Debug("timeTracker: " + QString::number(timeTracker));
                    timeTracker = currentTime;
                }
                else
                {
                    timeTracker++;
                    currentTime = timeTracker;
                }
                //LogHandler::Debug("funscriptHandler->getPosition: "+QString::number(currentTime));
                actionPosition = funscriptHandler->getPosition(currentTime);
                foreach(auto funscriptHandlerOther, funscriptHandlers)
                {
                    auto otherAction = funscriptHandlerOther->getPosition(currentTime);
                    if(otherAction != nullptr)
                        otherActions.insert(funscriptHandlerOther->channel(), otherAction);
                }
                QString tcode = tcodeHandler->funscriptToTCode(actionPosition, otherActions);
                if(tcode != nullptr)
                    device->sendTCode(tcode);
                otherActions.clear();
                //LogHandler::Debug("timer "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
                //timer.start();
            }
            timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
            //LogHandler::Debug("timer nsecsElapsed: "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
        }
        else if(xSettings->isConnected() && currentVRPacket.duration > 0 && currentVRPacket.playing)
        {
            if (!currentVRPacket.path.isEmpty() && !currentVRPacket.path.isNull())
            {
                QString funscriptPath = SettingsHandler::getDeoDnlaFunscript(currentVRPacket.path);
                currentVideo = currentVRPacket.path;
                funscriptHandler->load(funscriptPath);

                qDeleteAll(funscriptHandlers);
                funscriptHandlers.clear();
                xSettings->getSelectedDeviceHandler()->sendTCode(tcodeHandler->getHome());

                foreach(auto axisName, axisNames.BasicAxis)
                {
                    if(axisName.first == axisNames.TcXUpDownL0)
                        continue;
                    QString funscriptPathTemp = funscriptPath;
                    auto funscriptNoExtension = funscriptPathTemp.remove(funscriptPathTemp.lastIndexOf('.'), funscriptPathTemp.length() -  1);
                    QFileInfo fileInfo(funscriptNoExtension + "." + axisName.second.trackName + ".funscript");
                    if(fileInfo.exists())
                    {
                        FunscriptHandler* otherFunscript = new FunscriptHandler(axisName.first);
                        otherFunscript->load(fileInfo.absoluteFilePath());
                        funscriptHandlers.append(otherFunscript);
                    }
                }
            }
        }

        if(currentVideo != currentVRPacket.path)
        {
            currentVideo = currentVRPacket.path;
            funscriptHandler->setLoaded(false);
        }

        //LogHandler::Debug("Get deo packet: "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
        currentVRPacket = vrPlayer->getCurrentPacket();
        //LogHandler::Debug("After get deo packet: "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
        //QThread::currentThread()->msleep(1);
    }
    LogHandler::Debug("exit syncDeoFunscript");
}

void syncFunscript(VideoHandler* player, SettingsDialog* xSettings, TCodeHandler* tcodeHandler, FunscriptHandler* funscriptHandler, QList<FunscriptHandler*> funscriptHandlers)
{
    AxisNames axisNames;
    std::shared_ptr<FunscriptAction> actionPosition;
    QMap<QString, std::shared_ptr<FunscriptAction>> otherActions;
    DeviceHandler* device = xSettings->getSelectedDeviceHandler();
    QElapsedTimer mSecTimer;
    qint64 timer1 = 0;
    qint64 timer2 = 0;
    mSecTimer.start();
    while (player->isPlaying())
    {
        if (timer2 - timer1 >= 1)
        {
            timer1 = timer2;
            if(xSettings->isConnected())
            {
                qint64 currentTime = player->position();
                actionPosition = funscriptHandler->getPosition(currentTime);
                foreach(auto funscriptHandlerOther, funscriptHandlers)
                {
                    auto otherAction = funscriptHandlerOther->getPosition(currentTime);
                    if(otherAction != nullptr)
                        otherActions.insert(funscriptHandlerOther->channel(), otherAction);
                }
                QString tcode = tcodeHandler->funscriptToTCode(actionPosition, otherActions);
                if(tcode != nullptr)
                    device->sendTCode(tcode);
                otherActions.clear();
            }
        }
        timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
    }
    LogHandler::Debug("exit syncFunscript");
}

void MainWindow::on_gamepad_sendTCode(QString value)
{
    if(_xSettings->isConnected())
    {
        if(funscriptHandler->isLoaded() && (videoHandler->isPlaying() || _xSettings->getDeoHandler()->isPlaying()))
        {
            QRegularExpression rx("L0[^\\s]*\\s?");
            value = value.remove(rx);
        }
        _xSettings->getSelectedDeviceHandler()->sendTCode(value);
    }
}

void MainWindow::on_gamepad_sendAction(QString action)
{
    mediaAction(action);
}

void MainWindow::on_skipForwardButton_clicked()
{
    skipForward();
}

void MainWindow::on_skipBackButton_clicked()
{
    skipBack();
}

void MainWindow::on_media_statusChanged(MediaStatus status)
{
    switch (status) {
    case EndOfMedia:
        if (!ui->loopToggleButton->isChecked())
            skipForward();
    break;
    case NoMedia:
        //status = tr("No media");
        break;
    case InvalidMedia:
        //status = tr("Invalid meida");
        break;
    case BufferingMedia:
        ui->videoLoadingLabel->show();
        _movie->start();
        break;
    case BufferedMedia:
        ui->videoLoadingLabel->hide();
        _movie->stop();
        break;
    case LoadingMedia:
        ui->videoLoadingLabel->show();
        _movie->start();
        break;
    case LoadedMedia:
        ui->videoLoadingLabel->hide();
        _movie->stop();
        break;
    case StalledMedia:

        break;
    default:
        //status = QString();
        //onStopPlay();
        break;
    }
}

void MainWindow::skipForward()
{
    if (ui->LibraryList->count() > 0)
    {
        ++playingVideoListIndex;
        if(playingVideoListIndex < ui->LibraryList->count())
        {
            ui->LibraryList->setCurrentRow(playingVideoListIndex);
            LibraryListItem selectedFileListItem = getLibraryListItemFromQListItem(ui->LibraryList->selectedItems().first());
            playVideo(selectedFileListItem);
        }
        else
        {
            playingVideoListIndex = ui->LibraryList->count() - 1;
        }
    }
}

void MainWindow::skipBack()
{
    if (ui->LibraryList->count() > 0)
    {
        --playingVideoListIndex;
        if(playingVideoListIndex >= 0)
        {
            ui->LibraryList->setCurrentRow(playingVideoListIndex);
            LibraryListItem selectedFileListItem = getLibraryListItemFromQListItem(ui->LibraryList->selectedItems().first());
            playVideo(selectedFileListItem);
        }
        else
        {
            playingVideoListIndex = 0;
        }
    }
}

void MainWindow::rewind()
{
    qint64 position = videoHandler->position();
    qint64 videoIncrement = SettingsHandler::getVideoIncrement() * 1000;
    if (position > videoIncrement)
        videoHandler->seek(videoHandler->position() - videoIncrement);
    else
        skipBack();
}

void MainWindow::fastForward()
{
    qint64 position = videoHandler->position();
    qint64 videoIncrement = SettingsHandler::getVideoIncrement() * 1000;
    if (position < videoHandler->duration() - videoIncrement)
        videoHandler->seek(videoHandler->position() + videoIncrement);
    else
        skipForward();
}

void MainWindow::media_double_click_event(QMouseEvent * event)
{
    if ( event->button() == Qt::LeftButton )
    {
        MainWindow::toggleFullScreen();
    }
}

void MainWindow::media_single_click_event(QMouseEvent * event)
{
    if (event->button() == Qt::MouseButton::RightButton)
    {
        videoHandler->togglePause();
    }
}

void MainWindow::on_device_connectionChanged(ConnectionChangedSignal event)
{
    deviceConnected = event.status == ConnectionStatus::Connected;
    QString message = "";
    if (event.deviceType == DeviceType::Serial)
    {
        message += "Serial: ";
    }
    else if(event.deviceType == DeviceType::Network)
    {
        message += "Network: ";
    }
    message += " " + event.message;
    connectionStatusLabel->setText(message);
    if(event.status == ConnectionStatus::Error || event.status == ConnectionStatus::Disconnected)
    {
        retryConnectionButton->show();
    }
    else
    {
        retryConnectionButton->hide();
    }
}

void MainWindow::on_gamepad_connectionChanged(ConnectionChangedSignal event)
{
    QString message = "";
    message += "Gamepad: ";
    message += " " + event.message;
    QPixmap bgPixmap;
    if(event.status == ConnectionStatus::Connected)
    {
        bgPixmap.load(("://images/gamepad-icon.png"));
    }
    else if(event.status == ConnectionStatus::Disconnected)
    {

        bgPixmap.load(("://images/gamepad-icon-disconnected.png"));
    }
    else if(event.status == ConnectionStatus::Connecting)
    {
        bgPixmap.load(("://images/gamepad-icon-disconnected.png"));
    }
    QPixmap scaled = bgPixmap.scaled({40, 30}, Qt::AspectRatioMode::KeepAspectRatio);
    gamepadConnectionStatusLabel->setPixmap(scaled);
    gamepadConnectionStatusLabel->setToolTip(message);
}

void MainWindow::on_device_error(QString error)
{
    LogHandler::Dialog(error, XLogLevel::Critical);
}

void MainWindow::on_deo_device_connectionChanged(ConnectionChangedSignal event)
{
    QString message = "";
    message += "DeoVR: ";
    message += " " + event.message;
    deoConnectionStatusLabel->setText(message);
    if(SettingsHandler::getDeoEnabled() && (event.status == ConnectionStatus::Error || event.status == ConnectionStatus::Disconnected))
    {
        ui->actionChange_current_deo_script->setEnabled(false);
        deoRetryConnectionButton->show();
        if(funscriptVRSyncFuture.isRunning())
        {
            funscriptVRSyncFuture.cancel();
            funscriptVRSyncFuture.waitForFinished();
        }
    }
    else if(event.status == ConnectionStatus::Connected)
    {
        ui->actionChange_current_deo_script->setEnabled(true);
        deoRetryConnectionButton->hide();
        if(funscriptVRSyncFuture.isRunning())
        {
            funscriptVRSyncFuture.cancel();
            funscriptVRSyncFuture.waitForFinished();
        }
        funscriptVRSyncFuture = QtConcurrent::run(syncVRFunscript, _xSettings->getDeoHandler(), videoHandler, _xSettings, tcodeHandler, funscriptHandler);

    }
    else if(event.status == ConnectionStatus::Connecting)
    {
        deoConnectionStatusLabel->show();
    }
    else
    {
        ui->actionChange_current_deo_script->setEnabled(false);
        deoConnectionStatusLabel->hide();
        deoRetryConnectionButton->hide();
    }
}

void MainWindow::on_deo_device_error(QString error)
{
    LogHandler::Dialog("Deo error: "+error, XLogLevel::Critical);
}

void MainWindow::on_whirligig_device_connectionChanged(ConnectionChangedSignal event)
{
    QString message = "";
    message += "Whirligig: ";
    message += " " + event.message;
    whirligigConnectionStatusLabel->setText(message);
    if(SettingsHandler::getWhirligigEnabled() && (event.status == ConnectionStatus::Error || event.status == ConnectionStatus::Disconnected))
    {
        ui->actionChange_current_deo_script->setEnabled(false);
        whirligigRetryConnectionButton->show();
        if(funscriptVRSyncFuture.isRunning())
        {
            funscriptVRSyncFuture.cancel();
            funscriptVRSyncFuture.waitForFinished();
        }
    }
    else if(event.status == ConnectionStatus::Connected)
    {
        ui->actionChange_current_deo_script->setEnabled(true);
        whirligigRetryConnectionButton->hide();
        if(funscriptVRSyncFuture.isRunning())
        {
            funscriptVRSyncFuture.cancel();
            funscriptVRSyncFuture.waitForFinished();
        }
        funscriptVRSyncFuture = QtConcurrent::run(syncVRFunscript, _xSettings->getWhirligigHandler(), videoHandler, _xSettings, tcodeHandler, funscriptHandler);

    }
    else if(event.status == ConnectionStatus::Connecting)
    {
        whirligigConnectionStatusLabel->show();
    }
    else
    {
        ui->actionChange_current_deo_script->setEnabled(false);
        whirligigConnectionStatusLabel->hide();
        whirligigRetryConnectionButton->hide();
    }
}

void MainWindow::on_whirligig_device_error(QString error)
{
    LogHandler::Dialog("Whirligig error: "+error, XLogLevel::Critical);
}


void MainWindow::on_actionAbout_triggered()
{
    QDialog aboutWindow;
    QGridLayout layout;
    QRect windowRect;
    windowRect.setSize({300, 200});
    layout.setGeometry(windowRect);
    QLabel copyright;
    copyright.setText("<b>XTPlayer v"+SettingsHandler::XTPVersion + "</b><br>"
                       + SettingsHandler::TCodeVersion + "<br>"
                                                "Copyright 2020 Jason C. Fain<br>"
                                                "Donate: <a href='https://www.patreon.com/Khrull'>https://www.patreon.com/Khrull</a><br>"
                                                "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND.");
    copyright.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&copyright);
    QLabel sources;
    sources.setText("This software uses libraries from:");
    sources.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&sources);
    QLabel qtInfo;
    qtInfo.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    qtInfo.setText("<b>Qt v5.15.0</b><br>"
                   "Distributed under the terms of LGPLv3 or later.<br>"
                   "Source: <a href='https://github.com/qt/qt5/releases/tag/v5.15.0'>https://github.com/qt/qt5/releases/tag/v5.15.0</a>");
    qtInfo.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&qtInfo);
    QLabel qtAVInfo;
    qtAVInfo.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    qtAVInfo.setText("<b>QtAV 1.12.0(Aug 17 2020, 22:01:37)</b><br>"
                     "Multimedia framework base on Qt and FFmpeg.<br>"
                     "Distributed under the terms of LGPLv2.1 or later.<br>"
                     "Shanghai University->S3 Graphics->Deepin->PPTV<br>"
                     "Shanghai, China<br>"
                     "Copyright (C) 2012-2016 Wang Bin (aka. Lucas Wang)<br>"
                     "Email: <a href='mailto:wbsecg1@gmail.com'>wbsecg1@gmail.com</a><br>"
                     "Donate: <a href='http://qtav.org/donate.html'>http://qtav.org/donate.html</a><br>"
                     "Source: <a href='https://github.com/wang-bin/QtAV'>https://github.com/wang-bin/QtAV</a><br>"
                     "Home page: <a href='http://qtav.org'>http://qtav.org</a>");
    qtAVInfo.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&qtAVInfo);
    QLabel libAVInfo;
    libAVInfo.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    libAVInfo.setText("<b>Libav 12.3</b><br>"
                      "the Libav project under the LGPLv2.1<br>"
                      "<a href='https://libav.org/download/'>https://libav.org/download/</a>");
    libAVInfo.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&libAVInfo);
    QLabel rangeSliderInfo;
    rangeSliderInfo.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    rangeSliderInfo.setText("<b>Qt-RangeSlider (MIT)</b><br>"
                      "Copyright (c) 2019 ThisIsClark<br>"
                      "Modifications Copyright (c) 2020 Jason C. Fain<br>"
                      "<a href='https://github.com/ThisIsClark/Qt-RangeSlider'>https://github.com/ThisIsClark/Qt-RangeSlider</a>");
    rangeSliderInfo.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&rangeSliderInfo);
    QLabel boolinqInfo;
    boolinqInfo.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    boolinqInfo.setText("<b>boolinq</b><br>"
                      "Copyright (C) 2019 by Anton Bukov (MIT)<br>"
                      "<a href='https://github.com/k06a/boolinq'>https://github.com/k06a/boolinq</a>");
    boolinqInfo.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&boolinqInfo);
    aboutWindow.setLayout(&layout);
    aboutWindow.exec();
}
//<a href="https://www.vecteezy.com/free-vector/media-player-icons">Media Player Icons Vectors by Vecteezy</a>

void MainWindow::on_actionDonate_triggered()
{
    QDesktopServices::openUrl(QUrl(QString::fromLatin1("https://www.patreon.com/Khrull")));
}

void MainWindow::on_actionSettings_triggered()
{
    _xSettings->initLive();
    _xSettings->exec();
}

void MainWindow::on_actionThumbnail_triggered()
{
    SettingsHandler::setLibraryView(LibraryView::Thumb);
    ui->LibraryList->setResizeMode(QListView::Adjust);
    ui->LibraryList->setFlow(QListView::LeftToRight);
    ui->LibraryList->setViewMode(QListView::IconMode);
    ui->LibraryList->setTextElideMode(Qt::ElideMiddle);
    ui->LibraryList->setSpacing(2);
    updateThumbSizeUI(SettingsHandler::getThumbSize());
}

void MainWindow::on_actionList_triggered()
{
    SettingsHandler::setLibraryView(LibraryView::List);
    ui->LibraryList->setResizeMode(QListView::Fixed);
    ui->LibraryList->setFlow(QListView::TopToBottom);
    ui->LibraryList->setViewMode(QListView::ListMode);
    ui->LibraryList->setTextElideMode(Qt::ElideRight);
    ui->LibraryList->setSpacing(1);
    updateThumbSizeUI(SettingsHandler::getThumbSizeList());
}

void MainWindow::updateThumbSizeUI(int size)
{
    switch(size)
    {
        case 75:
            action75_Size->setChecked(true);
            on_action75_triggered();
        break;
        case 100:
            action100_Size->setChecked(true);
            on_action100_triggered();
        break;
        case 125:
            action125_Size->setChecked(true);
            on_action125_triggered();
        break;
        case 150:
            action150_Size->setChecked(true);
            on_action150_triggered();
        break;
        case 175:
            action175_Size->setChecked(true);
            on_action175_triggered();
        break;
    }
}

void MainWindow::on_action75_triggered()
{
    setThumbSize(75);
    ui->LibraryList->setIconSize(_currentThumbSize);
}

void MainWindow::on_action100_triggered()
{
    setThumbSize(100);
    ui->LibraryList->setIconSize(_currentThumbSize);
}

void MainWindow::on_action125_triggered()
{
    setThumbSize(125);
    ui->LibraryList->setIconSize(_currentThumbSize);
}

void MainWindow::on_action150_triggered()
{
    setThumbSize(150);
    ui->LibraryList->setIconSize(_currentThumbSize);
}

void MainWindow::on_action175_triggered()
{
    setThumbSize(175);
    ui->LibraryList->setIconSize(_currentThumbSize);
}

void MainWindow::setThumbSize(int size)
{
    _currentThumbSize = {size, size};
    if (SettingsHandler::getLibraryView() == LibraryView::List)
    {
        SettingsHandler::setThumbSizeList(size);
    }
    else
    {
        SettingsHandler::setThumbSize(size);
    }
}

LibraryListItem MainWindow::getLibraryListItemFromQListItem(QListWidgetItem* qListWidgetItem)
{
    return qListWidgetItem->data(Qt::UserRole).value<LibraryListItem>();
}


void MainWindow::on_actionChange_theme_triggered()
{
    QFileInfo selectedThemeInfo(SettingsHandler::getSelectedTheme());
    QString selectedTheme = QFileDialog::getOpenFileName(this, "Choose XTP theme", selectedThemeInfo.absoluteDir().absolutePath(), "CSS Files (*.css)");
    if(!selectedTheme.isEmpty())
    {
        SettingsHandler::setSelectedTheme(selectedTheme);
        QFile file(selectedTheme);
        file.open(QFile::ReadOnly);
        QString styleSheet = QLatin1String(file.readAll());
        setStyleSheet(styleSheet);
    }
}

void MainWindow::on_actionChange_current_deo_script_triggered()
{
    changeDeoFunscript();
}

void MainWindow::on_settingsButton_clicked()
{
    on_actionSettings_triggered();
}

void MainWindow::on_loopToggleButton_toggled(bool checked)
{
    if (checked)
    {
        ui->SeekSlider->setOption(RangeSlider::Option::DoubleHandles);
        ui->SeekSlider->SetRange(0, 100);
        connect(ui->SeekSlider, QOverload<int>::of(&RangeSlider::lowerValueChanged), this, &MainWindow::onLoopRange_valueChanged);
        connect(ui->SeekSlider, QOverload<int>::of(&RangeSlider::upperValueChanged), this, &MainWindow::onLoopRange_valueChanged);
        videoHandler->setRepeat(-1);
    }
    else
    {
        ui->SeekSlider->setOption(RangeSlider::Option::RightHandle);
        on_media_positionChanged(videoHandler->position());
        ui->SeekSlider->updateColor();
        disconnect(ui->SeekSlider, QOverload<int>::of(&RangeSlider::lowerValueChanged), this, &MainWindow::onLoopRange_valueChanged);
        disconnect(ui->SeekSlider, QOverload<int>::of(&RangeSlider::upperValueChanged), this, &MainWindow::onLoopRange_valueChanged);
        qint64 position = videoHandler->position();
        videoHandler->setRepeat();
        videoHandler->setPosition(position);
    }
}

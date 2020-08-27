#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setStyleSheet("QMainWindow, QDialog {"
                  "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1.5, stop: 0 black, stop: 1 darkred); "
                  "color: white; "
                  "}"
                  "QLabel {"
                  "background-color: transparent; "
                  "color: white; "
                  "}"
                  "QListWidget {"
                  "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1.5, stop: 0 black, stop: 1 darkred); "
                  "color: white; "
                  "}"
                  "QMenuBar {"
                  "background-color: black; "
                  "color: white; "
                  "}"
                  "QMenu {"
                  "background-color: black; "
                  "color: white; "
                  "}"
                  "QMenu:hover:!pressed {"
                  "background-color: darkred; "
                  "color: white; "
                  "}"
                  "QAction:hover:!pressed {"
                  "background-color: darkred; "
                  "color: white; "
                  "}"
                  "QRadioButton, QCheckBox, QGroupBox {"
                  "background-color: transparent; "
                  "color: white; "
                  "}"
                  "QScrollBar:handle:vertical {"
                  "background-color: #4f0000; "
                  "}"
                  "QScrollBar:horizontal {"
                  "background-color: black; "
                  "}"
                  "QScrollBar:handle:horizontal {"
                  "background-color: #3e0000; "
                  "}"
                  "QScrollBar:vertical {"
                  "background-color: black; "
                  "}"
                  "QPushButton { "
                    "background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 black, stop: 1 grey); "
                    "color: red; "
                    "}; ");
//                   "QPushButton:pressed { "
//                      "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 red, stop: 1 black); "
//                      "border-style: inset; "
//                    "}; ");

//    QFile file(":/qss/default.qss");
//    file.open(QFile::ReadOnly);
//    QString styleSheet = QLatin1String(file.readAll());

    SettingsHandler::Load();

    videoHandler = new VideoHandler(this);
    ui->MediaGrid->addWidget(videoHandler);

    _xSettings = new SettingsDialog(this);
    _xSettings->init(videoHandler);

    ui->SeekSlider->setDisabled(true);
    ui->SeekSlider->SetRange(0, 100);
    ui->SeekSlider->setOption(RangeSlider::Option::RightHandle);
    ui->SeekSlider->setUpperValue(0);
    ui->SeekSlider->setBackGroundEnabledColor(QColorConstants::Red);

    on_load_library(SettingsHandler::getSelectedLibrary());

    ui->VolumeSlider->setDisabled(false);
    ui->VolumeSlider->SetRange(0, 30);
    ui->VolumeSlider->setOption(RangeSlider::Option::RightHandle);
    ui->VolumeSlider->setUpperValue(SettingsHandler::getPlayerVolume());
    ui->VolumeSlider->setBackGroundEnabledColor(QColorConstants::Red);
    setVolumeIcon(SettingsHandler::getPlayerVolume());


    connect(videoHandler, &VideoHandler::positionChanged, this, &MainWindow::on_media_positionChanged);
    connect(videoHandler, &VideoHandler::mediaStatusChanged, this, &MainWindow::on_media_statusChanged);
    connect(videoHandler, &VideoHandler::started, this, &MainWindow::on_media_start);
    connect(videoHandler, &VideoHandler::stopped, this, &MainWindow::on_media_stop);

    connect(ui->SeekSlider, &RangeSlider::upperValueMove, this, &MainWindow::on_seekSlider_sliderMoved);
    connect(ui->VolumeSlider, &RangeSlider::upperValueMove, this, &MainWindow::on_VolumeSlider_valueChanged);
    //connect(player, static_cast<void(AVPlayer::*)(AVPlayer::Error )>(&AVPlayer::error), this, &MainWindow::on_media_error);

    connect(videoHandler, &VideoHandler::doubleClicked, this, &MainWindow::media_double_click_event);
    //connect(vw, &XVideoWidget::singleClicked, this, &MainWindow::media_single_click_event);
    connect(this, &MainWindow::keyPressed, this, &MainWindow::on_key_press);
    //connect(videoHandler, &VideoHandler::mouseEnter, this, &MainWindow::on_video_mouse_enter);

    ui->LibraryList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->LibraryList, &QListWidget::customContextMenuRequested, this, &MainWindow::onLibraryList_ContextMenuRequested);

    connect(_xSettings, &SettingsDialog::deviceConnectionChange, this, &MainWindow::on_device_connectionChanged);
    connect(_xSettings, &SettingsDialog::deviceError, this, &MainWindow::on_device_error);

}

MainWindow::~MainWindow()
{
    SettingsHandler::Save();
    delete ui;
    delete videoHandler;
    delete _xSettings;
    if(funscriptFuture.isRunning())
    {
        funscriptFuture.cancel();
        funscriptFuture.waitForFinished();
    }
}

void MainWindow::on_key_press(QKeyEvent * event)
{
    switch(event->key())
    {
        case Qt::Key_Space:
        case Qt::Key_MediaPause:
            MainWindow::togglePause();
            break;
        case Qt::Key_F11:
        case Qt::Key_Escape:
            MainWindow::toggleFullScreen();
            break;
        case Qt::Key_M:
            MainWindow::on_MuteBtn_toggled(!videoHandler->isMute());
            break;
        case Qt::Key_MediaStop:
        case Qt::Key_MediaTogglePlayPause:
            MainWindow::on_media_stop();
            break;
        case Qt::Key_MediaNext:
        case Qt::Key_Right:
            MainWindow::skipForward();
            break;
        case Qt::Key_MediaPrevious:
        case Qt::Key_Left:
            MainWindow::skipBack();
            break;
    }
}

void MainWindow::on_video_mouse_enter(QEvent * event)
{
    if (isFullScreen())
    {
        toggleControls();
        QTimer::singleShot(2000, this, &MainWindow::toggleControls);
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

    // Show context menu at handling position
    myMenu.exec(globalPos);
}

void MainWindow::on_load_library(QString path)
{
    if (!path.isNull() && !path.isEmpty())
    {
        QDir directory(path);
        if (directory.exists())
        {
            ui->LibraryList->clear();
            videos.clear();
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
                QString scriptFile = fileNameTemp.remove(fileNameTemp.lastIndexOf('.'), fileNameTemp.length() -  1) + ".funscript";
                QString scriptPath;
                if (SettingsHandler::getSelectedFunscriptLibrary() == Q_NULLPTR)
                {
                    scriptPath = videoPathTemp.remove(videoPathTemp.lastIndexOf('.'), videoPathTemp.length() -  1) + ".funscript";
                }
                else
                {
                    scriptPath = SettingsHandler::getSelectedFunscriptLibrary() + QDir::separator() + scriptFile;
                }
                if (!funscriptHandler->exists(scriptPath))
                {
                    scriptPath = nullptr;
                }
                LibraryListItem item
                {
                    videoPath, // path
                    fileName, // name
                    scriptPath // script
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
                qListWidgetItem->setText(fileinfo.fileName());
                qListWidgetItem->setData(Qt::UserRole, listItem);
                ui->LibraryList->addItem(qListWidgetItem);
                videos.push_back(videoPath);
            }
        }
        else
        {
           LogHandler::Dialog("Library path '" + path + "' does not exist", XLogLevel::Critical);
           on_actionSelect_library_triggered();
        }
    }
    else
    {
        on_actionSelect_library_triggered();
    }
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
    selectedFileListItem = item->data(Qt::UserRole).value<LibraryListItem>();
}

void MainWindow::on_LibraryList_itemDoubleClicked(QListWidgetItem *item)
{
    selectedFileListItem = item->data(Qt::UserRole).value<LibraryListItem>();
    playFile(selectedFileListItem);
}
void MainWindow::playFileFromContextMenu()
{
    LibraryListItem selectedFileListItem = ui->LibraryList->selectedItems().first()->data(Qt::UserRole).value<LibraryListItem>();
    playFile(selectedFileListItem);
}

void MainWindow::playFileWithCustomScript()
{
    QString selectedScript = QFileDialog::getOpenFileName(this, tr("Choose script"), SettingsHandler::getSelectedLibrary(), tr("Script Files (*.funscript)"));
    if (selectedScript != Q_NULLPTR)
    {
        LibraryListItem selectedFileListItem = ui->LibraryList->selectedItems().first()->data(Qt::UserRole).value<LibraryListItem>();
        playFile(selectedFileListItem, selectedScript);
    }
}

void MainWindow::playFile(LibraryListItem selectedFileListItem, QString customScript)
{
    videoHandler->stop();
    QFile file(selectedFileListItem.path);
    if (file.exists())
    {
        videoHandler->setFile(selectedFileListItem.path);
        videoHandler->load();
        QString scriptFile = customScript == nullptr ? selectedFileListItem.script : customScript;
        bool funscriptLoaded = funscriptHandler->load(scriptFile);
        SettingsHandler::setSelectedFile(selectedFileListItem.path);
        //QUrl url = QUrl::fromLocalFile(selectedFileListItem.path);
        videoHandler->play();
        selectedFileListIndex = ui->LibraryList->currentRow();
        if(!funscriptLoaded)
        {
            LogHandler::Dialog("Error loading script " + customScript + "!", XLogLevel::Warning);

        }
    }
    else {
        LogHandler::Dialog("File '" + selectedFileListItem.path + "' does not exist!", XLogLevel::Critical);
    }
}


void MainWindow::togglePause()
{
    videoHandler->togglePause();
}

void MainWindow::toggleFullScreen()
{
    if(videoHandler->isPlaying()) {
        //toggleUI();
        QScreen *screen = QGuiApplication::primaryScreen();
        QSize screenSize = screen->size();
        if(!QMainWindow::isFullScreen()) {
            videoSize = videoHandler->size();
            appSize = size();
            appPos = pos();
            setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
            videoHandler->move(QPoint(0, 0));
            ui->MainFrame->layout()->removeWidget(videoHandler);
            ui->menubar->hide();
            QMainWindow::layout()->addWidget(videoHandler);
            QMainWindow::showFullScreen();
            videoHandler->layout()->setMargin(0);
            videoHandler->move(QPoint(0, 0));
            //videoHandler->resize(QSize(screenSize.width()+1, screenSize.height()+1));
            videoHandler->resize(screenSize);
        }
        else
        {
            setWindowFlags(Qt::Window);
            QMainWindow::showNormal();
            QMainWindow::resize(appSize);
            QMainWindow::move(appPos);
            QMainWindow::layout()->removeWidget(videoHandler);
            ui->MediaGrid->addWidget(videoHandler);
            videoHandler->layout()->setMargin(9);
        }
    }
}

void MainWindow::toggleControls()
{
    if(!ui->playerControlsFrame->isHidden())
    {
        ui->playerControlsFrame->hide();
    }
    else
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
    if(ui->LibraryList->selectedItems().length() > 0 && selectedFileListItem.path != videoHandler->file() || ui->LibraryList->selectedItems().length() > 0 && !videoHandler->isPlaying())
    {
        MainWindow::playFile(selectedFileListItem);
    }
    else if(ui->LibraryList->selectedItems().length() == 0)
    {
        selectedFileListIndex = 0;
        ui->LibraryList->setCurrentRow(0);
        selectedFileListItem = ui->LibraryList->selectedItems()[0]->data(Qt::UserRole).value<LibraryListItem>();
        MainWindow::playFile(selectedFileListItem);
    }
}

void MainWindow::on_PauseBtn_clicked()
{
    MainWindow::togglePause();
}

void MainWindow::on_StopBtn_clicked()
{
    if(videoHandler->isPlaying())
    {
        if(videoHandler->isPaused())
        {
            ui->PauseBtn->setChecked(false);
        }
        videoHandler->stop();
    }
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

void MainWindow::on_seekSlider_sliderMoved(int position)
{
    qint64 playerPosition = XMath::mapRange(static_cast<qint64>(position), 0, 100, 0, videoHandler->duration());
    ui->SeekSlider->setToolTip(QTime(0, 0, 0).addMSecs(position).toString(QString::fromLatin1("HH:mm:ss")));
    videoHandler->setPosition(playerPosition);
}

void MainWindow::on_media_positionChanged(qint64 position)
{
    ui->lblCurrentDuration->setText( second_to_minutes(position / 1000).append("/").append( second_to_minutes( (videoHandler->duration())/1000 ) ) );

    //ui->lblCurrentDuration->setText(QTime(0, 0, 0).addMSecs(position).toString(QString::fromLatin1("HH:mm:ss")));
    qint64 duration = videoHandler->duration();
    if (duration > 0)
    {
        qint64 sliderPosition = XMath::mapRange(position, 0, duration, 0, 100);
        ui->SeekSlider->setUpperValue(static_cast<int>(sliderPosition));
    }
}

void MainWindow::on_media_start()
{
    ui->SeekSlider->setDisabled(false);
    ui->StopBtn->setDisabled(false);
    ui->PauseBtn->setDisabled(false);
    ui->fullScreenBtn->setDisabled(false);
    if(funscriptFuture.isRunning())
    {
        funscriptFuture.cancel();
        funscriptFuture.waitForFinished();
    }
    funscriptFuture = QtConcurrent::run(syncFunscript, videoHandler, _xSettings, tcodeHandler, funscriptHandler);
}

void syncFunscript(VideoHandler* player, SettingsDialog* xSettings, TCodeHandler* tcodeHandler, FunscriptHandler* funscriptHandler)
{
    std::unique_ptr<FunscriptAction> actionPosition;
    DeviceHandler* device = xSettings->getSelectedDeviceHandler();
    while (player->isPlaying())
    {
        if(xSettings->isConnected()) {
            qint64 position = player->position();
            actionPosition = funscriptHandler->getPosition(position);
            if (actionPosition != nullptr)
            {
                device->sendTCode(tcodeHandler->funscriptToTCode(actionPosition->pos, actionPosition->speed));
            }
            actionPosition.reset();
        }
        Sleep(1);
    }
    //serialHandler->sendTCode(tcodeHandler->funscriptToTCode(actionPosition->pos));
    LogHandler::Debug("exit syncFunscript");
}

void MainWindow::on_media_stop()
{
    ui->SeekSlider->setUpperValue(0);
    qDebug(">>>>>>>>>>>>>>disable slider");
    ui->SeekSlider->setDisabled(true);
    ui->StopBtn->setDisabled(true);
    ui->PauseBtn->setDisabled(true);
    ui->fullScreenBtn->setDisabled(true);
    ui->lblCurrentDuration->setText("00:00:00");

    if(funscriptFuture.isRunning())
    {
        funscriptFuture.cancel();
    }
    if (_xSettings->getSelectedDeviceHandler()->isRunning())
    {
        _xSettings->getSelectedDeviceHandler()->dispose();
    }

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
        skipForward();
    break;
    case NoMedia:
        //status = tr("No media");
        break;
    case InvalidMedia:
        //status = tr("Invalid meida");
        break;
    case BufferingMedia:
        //status = tr("Buffering...");
        break;
    case BufferedMedia:
        //status = tr("Buffered");
        break;
    case LoadingMedia:
        //status = tr("Loading...");
        break;
    case LoadedMedia:
        //status = tr("Loaded");
        break;
    case StalledMedia:
        //status = tr("Stalled");
        break;
    default:
        //status = QString();
        //onStopPlay();
        break;
    }
}

void MainWindow::skipForward()
{
    ++selectedFileListIndex;
    if(selectedFileListIndex < videos.length())
    {
        ui->LibraryList->setCurrentRow(selectedFileListIndex);
        selectedFileListItem = ui->LibraryList->selectedItems()[0]->data(Qt::UserRole).value<LibraryListItem>();
        on_PlayBtn_clicked();
    }
}

void MainWindow::skipBack()
{
    --selectedFileListIndex;
    if(selectedFileListIndex >= 0)
    {
        ui->LibraryList->setCurrentRow(selectedFileListIndex);
        selectedFileListItem = ui->LibraryList->selectedItems()[0]->data(Qt::UserRole).value<LibraryListItem>();
        on_PlayBtn_clicked();
    }
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
    if ( event->button() == Qt::LeftButton )
    {
        MainWindow::togglePause();
    }
}

void MainWindow::on_device_connectionChanged(ConnectionChangedSignal event)
{
    deviceConnected = event.status == ConnectionStatus::Connected;
}

void MainWindow::on_device_error(QString error)
{
    LogHandler::Dialog(error, XLogLevel::Critical);
}

QString MainWindow::second_to_minutes(int seconds)
{
    int sec = seconds;
    QString mn = QString::number( (sec ) / 60);
    int _tmp_mn  = mn.toInt() * 60;
    QString sc= QString::number( (seconds - _tmp_mn  ) % 60 );

    return (mn.length() == 1 ? "0" + mn : mn ) + ":" + (sc.length() == 1 ? "0" + sc : sc);
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
                                                                    "Copyright 2020 Jason C. Fain<br>"
                                                                    "Donate: <a href='https://www.patreon.com/Khrull'>https://www.patreon.com/Khrull</a>");
    copyright.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&copyright);
    QLabel sources;
    sources.setText("This software uses libraries from:");
    sources.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&sources);
    QLabel qtInfo;
    qtInfo.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    qtInfo.setText("<b>Qt 1.15.0<br>"
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
                     "Home page: <a href='http://qtav.org'>href='http://qtav.org</a>");
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
    rangeSliderInfo.setText("<b>Qt-RangeSlider</b><br>"
                      "Copyright (c) 2019 ThisIsClark<br>"
                      "<a href='https://github.com/ThisIsClark/Qt-RangeSlider'>https://github.com/ThisIsClark/Qt-RangeSlider</a>");
    rangeSliderInfo.setAlignment(Qt::AlignHCenter);
    layout.addWidget(&rangeSliderInfo);
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
    _xSettings->setupUi();
    _xSettings->exec();
}

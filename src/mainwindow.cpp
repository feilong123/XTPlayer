#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QStringList arguments, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , xtEngine(this)
{
    QCoreApplication::setOrganizationName("cUrbSide prOd");
    QCoreApplication::setApplicationName("XTPlayer");
    const QString fullVersion = "XTP: v"+ XTPSettings::XTPVersionTimeStamp + "\nXTE: v" + SettingsHandler::XTEVersionTimeStamp;

    QPixmap pixmap("://images/XTP_Splash.png");
    loadingSplash = new QSplashScreen(pixmap);
    loadingSplash->setStyleSheet("color: white");
    loadingSplash->show();
    loadingSplash->raise();

    ui->setupUi(this);
    loadingSplash->showMessage(fullVersion + "\nLoading Settings...", Qt::AlignBottom, Qt::white);

    _xSettings = new SettingsDialog(this);
    if(_xSettings->HasLaunchPass()) {
        int tries = 1;
        while(_isPasswordIncorrect != PasswordResponse::CANCEL && _isPasswordIncorrect == PasswordResponse::INCORRECT)
        {
            _isPasswordIncorrect = _xSettings->GetLaunchPass();
            if(_isPasswordIncorrect == PasswordResponse::CANCEL)
            {
                QTimer::singleShot(0, this, SLOT(onPasswordIncorrect()));
                return;
            }
            else if(_isPasswordIncorrect == PasswordResponse::INCORRECT)
            {
                switch(tries) {
                    case 1:
                        DialogHandler::MessageBox(this, "Wrong!", XLogLevel::Critical);
                    break;
                    case 2:
                        DialogHandler::MessageBox(this, "Nope!", XLogLevel::Critical);
                    break;
                    case 3:
                        DialogHandler::MessageBox(this, "K thx byyye!", XLogLevel::Critical);
                    break;
                }

                if( tries >= 3)
                {
                    QTimer::singleShot(0, this, SLOT(onPasswordIncorrect()));
                    return;
                }
            }
            tries++;
        }
    }

    if(arguments.length() > 0)
    {
        foreach(QString arg, arguments)
        {
            if(arg.toLower().startsWith("-verbose")) {
                LogHandler::Debug("Starting in verbose mode");
                LogHandler::setUserDebug(true);
                LogHandler::setQtDebuging(true);
            } else if(arg.toLower().startsWith("-debug")) {
                LogHandler::Debug("Starting in debug mode");
                LogHandler::setUserDebug(true);
            } else if(arg.toLower().startsWith("-reset")) {
                LogHandler::Debug("Resettings settings to default!");
                SettingsHandler::Default();
            } else if(arg.toLower().startsWith("-resetwindow")) {
                LogHandler::Debug("Resettings window size to default!");
                XTPSettings::resetWindowSize();
            }
        }
    }
    XTPSettings::load();

    _dlnaScriptLinksDialog = new DLNAScriptLinks(this);

    loadingSplash->showMessage(fullVersion + "\nLoading UI...", Qt::AlignBottom, Qt::white);

    textToSpeech = new QTextToSpeech(this);
    auto availableVoices = textToSpeech->availableVoices();

    const QVoice voice = boolinq::from(availableVoices).firstOrDefault([](const QVoice &x) { return x.gender() == QVoice::Female; });
    textToSpeech->setVoice(voice);


    deoConnectionStatusLabel = new QLabel(this);
    deoRetryConnectionButton = new QPushButton(this);
    deoRetryConnectionButton->hide();
    deoRetryConnectionButton->setProperty("cssClass", "retryButton");
    deoRetryConnectionButton->setText("HereSphere Retry");
    ui->statusbar->addPermanentWidget(deoConnectionStatusLabel);
    ui->statusbar->addPermanentWidget(deoRetryConnectionButton);

    vrConnectionStatusLabel = new QLabel(this);
    vrRetryConnectionButton = new QPushButton(this);
    vrRetryConnectionButton->hide();
    vrRetryConnectionButton->setProperty("cssClass", "retryButton");
    vrRetryConnectionButton->setText("Whirligig Retry");
    ui->statusbar->addPermanentWidget(vrConnectionStatusLabel);
    ui->statusbar->addPermanentWidget(vrRetryConnectionButton);

    xtpWebStatusLabel = new QLabel(this);
    ui->statusbar->addPermanentWidget(xtpWebStatusLabel);

    connectionStatusLabel = new QLabel(this);
    connectionStatusLabel->setObjectName("connectionStatusLabel");
    retryConnectionButton = new QPushButton(this);
    retryConnectionButton->hide();
    retryConnectionButton->setProperty("cssClass", "retryButton");
    retryConnectionButton->setText("TCode Retry");
    ui->statusbar->addPermanentWidget(connectionStatusLabel);
    ui->statusbar->addPermanentWidget(retryConnectionButton);

    gamepadConnectionStatusLabel = new QLabel(this);
    ui->statusbar->addPermanentWidget(gamepadConnectionStatusLabel);

    settingsButton = new QPushButton(this);
    settingsButton->setObjectName(QString::fromUtf8("settingsButton"));
    settingsButton->setProperty("cssClass", "settingsButton");
    settingsButton->setMinimumSize(QSize(0, 20));
    QIcon icon5;
    icon5.addFile(QString::fromUtf8("://images/icons/settings-black.png"), QSize(), QIcon::Normal, QIcon::Off);
    settingsButton->setIcon(icon5);
    settingsButton->setIconSize(QSize(20, 20));
    settingsButton->setFlat(true);
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::on_actionSettings_triggered);
    ui->statusbar->addPermanentWidget(settingsButton);

    _mediaFrame = new QFrame(this);
    _mediaGrid = new QGridLayout(_mediaFrame);
    _mediaFrame->setLayout(_mediaGrid);
    _mediaFrame->setContentsMargins(0,0,0,0);

    videoHandler = new VideoHandler(this);
    _mediaGrid->addWidget(videoHandler, 0, 0, 3, 5);
    _mediaGrid->setMargin(0);
    _mediaGrid->setContentsMargins(0,0,0,0);


    _controlsHomePlaceHolderFrame = new QFrame(this);
    _controlsHomePlaceHolderGrid = new QGridLayout(_controlsHomePlaceHolderFrame);
    _controlsHomePlaceHolderFrame->setLayout(_controlsHomePlaceHolderGrid);

    _playerControlsFrame = new PlayerControls(this);
    _controlsHomePlaceHolderGrid->addWidget(_playerControlsFrame, 0, 0);

    ui->mediaAndControlsGrid->addWidget(_mediaFrame, 0, 0, 19, 3);
    ui->mediaAndControlsGrid->addWidget(_controlsHomePlaceHolderFrame, 20, 0, 1, 3);

    _playerControlsFrame->setVolume(SettingsHandler::getPlayerVolume());

    libraryList = new XLibraryList(this);

    _playListViewModel = new PlaylistViewModel(xtEngine.mediaLibraryHandler(), this);

    _librarySortFilterProxyModel = new LibrarySortFilterProxyModel(xtEngine.mediaLibraryHandler(), this);
    auto libraryListViewModel = new LibraryListViewModel(xtEngine.mediaLibraryHandler(), this);
    _librarySortFilterProxyModel->setSourceModel(libraryListViewModel);
    libraryList->setModel(_librarySortFilterProxyModel);


    QScroller::grabGesture(libraryList->viewport(), QScroller::LeftMouseButtonGesture);
    auto scroller = QScroller::scroller(libraryList->viewport());
    QScrollerProperties scrollerProperties;
    QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
    scrollerProperties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, overshootPolicy);
    QScroller::scroller(libraryList)->setScrollerProperties(scrollerProperties);
    scroller->setScrollerProperties(scrollerProperties);
    ui->libraryGrid->addWidget(libraryList, 1, 0, 20, 12);

    ui->libraryGrid->setSpacing(5);
    ui->libraryGrid->setColumnMinimumWidth(0, 0);

    backLibraryButton = new QPushButton(this);
    backLibraryButton->setProperty("id", "backLibraryButton");
    QIcon backIcon("://images/icons/back.svg");
    backLibraryButton->setIcon(backIcon);
    ui->libraryGrid->addWidget(backLibraryButton, 0, 0);
    backLibraryButton->hide();

    windowedLibraryButton = new QPushButton(this);
    windowedLibraryButton->setProperty("id", "windowedLibraryButton");
    QIcon windowedIcon("://images/icons/windowed.svg");
    windowedLibraryButton->setIcon(windowedIcon);
    ui->libraryGrid->addWidget(windowedLibraryButton, 0, ui->libraryGrid->columnCount() - 1);

    libraryWindow = new LibraryWindow(this);
    libraryWindow->setProperty("id", "libraryWindow");
    libraryWindow->hide();

    randomizeLibraryButton = new QPushButton(this);
    randomizeLibraryButton->setProperty("id", "randomizeLibraryButton");
    QIcon reloadIcon("://images/icons/reload.svg");
    randomizeLibraryButton->setIcon(reloadIcon);
    ui->libraryGrid->addWidget(randomizeLibraryButton, 0, 1);
    randomizeLibraryButton->hide();

    editPlaylistButton = new QPushButton(this);
    editPlaylistButton->setProperty("id", "editPlaylistButton");
    QIcon editIcon("://images/icons/edit.svg");
    editPlaylistButton->setIcon(editIcon);
    ui->libraryGrid->addWidget(editPlaylistButton, 0, ui->libraryGrid->columnCount() - 2);
    editPlaylistButton->hide();

    savePlaylistButton = new QPushButton(this);
    savePlaylistButton->setProperty("id", "savePlaylistButton");
    QIcon saveIcon("://images/icons/save.svg");
    savePlaylistButton->setIcon(saveIcon);
    ui->libraryGrid->addWidget(savePlaylistButton, 0, ui->libraryGrid->columnCount() - 3);
    savePlaylistButton->hide();

    cancelEditPlaylistButton = new QPushButton(this);
    cancelEditPlaylistButton->setProperty("id", "cancelEditPlaylistButton");
    QIcon xIcon("://images/icons/x.svg");
    cancelEditPlaylistButton->setIcon(xIcon);
    ui->libraryGrid->addWidget(cancelEditPlaylistButton, 0, ui->libraryGrid->columnCount() - 2);
    cancelEditPlaylistButton->hide();

    libraryFilterLineEdit = new QLineEdit(this);
    libraryFilterLineEdit->setPlaceholderText("Filter");
    ui->libraryGrid->addWidget(libraryFilterLineEdit, 0, 2, 1, ui->libraryGrid->columnCount() - 4);

    ui->libraryFrame->setFrameShadow(QFrame::Sunken);

    libraryLoadingLabel = new QLabel(ui->libraryFrame);
    libraryLoadingLabel->setAttribute(Qt::WA_NoMousePropagation);
    libraryLoadingMovie = new QMovie("://images/Eclipse-1s-loading-200px.gif", nullptr, libraryLoadingLabel);
    libraryLoadingMovie->setScaledSize({200,200});
    libraryLoadingLabel->setMovie(libraryLoadingMovie);
    libraryLoadingInfoLabel = new QLabel(this);
    //libraryLoadingLabel->setStyleSheet("* {background-color: rgba(128,128,128, 0.5)}");
    libraryLoadingLabel->setProperty("cssClass", "libraryLoadingSpinner");
    libraryLoadingLabel->setAlignment(Qt::AlignCenter);
    libraryLoadingInfoLabel->setProperty("cssClass", "libraryLoadingSpinnerText");
    libraryLoadingInfoLabel->setAlignment(Qt::AlignCenter);
    ui->libraryGrid->addWidget(libraryLoadingLabel, 0, 0, 21, 12);
    ui->libraryGrid->addWidget(libraryLoadingInfoLabel, 0, 0, 21, 12);
    libraryLoadingLabel->hide();
    libraryLoadingInfoLabel->hide();

    thumbCaptureTime = 35000;
    libraryViewGroup = new QActionGroup(this);
    libraryViewGroup->addAction(ui->actionList);
    libraryViewGroup->addAction(ui->actionThumbnail);

    _videoPreviewWidget = new XVideoPreviewWidget(this);
    _videoPreviewWidget->hide();

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
    action200_Size = submenuSize->addAction( "200" );
    action200_Size->setCheckable(true);
    actionCustom_Size = submenuSize->addAction( "Custom" );
    actionCustom_Size->setCheckable(true);
    libraryThumbSizeGroup->addAction(action75_Size);
    libraryThumbSizeGroup->addAction(action100_Size);
    libraryThumbSizeGroup->addAction(action125_Size);
    libraryThumbSizeGroup->addAction(action150_Size);
    libraryThumbSizeGroup->addAction(action175_Size);
    libraryThumbSizeGroup->addAction(action200_Size);
    libraryThumbSizeGroup->addAction(actionCustom_Size);

    updateThumbSizeUI(SettingsHandler::getThumbSize());

    QMenu* submenuSort = ui->menuView->addMenu( "Sort" );
    submenuSort->setObjectName("sortMenu");
    librarySortGroup = new QActionGroup(submenuSort);
    actionNameAsc_Sort = submenuSort->addAction( "Name (Asc)" );
    actionNameAsc_Sort->setCheckable(true);
    actionNameDesc_Sort = submenuSort->addAction( "Name (Desc)" );
    actionNameDesc_Sort->setCheckable(true);
    actionRandom_Sort = submenuSort->addAction( "Random" );
    actionRandom_Sort->setCheckable(true);
    actionCreatedAsc_Sort = submenuSort->addAction( "Created (Asc)" );
    actionCreatedAsc_Sort->setCheckable(true);
    actionCreatedDesc_Sort = submenuSort->addAction( "Created (Desc)" );
    actionCreatedDesc_Sort->setCheckable(true);
    actionTypeAsc_Sort = submenuSort->addAction( "Type (Asc)" );
    actionTypeAsc_Sort->setCheckable(true);
    actionTypeDesc_Sort = submenuSort->addAction( "Type (Desc)" );
    actionTypeDesc_Sort->setCheckable(true);
    librarySortGroup->addAction(actionNameAsc_Sort);
    librarySortGroup->addAction(actionNameDesc_Sort);
    librarySortGroup->addAction(actionRandom_Sort);
    librarySortGroup->addAction(actionCreatedAsc_Sort);
    librarySortGroup->addAction(actionCreatedDesc_Sort);
    librarySortGroup->addAction(actionTypeAsc_Sort);
    librarySortGroup->addAction(actionTypeDesc_Sort);

    updateLibrarySortUI(SettingsHandler::getLibrarySortMode());

    if (SettingsHandler::getLibraryView() == LibraryView::List)
    {
        ui->actionList->setChecked(true);
    }
    else
    {
        ui->actionThumbnail->setChecked(true);
    }

    auto splitterSizes = XTPSettings::getMainWindowSplitterPos();
    if (splitterSizes.count() > 0)
        ui->mainFrameSplitter->setSizes(splitterSizes);

    auto sizes = ui->mainFrameSplitter->sizes();
    if(SettingsHandler::getEnableHttpServer()) {
        connect(xtEngine.httpHandler(), &HttpHandler::error, this, [this](QString error) {
            DialogHandler::MessageBox(this, error, XLogLevel::Critical);
        });
        connect(xtEngine.httpHandler(), &HttpHandler::skipToMoneyShot, this, &MainWindow::skipToMoneyShot);
        connect(xtEngine.httpHandler(), &HttpHandler::skipToNextAction, this, &MainWindow::skipToNextAction);
        connect(xtEngine.httpHandler(), &HttpHandler::connectInputDevice, _xSettings, &SettingsDialog::on_xtpWeb_initInputDevice);
        connect(xtEngine.httpHandler(), &HttpHandler::connectOutputDevice, _xSettings, &SettingsDialog::on_xtpWeb_initOutputDevice);
        connect(xtEngine.httpHandler(), &HttpHandler::restartService, _xSettings, &SettingsDialog::restart);
    }

    connect(&SettingsHandler::instance(), &SettingsHandler::messageSend, this, &MainWindow::on_settingsMessageRecieve);
    connect(_xSettings, &SettingsDialog::messageSend, this, &MainWindow::on_settingsMessageRecieve);

    connect(ui->mainFrameSplitter, &QSplitter::splitterMoved, this, &MainWindow::on_mainwindow_splitterMove);
    connect(backLibraryButton, &QPushButton::clicked, this, &MainWindow::backToMainLibrary);
    connect(randomizeLibraryButton, &QPushButton::clicked, this, &MainWindow::on_actionRandom_triggered);
    connect(windowedLibraryButton, &QPushButton::clicked, this, &MainWindow::onLibraryWindowed_Clicked);
    connect(savePlaylistButton, &QPushButton::clicked, this, &MainWindow::savePlaylist);
    connect(editPlaylistButton, &QPushButton::clicked, this, &MainWindow::editPlaylist);
    connect(cancelEditPlaylistButton, &QPushButton::clicked, this, &MainWindow::cancelEditPlaylist);
    connect(libraryFilterLineEdit, &QLineEdit::textChanged, _librarySortFilterProxyModel, &LibrarySortFilterProxyModel::onTextFilterChanged);

    connect(libraryWindow, &LibraryWindow::closeWindow, this, &MainWindow::onLibraryWindowed_Closed);

    connect(xtEngine.connectionHandler(), &ConnectionHandler::inputConnectionChange, this, &MainWindow::on_input_device_connectionChanged);
    connect(xtEngine.connectionHandler(), &ConnectionHandler::outputConnectionChange, this, &MainWindow::on_output_device_connectionChanged);
    connect(xtEngine.connectionHandler(), &ConnectionHandler::gamepadConnectionChange, this, &MainWindow::on_gamepad_connectionChanged);
    connect(xtEngine.connectionHandler(), &ConnectionHandler::outputConnectionChange, _xSettings, &SettingsDialog::on_output_device_connectionChanged);
    connect(xtEngine.connectionHandler(), &ConnectionHandler::inputConnectionChange, _xSettings, &SettingsDialog::on_input_device_connectionChanged);
    connect(xtEngine.connectionHandler(), &ConnectionHandler::gamepadConnectionChange, _xSettings, &SettingsDialog::on_gamepad_connectionChanged);



    connect(retryConnectionButton, &QPushButton::clicked, this, [this](bool checked){
        xtEngine.connectionHandler()->initOutputDevice(SettingsHandler::getSelectedOutputDevice());
    });
    connect(deoRetryConnectionButton, &QPushButton::clicked, this, [this](bool checked) {
        xtEngine.connectionHandler()->initInputDevice(SettingsHandler::getSelectedInputDevice());
    });
    connect(xtEngine.settingsActionHandler(), &SettingsActionHandler::actionExecuted, this, &MainWindow::mediaAction);

    connect(_xSettings, &SettingsDialog::TCodeHomeClicked, this, &MainWindow::deviceHome);
    connect(_xSettings, &SettingsDialog::onOpenWelcomeDialog, this, &MainWindow::openWelcomeDialog);
    connect(_xSettings, &SettingsDialog::skipToMoneyShot, this, &MainWindow::skipToMoneyShot);
    connect(_xSettings, &SettingsDialog::skipToNextAction, this, &MainWindow::skipToNextAction);
    connect(_xSettings, &SettingsDialog::updateLibrary, xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::libraryChange);
    connect(_xSettings, &SettingsDialog::disableHeatmapToggled, _playerControlsFrame, &PlayerControls::on_heatmapToggled);
    connect(_xSettings, &SettingsDialog::cleanUpThumbsDirectory, this, [this] () {
        QtConcurrent::run([this]() {
            if(xtEngine.mediaLibraryHandler()->isLibraryLoading()) {
                emit cleanUpThumbsFailed();
                return;
            }
            xtEngine.mediaLibraryHandler()->cleanGlobalThumbDirectory();
            emit cleanUpThumbsFinished();
        });
    });
    connect(xtEngine.syncHandler(), &SyncHandler::channelPositionChange, _xSettings, &SettingsDialog::setAxisProgressBar, Qt::QueuedConnection);
    connect(xtEngine.syncHandler(), &SyncHandler::funscriptEnded, _xSettings, &SettingsDialog::resetAxisProgressBars, Qt::QueuedConnection);
    connect(xtEngine.syncHandler(), &SyncHandler::funscriptLoaded, this, [this](QString funscriptPath) {
        // Generate first load moneyshot based off heatmap if not already set.
        if(funscriptPath != playingLibraryListItem.script)// Are we loading moneyshot/alt script?
            return;
        auto funscript = xtEngine.syncHandler()->getFunscriptHandler()->currentFunscript();
        if(funscript) {
            _playerControlsFrame->setActions(funscript->actions);
        }
        auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(playingLibraryListItem.path);
        if(libraryListItemMetaData.moneyShotMillis > 0)
            return;

        if(funscript) {
            m_heatmap->getMaxHeatAsync(funscript->actions);
        }
    });

    _xSettings->init(videoHandler, xtEngine.connectionHandler());

    //connect(this, &MainWindow::libraryIconResized, this, &MainWindow::libraryListSetIconSize);

    connect(ui->actionReload_library, &QAction::triggered, xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::loadLibraryAsync);
    connect(xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::libraryNotFound, this, &MainWindow::onLibraryNotFound);
    connect(xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::libraryLoaded, this, &MainWindow::onSetLibraryLoaded);
    connect(xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::libraryStopped, this, &MainWindow::onSetLibraryStopped);
    connect(xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::libraryLoadingStatus, this, &MainWindow::onLibraryLoadingStatusChange);
    connect(xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::libraryLoading, this, &MainWindow::onSetLibraryLoading);
    connect(xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::prepareLibraryLoad, this, &MainWindow::onPrepareLibraryLoad);
    connect(xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::saveThumbError, this, &MainWindow::onSaveThumbError);
    connect(xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::saveNewThumb, this, &MainWindow::onSaveNewThumb);
    connect(xtEngine.mediaLibraryHandler(), &MediaLibraryHandler::saveNewThumbLoading, this, &MainWindow::onSaveNewThumbLoading);

    connect(action75_Size, &QAction::triggered, this, &MainWindow::on_action75_triggered);
    connect(action100_Size, &QAction::triggered, this, &MainWindow::on_action100_triggered);
    connect(action125_Size, &QAction::triggered, this, &MainWindow::on_action125_triggered);
    connect(action150_Size, &QAction::triggered, this, &MainWindow::on_action150_triggered);
    connect(action175_Size, &QAction::triggered, this, &MainWindow::on_action175_triggered);
    connect(action200_Size, &QAction::triggered, this, &MainWindow::on_action200_triggered);
    connect(actionCustom_Size, &QAction::triggered, this, &MainWindow::on_actionCustom_triggered);

    connect(actionNameAsc_Sort, &QAction::triggered, this, &MainWindow::on_actionNameAsc_triggered);
    connect(actionNameDesc_Sort, &QAction::triggered, this, &MainWindow::on_actionNameDesc_triggered);
    connect(actionRandom_Sort, &QAction::triggered, this, &MainWindow::on_actionRandom_triggered);
    connect(actionCreatedAsc_Sort, &QAction::triggered, this, &MainWindow::on_actionCreatedAsc_triggered);
    connect(actionCreatedDesc_Sort, &QAction::triggered, this, &MainWindow::on_actionCreatedDesc_triggered);
    connect(actionTypeAsc_Sort, &QAction::triggered, this, &MainWindow::on_actionTypeAsc_triggered);
    connect(actionTypeDesc_Sort, &QAction::triggered, this, &MainWindow::on_actionTypeDesc_triggered);

    connect(xtEngine.syncHandler(), &SyncHandler::funscriptPositionChanged, this, &MainWindow::on_media_positionChanged, Qt::QueuedConnection);
    connect(xtEngine.syncHandler(), &SyncHandler::funscriptStatusChanged, this, &MainWindow::on_media_statusChanged, Qt::QueuedConnection);
    connect(xtEngine.syncHandler(), &SyncHandler::funscriptStarted, this, &MainWindow::on_standaloneFunscript_start, Qt::QueuedConnection);
    connect(xtEngine.syncHandler(), &SyncHandler::funscriptStopped, this, &MainWindow::on_standaloneFunscript_stop, Qt::QueuedConnection);
    connect(xtEngine.syncHandler(), &SyncHandler::funscriptSearchResult, this, &MainWindow::onFunscriptSearchResult);

    connect(videoHandler, &VideoHandler::positionChanged, this, &MainWindow::on_media_positionChanged, Qt::QueuedConnection);
    connect(videoHandler, &VideoHandler::mediaStatusChanged, this, &MainWindow::on_media_statusChanged, Qt::QueuedConnection);
    connect(videoHandler, &VideoHandler::started, this, &MainWindow::on_media_start, Qt::QueuedConnection);
    connect(videoHandler, &VideoHandler::stopped, this, &MainWindow::on_media_stop, Qt::QueuedConnection);
    connect(videoHandler, &VideoHandler::togglePaused, this, &MainWindow::on_togglePaused);
    connect(videoHandler, &VideoHandler::doubleClicked, this, &MainWindow::media_double_click_event);
    connect(videoHandler, &VideoHandler::singleClicked, this, &MainWindow::media_single_click_event);
    connect(videoHandler, &VideoHandler::keyPressed, this, &MainWindow::on_key_press);
    connect(videoHandler, &VideoHandler::keyReleased, this, &MainWindow::on_key_press);
    connect(videoHandler, &VideoHandler::togglePaused, this, [this](bool paused) {
        if(paused)
            xtEngine.connectionHandler()->stopOutputDevice();
    });
    connect(videoHandler, &VideoHandler::stopped, this, [this]() {
        xtEngine.connectionHandler()->stopOutputDevice();
    });
    connect(videoHandler, &VideoHandler::durationChange, this, [this](qint64 value) {
        //m_heatmap->drawPixmapAsync(_playerControlsFrame->width(), 25, xtEngine.syncHandler()->getFunscriptHandler(), value);
        _playerControlsFrame->setDuration(value);
        processMetaData(playingLibraryListItem);
    });

    m_heatmap = new HeatMap(this);
    connect(m_heatmap, &HeatMap::maxHeat, this, [this](qint64 maxHeatAt) {
        if(maxHeatAt > 0)
            onSetMoneyShot(playingLibraryListItem, maxHeatAt, false);
    });

    connect(_playerControlsFrame, &PlayerControls::seekSliderMoved, this, &MainWindow::on_timeline_currentTimeMove);
    connect(_playerControlsFrame, &PlayerControls::seekSliderHover, this, &MainWindow::on_seekslider_hover );
    connect(_playerControlsFrame, &PlayerControls::seekSliderLeave, this, &MainWindow::on_seekslider_leave );
    connect(_playerControlsFrame, &PlayerControls::volumeChanged, this, &MainWindow::on_VolumeSlider_valueChanged);
    connect(_playerControlsFrame, &PlayerControls::loopButtonToggled, this, &MainWindow::on_loopToggleButton_toggled);
    connect(_playerControlsFrame, &PlayerControls::muteChanged, this, &MainWindow::on_MuteBtn_toggled);
    connect(_playerControlsFrame, &PlayerControls::fullscreenToggled, this, &MainWindow::on_fullScreenBtn_clicked);
    connect(_playerControlsFrame, &PlayerControls::settingsClicked, this, &MainWindow::on_settingsButton_clicked);
    connect(_playerControlsFrame, &PlayerControls::playClicked, this, &MainWindow::on_PlayBtn_clicked);
    connect(_playerControlsFrame, &PlayerControls::stopClicked, this, &MainWindow::stopMedia);
    connect(_playerControlsFrame, &PlayerControls::skipForward, this, &MainWindow::on_skipForwardButton_clicked);
    connect(_playerControlsFrame, &PlayerControls::skipToMoneyShot, this, &MainWindow::skipToMoneyShot);
    connect(_playerControlsFrame, &PlayerControls::skipToNextAction, this, &MainWindow::skipToNextAction);
    connect(_playerControlsFrame, &PlayerControls::skipBack, this, &MainWindow::on_skipBackButton_clicked);

    connect(this, &MainWindow::keyPressed, this, &MainWindow::on_key_press);
    connect(this, &MainWindow::keyReleased, this, &MainWindow::on_key_press);
    connect(this, &MainWindow::change, this, &MainWindow::on_mainwindow_change);
    connect(this, &MainWindow::playVideo, this, &MainWindow::on_playVideo);
    connect(this, &MainWindow::stopAndPlayVideo, this, &MainWindow::stopAndPlayMedia);
    connect(this, &MainWindow::playlistLoaded, this, &MainWindow::onPlaylistLoaded);
    connect(this, &MainWindow::backFromPlaylistLoaded, this, &MainWindow::onBackFromPlaylistLoaded);
    connect(this, &MainWindow::cleanUpThumbsFinished, this, [this]() {
        _xSettings->onCleanUpThumbsDirectoryComplete();
        DialogHandler::MessageBox(this, "Thumb cleanup finished", XLogLevel::Information);

    });
    connect(this, &MainWindow::cleanUpThumbsFailed, this, [this]() {
        _xSettings->onCleanUpThumbsDirectoryStopped();
        DialogHandler::MessageBox(this, "Thumb cleanup cannot be run while the media is loading", XLogLevel::Warning);

    });


    //    connect(this, &MainWindow::setLoading, this, &MainWindow::on_setLoading);
    //connect(videoHandler, &VideoHandler::mouseEnter, this, &MainWindow::on_video_mouse_enter);

    connect(libraryList, &XLibraryList::customContextMenuRequested, this, &MainWindow::onLibraryList_ContextMenuRequested);
    connect(libraryList, &XLibraryList::doubleClicked, this, &MainWindow::on_LibraryList_itemDoubleClicked);
    connect(libraryList, &XLibraryList::clicked, this, &MainWindow::on_LibraryList_itemClicked);
    connect(libraryList, &XLibraryList::keyPressed, this, &MainWindow::on_key_press);
    connect(libraryList, &XLibraryList::keyReleased, this, &MainWindow::on_key_press);


    connect(QApplication::instance(), &QCoreApplication::aboutToQuit, this, &MainWindow::dispose);

    loadingSplash->showMessage(fullVersion + "\nSetting user styles...", Qt::AlignBottom, Qt::white);
    loadTheme(XTPSettings::getSelectedTheme());

    setFocus();
    _defaultAppSize = this->size();
    _appSize = _defaultAppSize;
    _appPos = this->pos();

    changeLibraryDisplayMode(SettingsHandler::getLibraryView());
    loadingSplash->showMessage(fullVersion + "\nLoading Library...", Qt::AlignBottom, Qt::white);
    xtEngine.init();

//    QScreen *screen = this->screen();
//    QSize screenSize = screen->size();
//    auto minHeight = round(screenSize.height() * .06f);
//    _playerControlsFrame->setMinimumHeight(minHeight);
//    _controlsHomePlaceHolderFrame->setMinimumHeight(minHeight);
//    _playerControlsFrame->setMaximumHeight(minHeight);
//    _controlsHomePlaceHolderFrame->setMaximumHeight(minHeight);

    loadingSplash->showMessage(fullVersion + "\nStarting Application...", Qt::AlignBottom, Qt::white);
    loadingSplash->finish(this);
    if(!SettingsHandler::getHideWelcomeScreen())
    {
        QTimer::singleShot(500, [this](){
            openWelcomeDialog();
        });
    }
}

MainWindow::~MainWindow()
{
}
void MainWindow::showEvent(QShowEvent* event) {
    XTPSettings::resetWindowSize();
    if(XTPSettings::getRememberWindowsSettings()) {
        auto pos = XTPSettings::getXWindowPosition();
        if(!pos.isNull()) {
            move(pos);
        }
        auto size = XTPSettings::getXWindowSize();
        if(!size.isNull()) {
            resize(size);
        }
        if(XTPSettings::getLibraryWindowOpen()) {
            onLibraryWindowed_Clicked();
        }
    }

    _windowInitialized = true;
}

void MainWindow::on_settingsMessageRecieve(QString message, XLogLevel logLevel) {
    DialogHandler::MessageBox(this, message, logLevel);
}
void MainWindow::onPasswordIncorrect()
{
    if(_isPasswordIncorrect)
        QApplication::quit();
}
void MainWindow::dispose()
{
    XTPSettings::setLibraryWindowOpen(_libraryDockMode);
    closeWelcomeDialog();
    if(!playingLibraryListItem.ID.isEmpty())
        updateMetaData(playingLibraryListItem);
    XTPSettings::save();
    loadingLibraryStop = true;
    _waitForStopFutureCancel = true;
    _mediaStopped = true;
    if (videoHandler->isPlaying())
    {
        videoHandler->stop();
    }
    _xSettings->dispose();
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
    else if (obj == (QObject*)playerLibraryPlaceHolder) {
        if (event->type() == QEvent::Enter)
        {
            showLibrary();
        }
        else if(event->type() == QEvent::Leave)
        {
            hideLibrary();
        }
        return true;
    }
    else
    {
        // pass the event on to the parent class
        return QWidget::eventFilter(obj, event);
    }
}


void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if(_windowInitialized)
        XTPSettings::setXWindowSize(event->size());
}
void MainWindow::moveEvent(QMoveEvent* event) {
    QMainWindow::moveEvent(event);
    if(_windowInitialized)
        XTPSettings::setXWindowPosition(event->pos());
}

void MainWindow::on_key_press(QKeyEvent * event)
{
    MediaActions mediaActions;
    auto keyActions = SettingsHandler::getKeyboardKeyActionList(event->key(), event->modifiers());
    QVector<ChannelValueModel> channelValues;
    foreach(auto action, keyActions) {
        if (!mediaActions.Values.contains(action) && !MediaActions::HasOtherAction(action)) {
            //auto channel = TCodeChannelLookup::getChannel(action);
            float value = event->type() == QKeyEvent::KeyRelease ? 0.0f : 1.0f;
//            if(event->type() == QKeyEvent::KeyRelease) {
//                if(channel.Type == AxisType::Range || channel.Type == AxisType::Switch) {
//                    value = -1.0f;
//                } else if(channel.Type == AxisType::HalfRange) {
//                    value = 0.0f;
//                }
//            }
            xtEngine.tcodeFactory()->calculate(action, value, channelValues);
        }
        else {
            if(event->type() == QKeyEvent::KeyRelease)
                continue;
            xtEngine.settingsActionHandler()->media_action(action);
        }
    }
    if(channelValues.length() > 0) {
        QString currentTCode = xtEngine.tcodeFactory()->formatTCode(&channelValues);
        if (_lastKeyboardTCode != currentTCode)
        {
            _lastKeyboardTCode = currentTCode;
            on_sendTCode(currentTCode);
        }
    }
}
/**
 * @brief MainWindow::mediaAction Called from SettingsActionHandler::actionExecuted signal.
 * @param action the NediaAction value executed
 * @param actionText the speech text
 */
void MainWindow::mediaAction(QString action, QString actionText)
{
    MediaActions actions;
    if (action == actions.TogglePause)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying())
        {
            videoHandler->togglePause();
        }
//        else if(SettingsHandler::getDeoEnabled() && _xSettings->getDeoHandler()->isConnected())
//        {
//            //_xSettings->getDeoHandler()->togglePause();
//        }
        else if(xtEngine.syncHandler()->isPlayingStandAlone())
        {
            xtEngine.syncHandler()->togglePause();
        }
        else
        {
            on_PlayBtn_clicked();
        }
    }
    else if(action == actions.FullScreen)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying())
            toggleFullScreen();
    }
    else if(action == actions.Mute)
    {
        on_MuteBtn_toggled(!videoHandler->isMute());
    }
    else if(action == actions.Stop)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying() || xtEngine.syncHandler()->isPlayingStandAlone())
            stopMedia();
    }
     else if(action == actions.Next)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying() || xtEngine.syncHandler()->isPlayingStandAlone())
            skipForward();
    }
    else if(action == actions.Back)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying() || xtEngine.syncHandler()->isPlayingStandAlone())
            skipBack();
    }
    else if(action == actions.VolumeUp)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying())
            _playerControlsFrame->IncreaseVolume();
    }
    else if(action == actions.VolumeDown)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying())
            _playerControlsFrame->DecreaseVolume();
    }
    else if(action == actions.Loop)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying())
            _playerControlsFrame->toggleLoop(videoHandler->duration(), videoHandler->position());
    }
    else if(action == actions.Rewind)
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying() || xtEngine.syncHandler()->isPlayingStandAlone())
            rewind();
    }
    else if(action == actions.FastForward )
    {
        if (videoHandler->isPaused() || videoHandler->isPlaying() || xtEngine.syncHandler()->isPlayingStandAlone())
            fastForward();
    }
    else if (action == actions.SkipToMoneyShot)
    {
        skipToMoneyShot();
    }
    else if (action == actions.SkipToAction)
    {
        skipToNextAction();
    }
    else if (action == actions.IncreaseOffset || action == actions.DecreaseOffset || action == actions.ResetOffset)//TODO: move to XTEngine
    {
        bool increase = action == actions.IncreaseOffset;
        QString verb = increase ? "Increase" : "Decrease";
        bool reset = false;
        if(action == actions.ResetOffset) {
            reset = true;
            verb = "reset";
        }
        if (xtEngine.syncHandler()->isPlaying())
        {
           QString path = playingLibraryListItem.path;
           if(!path.isEmpty())
           {
               auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(path);
               int newOffset = 0;
               if(!reset) {
                    newOffset = increase ? libraryListItemMetaData.offset + SettingsHandler::getFunscriptOffsetStep() : libraryListItemMetaData.offset - SettingsHandler::getFunscriptOffsetStep();
               }
               libraryListItemMetaData.offset = newOffset;
               SettingsHandler::setLiveOffset(newOffset);
               SettingsHandler::updateLibraryListItemMetaData(libraryListItemMetaData);
               onText_to_speech(verb + " offset to " + QString::number(newOffset));
           }
        }
        else
            onText_to_speech("No script playing to " + verb + " offset.");
    }
    else if(MediaActions::HasOtherAction(action, ActionType::CHANNEL_PROFILE)) {
        _xSettings->set_channelProfilesComboBox_value(action);
        onText_to_speech(actionText);
    }
    else
    {
        onText_to_speech(actionText);
    }
}

void MainWindow::onText_to_speech(QString message) {
    if(!message.isEmpty()) {
        if(!SettingsHandler::getDisableSpeechToText())
            textToSpeech->say(message);
        if(SettingsHandler::getEnableHttpServer())
            xtEngine.httpHandler()->sendWebSocketTextMessage("textToSpeech", message);
    }
}

void MainWindow::deviceHome()
{
    xtEngine.connectionHandler()->sendTCode(xtEngine.tcodeHandler()->getAllHome());
}
void MainWindow::deviceSwitchedHome()
{
    xtEngine.connectionHandler()->sendTCode(xtEngine.tcodeHandler()->getSwitchedHome());
}

void MainWindow::on_mainwindow_splitterMove(int pos, int index)
{
    XTPSettings::setMainWindowSplitterPos(ui->mainFrameSplitter->sizes());
}

//qint64 strokerUpdateMillis = 50;
qint64 strokerLastUpdate;
int lastAction = 500;
int minAmplitude = 0;
int maxAmplitude = 0;
//QElapsedTimer mSecTimer;
//qint64 timer1 = 0;
//qint64 timer2 = 0;
void MainWindow::on_audioLevel_Change(int decibelL,int decibelR)
{
//    if (timer2 - timer1 >= 1)
//    {
//    timer1 = timer2;
        if(xtEngine.connectionHandler()->isOutputDeviceConnected())
        {
    //        strokerLastUpdate = time;
            auto availibleAxis = TCodeChannelLookup::getChannels();
            auto decibelLInverse = -decibelL;
            auto decibelRInverse = -decibelR;
            auto difference = decibelLInverse > decibelRInverse ? decibelLInverse - decibelRInverse : decibelRInverse - decibelLInverse;
            int average = round(difference / 2);
            auto amplitude = decibelLInverse > decibelRInverse ? decibelLInverse - average : decibelRInverse - average;
            if(amplitude > minAmplitude || minAmplitude - amplitude > 25)
                minAmplitude = amplitude;
    //        if(amplitude > 0 && amplitude > maxAmplitude)
    //            maxAmplitude = amplitude;
            //auto delta = decibelLInverse > decibelRInverse ? amplitude / average : decibelRInverse - average;
            QString tcode;
            foreach(auto axis, availibleAxis)
            {
                ChannelModel33* channel = TCodeChannelLookup::getChannel(axis);
                if (channel->AxisName == TCodeChannelLookup::Stroke()  || SettingsHandler::getMultiplierChecked(axis))
                {
                    if (channel->Type == AxisType::HalfOscillate || channel->Type == AxisType::None)
                        continue;
                    auto multiplierValue = SettingsHandler::getMultiplierValue(axis);
                    if (channel->AxisName == TCodeChannelLookup::Stroke())
                        multiplierValue = 1.0f;
                    auto angle = XMath::mapRange(amplitude * 2, minAmplitude, maxAmplitude, 0, 180);
                    auto magnifiedAmplitude = XMath::mapRange(amplitude * 2, minAmplitude, maxAmplitude, 0, 100);
                    auto value = XMath::constrain(XMath::randSine(angle * multiplierValue, magnifiedAmplitude), 0, 100);
                    //auto value = int(XMath::mapRange(amplitude, minAmplitude, maxAmplitude, 1, 100) * multiplierValue);

                    int distance = value >= lastAction ? value - lastAction : lastAction - value;
                    if(distance > 25)
                    {
                        lastAction = value;
    //                    LogHandler::Debug("value: "+QString::number(value));
    //                    LogHandler::Debug("distance: "+QString::number(distance));
    //                    LogHandler::Debug("amplitude: "+QString::number(amplitude));
    //                    LogHandler::Debug("minAmplitude: "+QString::number(minAmplitude));
    //                    LogHandler::Debug("decibelLInverse: "+QString::number(decibelLInverse));
    //                    LogHandler::Debug("decibelRInverse: "+QString::number(decibelRInverse));
    //                    LogHandler::Debug("difference: "+QString::number(difference));
                        auto time = QTime::currentTime().msecsSinceStartOfDay();
                        int speed = time - strokerLastUpdate;
                        strokerLastUpdate = time;
                        //LogHandler::Debug("speed: "+QString::number(speed));

                        char tcodeValueString[4];
                        sprintf(tcodeValueString, "%03d", xtEngine.tcodeHandler()->calculateRange(axis.toUtf8(), value));
                        tcode += " ";
                        tcode += axis;
                        tcode += tcodeValueString;
                        tcode += "I";
                        float speedModifierValue = SettingsHandler::getDamperValue(axis);
                        if (SettingsHandler::getDamperChecked(axis) && speedModifierValue > 0.0 && speed > 1000 && distance > 50)
                        {
                            tcode += QString::number(round(speed * speedModifierValue));
                        }
                        else
                        {
                            tcode += QString::number(speed > 0 ? speed : 1000);
                        }
                    }
                }
            }
            if(!tcode.isEmpty())
                xtEngine.connectionHandler()->sendTCode(tcode);
        }
        //timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
    //}
}


void MainWindow::turnOffAudioSync()
{
    // disconnect(audioSyncFilter, &AudioSyncFilter::levelChanged, this, &MainWindow::on_audioLevel_Change);
    minAmplitude = 0;
    maxAmplitude = 0;
//    strokerUpdateMillis = 1000;
//    strokerLastUpdate = 500;
}

void MainWindow::onLibraryWindowed_Clicked()
{
    _libraryDockMode = true;
    ui->libraryGrid->removeWidget(libraryList);
    ui->libraryGrid->removeWidget(randomizeLibraryButton);
    ui->libraryGrid->removeWidget(windowedLibraryButton);
    ui->libraryGrid->removeWidget(backLibraryButton);
    ui->libraryGrid->removeWidget(cancelEditPlaylistButton);
    ui->libraryGrid->removeWidget(editPlaylistButton);
    ui->libraryGrid->removeWidget(savePlaylistButton);
    ui->libraryGrid->removeWidget(libraryLoadingLabel);
    ui->libraryGrid->removeWidget(libraryFilterLineEdit);
    ((QGridLayout*)libraryWindow->layout())->addWidget(libraryList, 1, 0, 20, 12);
    ((QGridLayout*)libraryWindow->layout())->addWidget(backLibraryButton, 0, 0);
    ((QGridLayout*)libraryWindow->layout())->addWidget(randomizeLibraryButton, 0, 1);
    ((QGridLayout*)libraryWindow->layout())->addWidget(windowedLibraryButton, 0, ui->libraryGrid->columnCount() - 1);
    ((QGridLayout*)libraryWindow->layout())->addWidget(cancelEditPlaylistButton, 0, ui->libraryGrid->columnCount() - 2);
    ((QGridLayout*)libraryWindow->layout())->addWidget(editPlaylistButton, 0, ui->libraryGrid->columnCount() - 2);
    ((QGridLayout*)libraryWindow->layout())->addWidget(savePlaylistButton, 0, ui->libraryGrid->columnCount() - 3);
    ((QGridLayout*)libraryWindow->layout())->addWidget(libraryFilterLineEdit, 0, 2, 1, ui->libraryGrid->columnCount() - 4);
    ((QGridLayout*)libraryWindow->layout())->addWidget(libraryLoadingLabel, 0, 0, 21, 12);
    libraryList->setParent(libraryWindow);
    windowedLibraryButton->hide();
    ui->libraryFrame->hide();
    libraryWindow->show();
    if(SettingsHandler::getLibrarySortMode() != LibrarySortMode::RANDOM)
        randomizeLibraryButton->hide();
     else if(!isPlaylistMode())
        randomizeLibraryButton->show();

    if(isPlaylistMode())
    {
        backLibraryButton->show();
        if(_editPlaylistMode)
        {
            savePlaylistButton->show();
            editPlaylistButton->hide();
            cancelEditPlaylistButton->show();
        }
        else
        {
            savePlaylistButton->hide();
            editPlaylistButton->show();
            cancelEditPlaylistButton->hide();
        }
    }
    else
    {
        backLibraryButton->hide();
        savePlaylistButton->hide();
        editPlaylistButton->hide();
    }
}

void MainWindow::onLibraryWindowed_Closed()
{
    libraryWindow->layout()->removeWidget(libraryList);
    libraryList->setParent(this);
    ui->libraryGrid->addWidget(libraryList, 1, 0, 20, 12);
    ui->libraryGrid->addWidget(backLibraryButton, 0, 0);
    ui->libraryGrid->addWidget(randomizeLibraryButton, 0, 1);
    ui->libraryGrid->addWidget(windowedLibraryButton, 0, ui->libraryGrid->columnCount() - 1);
    ui->libraryGrid->addWidget(cancelEditPlaylistButton, 0, ui->libraryGrid->columnCount() - 2);
    ui->libraryGrid->addWidget(editPlaylistButton, 0, ui->libraryGrid->columnCount() - 2);
    ui->libraryGrid->addWidget(savePlaylistButton, 0, ui->libraryGrid->columnCount() - 3);
    ui->libraryGrid->addWidget(libraryFilterLineEdit, 0, 2, 1, ui->libraryGrid->columnCount() - 4);
    ui->libraryGrid->addWidget(libraryLoadingLabel, 0, 0, 21, 12);
    windowedLibraryButton->show();
    ui->libraryFrame->show();
    if(SettingsHandler::getLibrarySortMode() != LibrarySortMode::RANDOM)
        randomizeLibraryButton->hide();
    else if(!isPlaylistMode())
        randomizeLibraryButton->show();

    if(isPlaylistMode())
    {
        backLibraryButton->show();
        if(_editPlaylistMode)
        {
            savePlaylistButton->show();
            editPlaylistButton->hide();
            cancelEditPlaylistButton->show();
        }
        else
        {
            savePlaylistButton->hide();
            editPlaylistButton->show();
            cancelEditPlaylistButton->hide();
        }
    }
    else
        backLibraryButton->hide();
    _libraryDockMode = false;
}

void MainWindow::onLibraryList_ContextMenuRequested(const QPoint &pos)
{
    if (libraryList->count() > 0)
    {
        // Handle global position
        QPoint globalPos = libraryList->mapToGlobal(pos);

        // Create menu and insert some actions
        QMenu myMenu;

        LibraryListItem27 selectedFileListItem = libraryList->selectedItem();

        myMenu.addAction(tr("Play"), this, &MainWindow::playFileFromContextMenu);
        if(selectedFileListItem.type == LibraryListItemType::PlaylistInternal)
        {
            myMenu.addAction(tr("Open"), this, [this]()
            {
                LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
                loadPlaylistIntoLibrary(selectedFileListItem.nameNoExtension);
            });
            myMenu.addAction(tr("Rename..."), this, &MainWindow::renamePlaylist);
            myMenu.addAction(tr("Delete..."), this, [this, selectedFileListItem]() {

                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, tr("WARNING!"), tr("Are you sure you want to delete the playlist: ") + selectedFileListItem.nameNoExtension,
                                              QMessageBox::Yes|QMessageBox::No);
                if (reply == QMessageBox::Yes)
                {
                    deleteSelectedPlaylist();
                }
            });
        }
        if(selectedFileListItem.type != LibraryListItemType::PlaylistInternal)
        {
            if(isPlaylistMode())
            {
                myMenu.addAction(tr("Remove from playlist"), this, &MainWindow::removeFromPlaylist);
            }
            if(selectedFileListItem.type != LibraryListItemType::FunscriptType)
            {
                myMenu.addAction(tr("Play with chosen funscript..."), this, &MainWindow::playFileWithCustomScript);
            }
            if(selectedFileListItem.type == LibraryListItemType::FunscriptType)
            {
                myMenu.addAction(tr("Play with chosen video..."), this, &MainWindow::playFileWithCustomMedia);
            }
            // Experimental
            //myMenu.addAction("Play with audio sync (Experimental)", this, &MainWindow::playFileWithAudioSync);
            if(!isPlaylistMode())
            {
                QMenu* subMenu = myMenu.addMenu(tr("Add to playlist"));
                subMenu->addAction(tr("New playlist..."), this, [this]()
                {
                    LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
                    QString playlist = getPlaylistName();
                    if(!playlist.isEmpty())
                        addSelectedLibraryItemToPlaylist(playlist, selectedFileListItem);
                });
                subMenu->addSeparator();
                auto playlists = SettingsHandler::getPlaylists();
                foreach(auto playlist, playlists.keys())
                {
                    subMenu->addAction(playlist, this, [this, playlist]()
                    {
                        LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
                        addSelectedLibraryItemToPlaylist(playlist, selectedFileListItem);
                    });
                }
            }

            if(xtEngine.mediaLibraryHandler()->isLibraryItemVideo(selectedFileListItem) && !selectedFileListItem.thumbFile.contains(".lock.") && !isPlaylistMode())
            {
                myMenu.addAction(tr("Regenerate thumbnail"), this, &MainWindow::regenerateThumbNail);

                if(!playingLibraryListItem.ID.isEmpty() && (videoHandler->isPlaying() || videoHandler->isPaused()))
                {
                    if(playingLibraryListItem.ID == selectedFileListItem.ID)
                        myMenu.addAction(tr("Set thumbnail from current"), this, &MainWindow::setThumbNailFromCurrent);
                }
                if(selectedFileListItem.thumbFileExists)
                    myMenu.addAction(tr("Lock thumb"), this, &MainWindow::lockThumb);
            }
            else if(xtEngine.mediaLibraryHandler()->isLibraryItemVideo(selectedFileListItem) &&  !isPlaylistMode())
            {
                if(selectedFileListItem.thumbFileExists)
                    myMenu.addAction(tr("Unlock thumb"), this, &MainWindow::unlockThumb);
            }
            if(!playingLibraryListItem.ID.isEmpty() && (videoHandler->isPlaying() || videoHandler->isPaused()))
            {
                if(playingLibraryListItem.ID == selectedFileListItem.ID)
                    myMenu.addAction(tr("Set moneyshot from current"), this, [this, selectedFileListItem] () {
                        onSetMoneyShot(selectedFileListItem, videoHandler->position());
                    });
            }
    //        myMenu.addAction("Add bookmark from current", this, [this, selectedFileListItem] () {
    //            onAddBookmark(selectedFileListItem, "Book mark 1", videoHandler->position());
    //        });
            myMenu.addAction(tr("Reveal in directory"), this, [this, selectedFileListItem] () {
                if(selectedFileListItem.path.isNull()) {
                    DialogHandler::MessageBox(this, "Invalid media path.", XLogLevel::Critical);
                    return;
                }
                if(!QFile::exists(selectedFileListItem.path)) {
                    DialogHandler::MessageBox(this, "Media does not exist.", XLogLevel::Critical);
                    return;
                }
                showInGraphicalShell(selectedFileListItem.path);
            });
            myMenu.addAction(tr("Edit media settings..."), this, [this, selectedFileListItem] () {
                LibraryItemSettingsDialog::getSettings(this, selectedFileListItem.path);
            });
        }

        // Show context menu at handling position
        myMenu.exec(globalPos);
    }
}

void MainWindow::changeDeoFunscript()
{
    InputDevicePacket playingPacket = xtEngine.connectionHandler()->getSelectedInputDevice()->getCurrentPacket();
    if (playingPacket.path != nullptr)
    {
        QFileInfo videoFile(playingPacket.path);
        funscriptFileSelectorOpen = true;
        QString funscriptPath = QFileDialog::getOpenFileName(this, tr("Choose script for video: ") + videoFile.fileName(), SettingsHandler::getLastSelectedLibrary(), "Script Files (*.funscript)");
        funscriptFileSelectorOpen = false;
        if (!funscriptPath.isEmpty())
        {
            SettingsHandler::setLinkedVRFunscript(playingPacket.path, funscriptPath);
            xtEngine.syncHandler()->clear();
            SettingsHandler::SaveLinkedFunscripts();
        }
    }
    else
    {
        DialogHandler::MessageBox(this, tr("No script for current video or no video playing"), XLogLevel::Information);
    }
}

void MainWindow::openWelcomeDialog()
{
    _welcomeDialog = new WelcomeDialog(this);
    _welcomeDialog->show();
    _welcomeDialog->raise();
    _welcomeDialog->activateWindow();
}
void MainWindow::closeWelcomeDialog()
{
    if(_welcomeDialog) {
        _welcomeDialog->close();
    }

}
//void MainWindow::loadLibraryAsync()
//{
//    QString library = SettingsHandler::getSelectedLibrary();
//    QString vrLibrary = SettingsHandler::getVRLibrary();
//    if(library.isEmpty() && vrLibrary.isEmpty())
//    {
//        setLibraryLoading(false);
//        return;
//    }
//    if(!loadingLibraryFuture.isRunning())
//    {
//        emit libraryLoadingStatus(true, library.isEmpty() ? "Loading VR library..." : "Loading library...");
//        loadingLibraryFuture = QtConcurrent::run([this, library, vrLibrary]() {
//            on_load_library(library.isEmpty() ? vrLibrary : library, library.isEmpty());
//        });
//    }
//}


void MainWindow::onPrepareLibraryLoad()
{
    libraryList->setIconSize({SettingsHandler::getThumbSize(),SettingsHandler::getThumbSize()});
    ui->actionReload_library->setDisabled(true);
    //ui->actionSelect_library->setDisabled(true);
    _playerControlsFrame->setDisabled(true);
}

void MainWindow::onLibraryNotFound()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "ERROR!", "The media library stored in settings does not exist anymore.\nChoose a new one now?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        on_actionSelect_library_triggered();
    }
}

//void MainWindow::on_load_library(QString path, bool vrMode)
//{
//    if (path.isEmpty())
//    {
//        return;
//    }
//    else
//    {
//        QDir directory(path);
//        if(!directory.exists())
//        {
//            emit libraryNotFound();
//            return;
//        }
//    }
//    QString thumbPath = SettingsHandler::getSelectedThumbsDir();
//    QDir thumbDir(thumbPath);
//    if (!thumbDir.exists())
//    {
//        thumbDir.mkdir(thumbPath);
//    }
////    QStringList videoTypes = QStringList()
////            << "*.mp4"
////            << "*.avi"
////            << "*.mpg"
////            << "*.wmv"
////            << "*.mkv"
////            << "*.webm"
////            << "*.mp2"
////            << "*.mpeg"
////            << "*.mpv"
////            << "*.ogg"
////            << "*.m4p"
////            << "*.m4v"
////            << "*.mov"
////            << "*.qt"
////            << "*.flv"
////            << "*.swf"
////            << "*.avchd";

////    QStringList audioTypes = QStringList()
////            << "*.m4a"
////            << "*.mp3"
////            << "*.aac"
////            << "*.flac"
////            << "*.wav"
////            << "*.wma";
//    QStringList playlistTypes = QStringList()
//            << "*.m3u";

//    QStringList mediaTypes;
//    QStringList videoTypes;
//    QStringList audioTypes;
//    foreach(auto ext, SettingsHandler::getVideoExtensions())
//        videoTypes.append("*."+ext);
//    foreach(auto ext, SettingsHandler::getAudioExtensions())
//        audioTypes.append("*."+ext);
//    mediaTypes.append(videoTypes);
//    mediaTypes.append(audioTypes);
//    QDirIterator library(path, mediaTypes, QDir::Files, QDirIterator::Subdirectories);

//    if(!vrMode)
//    {
//        emit prepareLibraryLoad();
//        QThread::sleep(1);

//        auto playlists = SettingsHandler::getPlaylists();
//        foreach(auto playlist, playlists.keys())
//        {
//            setupPlaylistItem(playlist);
//        }
//    }
//    QStringList funscriptsWithMedia;
//    QList<QString> excludedLibraryPaths = SettingsHandler::getLibraryExclusions();
//    while (library.hasNext())
//    {
//        if(loadingLibraryStop)
//            return;
//        QFileInfo fileinfo(library.next());
//        QString fileDir = fileinfo.dir().path();
//        bool isExcluded = false;
//        foreach(QString dir, excludedLibraryPaths)
//        {
//            if(dir != path && (fileDir.startsWith(dir, Qt::CaseInsensitive)))
//                isExcluded = true;
//        }
//        if (isExcluded)
//            continue;
//        QString videoPath = fileinfo.filePath();
//        QString videoPathTemp = fileinfo.filePath();
//        QString fileName = fileinfo.fileName();
//        QString fileNameTemp = fileinfo.fileName();
//        QString fileNameNoExtension = fileNameTemp.remove(fileNameTemp.lastIndexOf('.'), fileNameTemp.length() -  1);
//        QString scriptFile = fileNameNoExtension + ".funscript";
//        QString scriptPath;
//        QString scriptNoExtension = videoPathTemp.remove(videoPathTemp.lastIndexOf('.'), videoPathTemp.length() - 1);
//        fileNameTemp = fileinfo.fileName();
//        QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));

//        if (SettingsHandler::getSelectedFunscriptLibrary() == Q_NULLPTR)
//        {
//            scriptPath = scriptNoExtension + ".funscript";
//        }
//        else
//        {
//            scriptNoExtension = SettingsHandler::getSelectedFunscriptLibrary() + QDir::separator() + fileNameNoExtension;
//            scriptPath = SettingsHandler::getSelectedFunscriptLibrary() + QDir::separator() + scriptFile;
//        }

//        QFile fpath(scriptPath);
//        if (!fpath.exists())
//        {
//            scriptPath = nullptr;
//        }

//        LibraryListItemType libratyItemType = LibraryListItemType::Video;
//        QFileInfo scriptZip(scriptNoExtension + ".zip");
//        QString zipFile;
//        if(scriptZip.exists())
//            zipFile = scriptNoExtension + ".zip";
//        if(audioTypes.contains(mediaExtension))
//        {
//            libratyItemType = LibraryListItemType::Audio;
//        }
//        LibraryListItem27 item
//        {
//            libratyItemType,
//            videoPath, // path
//            fileName, // name
//            fileNameNoExtension, //nameNoExtension
//            scriptPath, // script
//            scriptNoExtension,
//            mediaExtension,
//            nullptr,
//            zipFile,
//            fileinfo.birthTime().date(),
//            0
//        };
//        item.thumbFile = mediaLibraryHandler.getThumbPath(item);
//        LibraryListWidgetItem* qListWidgetItem = new LibraryListWidgetItem(item, vrMode ? nullptr : libraryList);
//        if(!vrMode)
//            libraryList->addItem(qListWidgetItem);
//        vrMode ? cachedVRItems.push_back((LibraryListWidgetItem*)qListWidgetItem->clone()) : cachedLibraryItems.push_back((LibraryListWidgetItem*)qListWidgetItem->clone());
//        if(!vrMode && !scriptPath.isEmpty())
//            funscriptsWithMedia.append(scriptPath);
//        if(!vrMode && !zipFile.isEmpty())
//            funscriptsWithMedia.append(zipFile);
//    }

//    if(!vrMode && !SettingsHandler::getHideStandAloneFunscriptsInLibrary())
//    {
//        emit libraryLoadingStatus(true, "Searching for lone funscripts...");
//        QStringList funscriptTypes = QStringList()
//                << "*.funscript"
//                << "*.zip";
//        mediaTypes.clear();
//        mediaTypes.append(funscriptTypes);
//        QDirIterator funscripts(path, mediaTypes, QDir::Files, QDirIterator::Subdirectories);
//        auto availibleAxis = SettingsHandler::getAvailableAxis();
//        while (funscripts.hasNext())
//        {
//            if(loadingLibraryStop)
//                return;
//            QFileInfo fileinfo(funscripts.next());
//            QString fileName = fileinfo.fileName();
//            QString fileNameTemp = fileinfo.fileName();
//            QString scriptPath = fileinfo.filePath();
//            QString scriptPathTemp = fileinfo.filePath();
//            QString scriptNoExtension = scriptPathTemp.remove(scriptPathTemp.lastIndexOf('.'), scriptPathTemp.length() - 1);
//            QString scriptNoExtensionTemp = QString(scriptNoExtension);
//            if(funscriptsWithMedia.contains(scriptPath, Qt::CaseSensitivity::CaseInsensitive))
//                continue;

//            QString scriptMFSExt = scriptNoExtensionTemp.remove(0, scriptNoExtensionTemp.length() - (scriptNoExtensionTemp.length() - scriptNoExtensionTemp.lastIndexOf('.')));
//            bool isMfs = false;
//            foreach(auto axisName, availibleAxis->keys())
//            {
//                auto track = availibleAxis->value(axisName);
//                if("."+track.TrackName == scriptMFSExt)
//                {
//                    isMfs = true;
//                    break;
//                }
//            }
//            if(isMfs)
//                continue;

//            QString fileDir = fileinfo.dir().path();
//            bool isExcluded = false;
//            foreach(QString dir, excludedLibraryPaths)
//            {
//                if(dir != path && (fileDir.startsWith(dir, Qt::CaseInsensitive)))
//                    isExcluded = true;
//            }
//            if (isExcluded)
//                continue;
//            QString zipFile = nullptr;
//            if(scriptPath.endsWith(".zip", Qt::CaseInsensitive))
//            {
//                zipFile = scriptPath;
//            }
//            fileNameTemp = fileinfo.fileName();
//            QString fileNameNoExtension = fileNameTemp.remove(fileNameTemp.lastIndexOf('.'), fileNameTemp.length() -  1);
//            fileNameTemp = fileinfo.fileName();
//            QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));
//            LibraryListItem27 item
//            {
//                LibraryListItemType::FunscriptType,
//                scriptPath, // path
//                fileName, // name
//                fileNameNoExtension, //nameNoExtension
//                scriptPath, // script
//                scriptNoExtension,
//                mediaExtension,
//                nullptr,
//                zipFile,
//                fileinfo.birthTime().date(),
//                0
//            };
//            LibraryListWidgetItem* qListWidgetItem = new LibraryListWidgetItem(item, libraryList);
//            libraryList->addItem(qListWidgetItem);
//            cachedLibraryItems.push_back((LibraryListWidgetItem*)qListWidgetItem->clone());
//        }
//    }
//    if(vrMode)
//        emit libraryLoaded();
//    else
//    {
//        QString vrLibrary = SettingsHandler::getVRLibrary();
//        if(vrLibrary.isEmpty())
//        {
//            emit libraryLoaded();
//            return;
//        }
//        QFileInfo vrLibraryInfo(vrLibrary);
//        if(!vrLibraryInfo.exists())
//        {
//            emit libraryLoaded();
//            return;
//        }
//        emit libraryLoadingStatus(true, "Loading VR library...");
//        on_load_library(SettingsHandler::getVRLibrary(), true);
//    }

//}

//LibraryListItem27 MainWindow::createLibraryListItemFromFunscript(QString funscript)
//{

//    QFileInfo fileinfo(funscript);
//    QString fileName = fileinfo.fileName();
//    QString fileNameTemp = fileinfo.fileName();
//    QString scriptPath = fileinfo.filePath();
//    QString scriptPathTemp = fileinfo.filePath();
//    QString scriptNoExtension = scriptPathTemp.remove(scriptPathTemp.lastIndexOf('.'), scriptPathTemp.length() - 1);
//    QString scriptNoExtensionTemp = QString(scriptNoExtension);
//    QString fileDir = fileinfo.dir().path();
//    QString zipFile = nullptr;
//    if(scriptPath.endsWith(".zip", Qt::CaseInsensitive))
//    {
//        zipFile = scriptPath;
//        scriptPath = nullptr;
//    }
//    fileNameTemp = fileinfo.fileName();
//    QString fileNameNoExtension = fileNameTemp.remove(fileNameTemp.lastIndexOf('.'), fileNameTemp.length() -  1);
//    fileNameTemp = fileinfo.fileName();
//    QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));
//    return
//    {
//        LibraryListItemType::FunscriptType,
//        scriptPath, // path
//        fileName, // name
//        fileNameNoExtension, //nameNoExtension
//        scriptPath, // script
//        fileNameNoExtension,
//        mediaExtension,
//        nullptr,
//        zipFile,
//        fileinfo.birthTime().date(),
//        0
//    };
//}

void MainWindow::onSetLibraryLoading()
{
    LogHandler::Debug("onSetLibraryLoading");
    toggleLibraryLoading(true);
}
void MainWindow::toggleLibraryLoading(bool loading)
{
    if(loading)
    {
        libraryLoadingLabel->show();
        libraryLoadingMovie->start();
        libraryLoadingInfoLabel->show();
    }
    else
    {
        libraryLoadingLabel->hide();
        libraryLoadingMovie->stop();
        libraryLoadingInfoLabel->hide();
        libraryLoadingInfoLabel->setText("");
    }
    libraryList->setVisible(!loading);
    _playerControlsFrame->setDisabled(loading);
    randomizeLibraryButton->setDisabled(loading);
    savePlaylistButton->setDisabled(loading);
    editPlaylistButton->setDisabled(loading);
    cancelEditPlaylistButton->setDisabled(loading);
    windowedLibraryButton->setDisabled(loading);
    //ui->actionSelect_library->setDisabled(loading);
    ui->actionReload_library->setDisabled(loading);

    ui->actionThumbnail->setDisabled(loading);
    ui->actionList->setDisabled(loading);
    libraryThumbSizeGroup->setDisabled(loading);
    librarySortGroup->setDisabled(loading);

}
void MainWindow::onLibraryLoadingStatusChange(QString message)
{
    if(!message.isEmpty())
    {
        libraryLoadingInfoLabel->setText(message);
    }
}
void MainWindow::onSetLibraryStopped() {
    LogHandler::Debug("onSetLibraryStopped");
    toggleLibraryLoading(false);
}
void MainWindow::onSetLibraryLoaded()
{
//    QDialog* widget = new QDialog(this);
//    QGridLayout* layout = new QGridLayout(widget);
//    QListView* libraryListView = new QListView(widget);
//    libraryListView->setUniformItemSizes(true);
//    libraryListView->setContextMenuPolicy(Qt::CustomContextMenu);
//    //libraryListView->setProperty("id", "libraryList");
//    libraryListView->setMovement(QListView::Static);
//    libraryListView->setTextElideMode(Qt::TextElideMode::ElideRight);
//    libraryListView->setWordWrap(true);

//    libraryListView->setResizeMode(QListView::Adjust);
//    libraryListView->setFlow(QListView::LeftToRight);
//    libraryListView->setViewMode(QListView::IconMode);
//    libraryListView->setTextElideMode(Qt::ElideMiddle);
//    libraryListView->setSpacing(2);

//    layout->addWidget(libraryListView);
//    widget->setLayout(layout);
//    _libraryListViewModel = new LibraryListViewModel(widget);
//    libraryListView->setModel(_libraryListViewModel);
//    widget->setModal(false);
//    widget->show();

    //_libraryListViewModel->populate(xtEngine.mediaLibraryHandler()->getLibraryCache());
    //changeLibraryDisplayMode(SettingsHandler::getLibraryView());
    sortLibraryList(SettingsHandler::getLibrarySortMode());
}

//void MainWindow::startThumbProcess(bool vrMode)
//{
//    stopThumbProcess();
//    thumbProcessIsRunning = true;
//    extractor = new VideoFrameExtractor(this);
//    thumbNailPlayer = new AVPlayer(this);
//    thumbNailPlayer->setInterruptOnTimeout(true);
//    thumbNailPlayer->setInterruptTimeout(10000);
//    thumbNailPlayer->setAsyncLoad(true);
//    extractor->setAsync(true);
//    saveNewThumbs(vrMode);
//}

//void MainWindow::stopThumbProcess()
//{
//    if(thumbProcessIsRunning)
//    {
//        disconnect(extractor, &QtAV::VideoFrameExtractor::frameExtracted,  nullptr, nullptr);
//        disconnect(extractor, &QtAV::VideoFrameExtractor::error,  nullptr, nullptr);
//        disconnect(thumbNailPlayer, &AVPlayer::loaded,  nullptr, nullptr);
//        disconnect(thumbNailPlayer, &AVPlayer::error,  nullptr, nullptr);
//        thumbNailSearchIterator = 0;
//        thumbProcessIsRunning = false;
//        delete extractor;
//        delete thumbNailPlayer;
//    }
//}

//void MainWindow::saveSingleThumb(LibraryListWidgetItem* qListWidgetItem, qint64 position)
//{
//    if(!thumbProcessIsRunning)
//    {
//        extractor = new VideoFrameExtractor(this);
//        thumbNailPlayer = new AVPlayer(this);
//        thumbNailPlayer->setInterruptOnTimeout(true);
//        thumbNailPlayer->setInterruptTimeout(10000);
//        thumbNailPlayer->setAsyncLoad(true);
//        extractor->setAsync(true);
//    }
//    saveThumb(qListWidgetItem, position);
//}

//void MainWindow::saveNewThumbs(bool vrMode)
//{
//    if (thumbProcessIsRunning && thumbNailSearchIterator < (vrMode ? cachedVRItems.count() : cachedLibraryItems.count()))
//    {
//        //Use a non user modifiable list incase they sort random when getting thumbs.
//        LibraryListWidgetItem* listWidgetItem = vrMode ? cachedVRItems.at(thumbNailSearchIterator) : cachedLibraryItems.at(thumbNailSearchIterator);
//        LibraryListItem27 item = listWidgetItem->getLibraryListItem();
//        thumbNailSearchIterator++;
//        QFileInfo thumbInfo(mediaLibraryHandler.getThumbPath(item));
//        if (xtEngine.mediaLibraryHandler()->isLibraryItemVideo(item) && !thumbInfo.exists())
//        {
//            disconnect(extractor, nullptr,  nullptr, nullptr);
//            disconnect(thumbNailPlayer, nullptr,  nullptr, nullptr);
//            saveThumb(listWidgetItem, -1, vrMode);
//        }
//        else
//        {

//            saveNewThumbs(vrMode);
//        }
//    }
//    else
//    {
//        stopThumbProcess();
//        if(!vrMode)
//            startThumbProcess(true);
//    }
//}
//void MainWindow::saveThumb(LibraryListWidgetItem* cachedListWidgetItem, qint64 position, bool vrMode)
//{
//    LibraryListItem27 cachedListItem = cachedListWidgetItem->getLibraryListItem();
//    QString videoFile = cachedListItem.path;
//    QString thumbFile = mediaLibraryHandler.getThumbPath(cachedListItem);
////    QIcon thumb;
////    QPixmap bgPixmap(QApplication::applicationDirPath() + "/themes/loading.png");
////    QPixmap scaled = bgPixmap.scaled(SettingsHandler::getThumbSize(), Qt::AspectRatioMode::KeepAspectRatio);
////    thumb.addPixmap(scaled);
////    qListWidgetItem->setIcon(thumb);
//    auto libraryListItems = libraryList->findItems(cachedListItem.nameNoExtension, Qt::MatchFlag::MatchEndsWith);
//    LibraryListWidgetItem* libraryListWidgetItem = 0;
//    if(libraryListItems.length() > 0)
//    {
//        libraryListWidgetItem = ((LibraryListWidgetItem*)libraryListItems.first());
//        libraryListWidgetItem->setThumbFileLoading(false);
//    }
//    cachedListWidgetItem->setThumbFileLoading(false);
//    //://images/icons/loading_current.png
//    if(cachedListItem.type == LibraryListItemType::Audio || cachedListItem.type == LibraryListItemType::FunscriptType)
//    {
////        int thumbSize = SettingsHandler::getThumbSize();
////        QSize size = {thumbSize, thumbSize};
////        cachedListWidgetItem->updateThumbSize(size);
////        if(libraryListWidgetItem)
////        {
////            libraryList->removeItemWidget(libraryListWidgetItem);
////            libraryListWidgetItem->updateThumbSize(size);
////        }

//        saveNewThumbs(vrMode);
//    }
//    else
//    {
//        // Get the duration and randomize the position with in the video.
//        connect(thumbNailPlayer,
//           &AVPlayer::loaded,
//           thumbNailPlayer,
//           [this, videoFile, position]()
//            {
//               LogHandler::Debug(tr("Loaded video for thumb duration: ") + QString::number(thumbNailPlayer->duration()));
//               qint64 randomPosition = position > 0 ? position : XMath::rand((qint64)1, thumbNailPlayer->duration());

//               disconnect(thumbNailPlayer, &AVPlayer::loaded,  nullptr, nullptr);
//               disconnect(thumbNailPlayer, &AVPlayer::error,  nullptr, nullptr);
//               extractor->setSource(videoFile);
//               extractor->setPosition(randomPosition);
//            });


//        connect(thumbNailPlayer,
//           &AVPlayer::error,
//           thumbNailPlayer,
//           [this, videoFile,  cachedListWidgetItem, libraryListWidgetItem, vrMode](QtAV::AVError er)
//            {
//            QString error = tr("Video load error from: ") + videoFile + tr(" Error: ") + er.ffmpegErrorString();
//               LogHandler::Error(error);
//               saveThumbError(cachedListWidgetItem, libraryListWidgetItem, vrMode, error);
//            });


//        connect(extractor,
//           &QtAV::VideoFrameExtractor::frameExtracted,
//           extractor,
//           [this, videoFile, thumbFile, cachedListWidgetItem, libraryListWidgetItem, vrMode](const QtAV::VideoFrame& frame)
//            {
//                if(frame.isValid())
//                {
//                    bool error = false;
//                    QImage img;
//                    try{
//                        LogHandler::Debug(tr("Saving thumbnail: ") + thumbFile + tr(" for video: ") + videoFile);
//                        img = frame.toImage();
////                        auto vf = VideoFormat::pixelFormatFromImageFormat(QImage::Format_ARGB32);
////                        auto vf2 = VideoFormat(vf);
////                        VideoFrame f = frame.to(vf2, {frame.width(), frame.height()}, QRect(0,0,frame.width(), frame.height()));
////                        QImage img(f.frameDataPtr(), f.width(), f.height(), f.bytesPerLine(0), QImage::Format_ARGB32);
//                    }
//                    catch (...) {
//                        error = true;
//                    }
//                    QString errorMessage;
//                    bool hasError = error || img.isNull() || !img.save(thumbFile, nullptr, 15);
//                    if (hasError)
//                    {
//                       errorMessage = tr("Error saving thumbnail: ") + thumbFile + tr(" for video: ") + videoFile;
//                       LogHandler::Debug(errorMessage);
//                    }
//                    cachedListWidgetItem->setThumbFileLoaded(hasError, errorMessage, thumbFile);
//                    if(libraryListWidgetItem)
//                    {
//                        libraryList->removeItemWidget(libraryListWidgetItem);
//                        libraryListWidgetItem->setThumbFileLoaded(hasError, errorMessage, thumbFile);;
//                    }
//                }
//                disconnect(extractor, &QtAV::VideoFrameExtractor::frameExtracted,  nullptr, nullptr);
//                disconnect(extractor, &QtAV::VideoFrameExtractor::error,  nullptr, nullptr);

//               saveNewThumbs(vrMode);
//            });
//        connect(extractor,
//           &QtAV::VideoFrameExtractor::error,
//           extractor,
//           [this, videoFile, cachedListWidgetItem, libraryListWidgetItem, vrMode](const QString &errorMessage)
//            {
//                QString error = tr("Error extracting image from: ") + videoFile + tr(" Error: ") + errorMessage;
//                LogHandler::Error(error);
//                saveThumbError(cachedListWidgetItem, libraryListWidgetItem, vrMode, error);
//            });

//        thumbNailPlayer->setFile(videoFile);
//        thumbNailPlayer->load();
//    }
//}
void MainWindow::onSaveNewThumbLoading(LibraryListItem27 item)
{
//    auto libraryListItems = xtEngine.mediaLibraryHandler()->findItemByID(item.ID);
//    if(libraryListItems.length() > 0)
//    {
//        LibraryListWidgetItem* libraryListWidgetItem = (LibraryListWidgetItem*)libraryListItems.first();
//        libraryListWidgetItem->setThumbFile(item.thumbFileLoadingCurrent);
//    }
}

void MainWindow::onSaveNewThumb(LibraryListItem27 item, bool vrMode, QString thumbFile)
{

//    LibraryListWidgetItem* cachedListWidgetItem = boolinq::from(cachedLibraryWidgetItems).firstOrDefault([item](LibraryListWidgetItem* x) { return x->getLibraryListItem().path == item.path; });
//    cachedListWidgetItem->setThumbFile(thumbFile);

//    auto libraryListItems = libraryList->findItems(item.nameNoExtension, Qt::MatchFlag::MatchEndsWith);
//    if(libraryListItems.length() > 0)
//    {
//        LibraryListWidgetItem* libraryListWidgetItem = (LibraryListWidgetItem*)libraryListItems.first();
//        libraryList->removeItemWidget(libraryListWidgetItem);
//        libraryListWidgetItem->setThumbFile(thumbFile);
//    }
}

void MainWindow::onSaveThumbError(LibraryListItem27 item, bool vrMode, QString errorMessage)
{
//    if(item.ID.isNull()) {
//        DialogHandler::MessageBox(this, "Missing media", XLogLevel::Critical);
//        return;
//    }

//    LibraryListWidgetItem* cachedListWidgetItem = boolinq::from(cachedLibraryWidgetItems).firstOrDefault([item](LibraryListWidgetItem* x) { return x->getLibraryListItem().path == item.path; });
//    cachedListWidgetItem->setThumbFile(item.thumbFile, errorMessage);

//    auto libraryListItems = libraryList->findItems(item.nameNoExtension, Qt::MatchFlag::MatchEndsWith);
//    if(libraryListItems.length() > 0)
//    {
//        LibraryListWidgetItem* libraryListWidgetItem = (LibraryListWidgetItem*)libraryListItems.first();
//        libraryList->removeItemWidget(libraryListWidgetItem);
//        libraryListWidgetItem->setThumbFile(item.thumbFile, errorMessage);
//    }
}

void MainWindow::on_actionSelect_library_triggered()
{
    QStringList oldPaths = SettingsHandler::getSelectedLibrary();
    QString firstPathExists;
    foreach(auto path, oldPaths) {
        if(QFileInfo::exists(path)) {
            firstPathExists = path;
            break;
        }
    }
    QDir currentDir(firstPathExists);
    QString defaultPath = !firstPathExists.isEmpty() ? firstPathExists : ".";
    LibraryManager libraryManager;
    libraryManager.exec();
    QStringList currentPaths = SettingsHandler::getSelectedLibrary();

    if(!currentPaths.isEmpty()) {
        if(oldPaths.isEmpty()) {
            xtEngine.mediaLibraryHandler()->loadLibraryAsync();
            return;
        }
        QSet<QString> additions = QSet<QString>(currentPaths.begin(),currentPaths.end()).subtract(QSet<QString>(oldPaths.begin(), oldPaths.end()));
        QSet<QString> subtraction = QSet<QString>(oldPaths.begin(), oldPaths.end()).subtract(QSet<QString>(currentPaths.begin(),currentPaths.end()));
        if(!subtraction.isEmpty() || !additions.empty()) {
            auto message = xtEngine.mediaLibraryHandler()->isLibraryLoading() ? "Stop current loading process and restart with new list now?" : "Load all libraries now?";
            if(DialogHandler::Dialog(this, message) == QDialog::DialogCode::Accepted) {
                xtEngine.mediaLibraryHandler()->loadLibraryAsync();
            }
        }
    }
//    QString selectedLibrary = QFileDialog::getExistingDirectory(this, tr("Choose media library"), defaultPath, QFileDialog::ReadOnly);
//    if (selectedLibrary != Q_NULLPTR)
//    {
//        SettingsHandler::addSelectedLibrary(selectedLibrary);
//        xtEngine.mediaLibraryHandler()->loadLibraryAsync();
//    }
}

void MainWindow::on_LibraryList_itemClicked(QModelIndex index)
{
    if(index.isValid())
    {
        LibraryListItem27 selectedFileListItem = index.data().value<LibraryListItem27>();
        if((videoHandler->isPlaying() && !videoHandler->isPaused()))
        {
            auto playingFile = videoHandler->file();
            _playerControlsFrame->setPlayIcon(playingFile == selectedFileListItem.path);
        }
        else if(xtEngine.syncHandler()->isPlayingStandAlone() && !xtEngine.syncHandler()->isPaused())
        {
            auto playingFile = xtEngine.syncHandler()->getPlayingStandAloneScript();
            _playerControlsFrame->setPlayIcon(playingFile == selectedFileListItem.path);
        }
        ui->statusbar->showMessage(selectedFileListItem.nameNoExtension);
//        selectedLibraryListItem = (LibraryListWidgetItem*)item;
//        selectedLibraryListIndex = libraryList->currentRow();
    }
}

void MainWindow::regenerateThumbNail()
{
    xtEngine.mediaLibraryHandler()->saveSingleThumb(libraryList->selectedItem().ID);
}

void MainWindow::setThumbNailFromCurrent()
{
    xtEngine.mediaLibraryHandler()->saveSingleThumb(libraryList->selectedItem().ID, videoHandler->position());
}

void MainWindow::lockThumb()
{
    LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
    xtEngine.mediaLibraryHandler()->lockThumb(selectedFileListItem);
}
void MainWindow::unlockThumb()
{
    LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
    xtEngine.mediaLibraryHandler()->unlockThumb(selectedFileListItem);
}

void MainWindow::on_LibraryList_itemDoubleClicked(QModelIndex index)
{
    auto libraryListItem = libraryList->item(index);
    if(libraryListItem.type == LibraryListItemType::Audio ||
            xtEngine.mediaLibraryHandler()->isLibraryItemVideo(libraryListItem) ||
            libraryListItem.type == LibraryListItemType::FunscriptType)
    {
        stopAndPlayMedia(libraryListItem);
    }
    else if(libraryListItem.type == LibraryListItemType::PlaylistInternal)
    {
        loadPlaylistIntoLibrary(libraryListItem.nameNoExtension);
    }
}

void MainWindow::playFileFromContextMenu()
{
    if(libraryList->count() > 0)
    {
        LibraryListItem27 libraryListItem = libraryList->selectedItem();
        if(libraryListItem.type == LibraryListItemType::Audio || xtEngine.mediaLibraryHandler()->isLibraryItemVideo(libraryListItem) || libraryListItem.type == LibraryListItemType::FunscriptType)
        {
            stopAndPlayMedia(libraryListItem);
        }
        else if(libraryListItem.type == LibraryListItemType::PlaylistInternal)
        {
            loadPlaylistIntoLibrary(libraryListItem.nameNoExtension, true);
        }
    }
}

void MainWindow::playFileWithAudioSync()
{
    LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
    stopAndPlayMedia(selectedFileListItem, nullptr, true);
}

void MainWindow::playFileWithCustomScript()
{
    LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
    QString selectedScript = QFileDialog::getOpenFileName(this, tr("Choose script"), QFileInfo(selectedFileListItem.path).absolutePath(), tr("Scripts (*.funscript *.zip)"));
    if (selectedScript != Q_NULLPTR)
    {
        stopAndPlayMedia(selectedFileListItem, selectedScript);
    }
}

void MainWindow::playFileWithCustomMedia()
{
    QStringList mediaTypes;
    QStringList videoTypes;
    QStringList audioTypes;
    foreach(auto ext, SettingsHandler::getVideoExtensions())
        videoTypes.append("*."+ext);
    foreach(auto ext, SettingsHandler::getAudioExtensions())
        audioTypes.append("*."+ext);
    mediaTypes.append(videoTypes);
    mediaTypes.append(audioTypes);
    const QString mediaMemeType = "Media (" + mediaTypes.join(QString(" ")) + ")";
    LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
    QString selectedMedia = QFileDialog::getOpenFileName(this, tr("Choose media"), QFileInfo(selectedFileListItem.path).absolutePath(), mediaMemeType);
    if (!selectedMedia.isEmpty())
    {
        LibraryListItem27 selectedMediaItem;
        selectedMediaItem.path = selectedMedia;
        auto fileNameTemp = QFileInfo(selectedMedia).fileName();
        QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));
        selectedMediaItem.type = videoTypes.contains(mediaExtension) ? LibraryListItemType::Video : LibraryListItemType::Audio;
        selectedMediaItem.script = selectedFileListItem.script;
        selectedMediaItem.zipFile = selectedFileListItem.zipFile;
        stopAndPlayMedia(selectedMediaItem);
    }
}

//Hack because QTAV calls stopped and start out of order
void MainWindow::stopAndPlayMedia(LibraryListItem27 selectedFileListItem, QString customScript, bool audioSync)
{
    QFile file(selectedFileListItem.path);
    if (file.exists())
    {
        if ((!videoHandler->isPlaying() && !xtEngine.syncHandler()->isPlayingStandAlone())
              || ((videoHandler->isPlaying() || videoHandler->isPaused()) && videoHandler->file() != selectedFileListItem.path)
              || (xtEngine.syncHandler()->isPlayingStandAlone() && xtEngine.syncHandler()->getPlayingStandAloneScript() != selectedFileListItem.path)
              || !customScript.isEmpty()
              || audioSync)
        {
            videoHandler->setLoading(true);
            if(videoHandler->isPlaying() || xtEngine.syncHandler()->isPlayingStandAlone())
            {
                stopMedia();
                if(_waitForStopFuture.isRunning())
                {
                    _waitForStopFuture.cancel();
                    _waitForStopFutureCancel = true;
                    _waitForStopFuture.waitForFinished();
                }

                _waitForStopFuture = QtConcurrent::run([this, selectedFileListItem, customScript, audioSync]()
                {
                    while(!_mediaStopped)
                    {
                        LogHandler::Debug(tr("Waiting for media stop..."));
                        if(!_waitForStopFutureCancel)
                            QThread::msleep(500);
                        else {
                            _waitForStopFutureCancel = false;
                            return;
                        }
                    }
                    emit playVideo(selectedFileListItem, customScript, audioSync);
                });
            }
            else
            {
                on_playVideo(selectedFileListItem, customScript, audioSync);
            }
        }
    }

}

#include "lib/tool/heatmap.h"
void MainWindow::on_playVideo(LibraryListItem27 selectedFileListItem, QString customScript, bool audioSync)
{
    QFile file(selectedFileListItem.path);
    if (file.exists())
    {
        if ((!videoHandler->isPlaying() && !xtEngine.syncHandler()->isPlayingStandAlone())
                || (selectedFileListItem.type == LibraryListItemType::FunscriptType && xtEngine.syncHandler()->getPlayingStandAloneScript() != selectedFileListItem.path)
                || (videoHandler->file() != selectedFileListItem.path
                || !customScript.isEmpty()))
        {
            QString scriptFile;
            QList<QString> invalidScripts;
            deviceHome();
            videoHandler->setLoading(true);
            xtEngine.syncHandler()->stopAll();

            //playingLibraryListIndex = libraryList->selectedRow();
            playingLibraryListItem = selectedFileListItem;

            if(selectedFileListItem.type != LibraryListItemType::FunscriptType)
            {
                videoHandler->setFile(selectedFileListItem.path);
                _videoPreviewWidget->setFile(selectedFileListItem.path);
                //videoHandler->load();
            }
            if(!audioSync)
            {
                turnOffAudioSync();
                scriptFile = customScript.isEmpty() ? selectedFileListItem.zipFile.isEmpty() ? selectedFileListItem.script : selectedFileListItem.zipFile : customScript;
                invalidScripts = xtEngine.syncHandler()->load(scriptFile);
            }
            else
            {
                turnOffAudioSync();
                //strokerLastUpdate = QTime::currentTime().msecsSinceStartOfDay();
                //connect(audioSyncFilter, &AudioSyncFilter::levelChanged, this, &MainWindow::on_audioLevel_Change);
            }
            QString filesWithLoadingIssues = "";
            if(selectedFileListItem.type == LibraryListItemType::FunscriptType && xtEngine.syncHandler()->isLoaded())
                xtEngine.syncHandler()->playStandAlone();
            else if(selectedFileListItem.type == LibraryListItemType::FunscriptType && !xtEngine.syncHandler()->isLoaded())
            {
                on_scriptNotFound("No scripts found for the media with the same name: " + selectedFileListItem.path);
                skipForward();
            }
            else if(selectedFileListItem.type != LibraryListItemType::FunscriptType)
                videoHandler->play();

            if(!invalidScripts.empty())
            {
                filesWithLoadingIssues += "The following scripts had issues loading:\n\n";
                foreach(auto invalidFunscript, invalidScripts)
                    filesWithLoadingIssues += "* " + invalidFunscript + "\n";
                filesWithLoadingIssues += "\n\nThis is may be due to an invalid JSON format.\nTry downloading the script again or asking the script maker.\nYou may also find some information running XTP in debug mode.";
                DialogHandler::MessageBox(this, filesWithLoadingIssues, XLogLevel::Critical);
            }
            if(selectedFileListItem.type != LibraryListItemType::FunscriptType && !audioSync && !xtEngine.syncHandler()->isLoaded() && !invalidScripts.contains(scriptFile))
            {
                on_scriptNotFound(scriptFile);
            }
        }
    }
    else
    {
        DialogHandler::MessageBox(this, tr("File '") + selectedFileListItem.path + tr("' does not exist!"), XLogLevel::Critical);
    }
}

void MainWindow::processMetaData(LibraryListItem27 libraryListItem)
{
    auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(libraryListItem.path);
    if(libraryListItem.type != LibraryListItemType::VR)
    {
        if(libraryListItemMetaData.lastLoopEnabled && libraryListItemMetaData.lastLoopStart > -1 && libraryListItemMetaData.lastLoopEnd > libraryListItemMetaData.lastLoopStart)
        {
            _playerControlsFrame->SetLoop(true);
        }
    }
    SettingsHandler::setLiveOffset(libraryListItemMetaData.offset);
}

void MainWindow::updateMetaData(LibraryListItem27 libraryListItem)
{
    auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(libraryListItem.path);
    if(libraryListItem.type != LibraryListItemType::VR)
    {
        libraryListItemMetaData.lastPlayPosition = videoHandler->position();
        libraryListItemMetaData.lastLoopEnabled = _playerControlsFrame->getAutoLoop();
        if(libraryListItemMetaData.lastLoopEnabled)
        {
            libraryListItemMetaData.lastLoopStart = _playerControlsFrame->getStartLoop();
            libraryListItemMetaData.lastLoopEnd = _playerControlsFrame->getEndLoop();
        }
    }
    SettingsHandler::updateLibraryListItemMetaData(libraryListItemMetaData);
}

void MainWindow::on_mainwindow_change(QEvent* event)
{

    if (event->type() == QEvent::WindowStateChange)
    {
        QWindowStateChangeEvent* stateEvent = (QWindowStateChangeEvent*)event;
        if(stateEvent->oldState() == Qt::WindowState::WindowNoState)
        {
            LogHandler::Debug("oldState No state: "+QString::number(stateEvent->oldState()));
        }
        else if(stateEvent->oldState() == Qt::WindowState::WindowMaximized && !_isFullScreen && !_isMaximized)
        {
            LogHandler::Debug("oldState WindowMaximized: "+QString::number(stateEvent->oldState()));
            //QMainWindow::resize(_appSize);
            //QMainWindow::move(_appPos.x() < 10 ? 10 : _appPos.x(), _appPos.y() < 10 ? 10 : _appPos.y());
        }
        else if(stateEvent->oldState() == Qt::WindowState::WindowFullScreen && !_isMaximized)
        {
            LogHandler::Debug("WindowFullScreen to normal: "+QString::number(stateEvent->oldState()));
            QTimer::singleShot(200, [this]{
                QMainWindow::resize(_appSize);
                QMainWindow::move(_appPos.x() < 10 ? 10 : _appPos.x(), _appPos.y() < 10 ? 10 : _appPos.y());
               // videoHandler->resize(_videoSize);
            });
        }
        else if(stateEvent->oldState() == Qt::WindowState::WindowMaximized && !_isFullScreen && !QMainWindow::isMaximized())
        {
            LogHandler::Debug("WindowMaximized to normal");

            _isMaximized = false;
        }
    }
}
QPoint _mainStackedWidgetPos;
void MainWindow::toggleFullScreen()
{
    videoHandler->toggleFullscreen();
//    if(!_isFullScreen)
//    {
//        videoHandler->showFullScreen();
//        LogHandler::Debug("Before full VideoSize: width: "+QString::number(videoHandler->size().width()) + " height: " + QString::number(videoHandler->size().height()));
//        QScreen *screen = this->window()->windowHandle()->screen();
//        QSize screenSize = screen->size();
//        _videoSize = videoHandler->size();
//        _appSize = this->size();
//        _appPos = this->pos();
//        _mainStackedWidgetPos = ui->mainStackedWidget->pos();
//        _isMaximized = this->isMaximized();
//        _isFullScreen = true;
//        //QMainWindow::setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
//        _mediaGrid->removeWidget(videoHandler);
////        _controlsHomePlaceHolderGrid->removeWidget(_playerControlsFrame);

////        placeHolderControlsGrid = new QGridLayout(this);
////        placeHolderControlsGrid->setContentsMargins(0,0,0,0);
////        placeHolderControlsGrid->setSpacing(0);
////        placeHolderControlsGrid->addWidget(_playerControlsFrame);

////        playerControlsPlaceHolder = new QFrame(this);
////        playerControlsPlaceHolder->setLayout(placeHolderControlsGrid);
////        playerControlsPlaceHolder->setContentsMargins(0,0,0,0);
////        playerControlsPlaceHolder->installEventFilter(this);
////        playerControlsPlaceHolder->move(QPoint(0, screenSize.height() - _playerControlsFrame->height()));
////        playerControlsPlaceHolder->setFixedWidth(screenSize.width());
////        playerControlsPlaceHolder->setFixedHeight(_playerControlsFrame->height());
////        playerControlsPlaceHolder->setFocusPolicy(Qt::StrongFocus);
//        int rows = screenSize.height() / _playerControlsFrame->height();
//        ui->fullScreenGrid->addWidget(videoHandler, 0, 0, rows, 5);
////        ui->fullScreenGrid->addWidget(playerControlsPlaceHolder, rows - 1, 0, 1, 5);

////        if(libraryWindow == nullptr || libraryWindow->isHidden())
////        {
////            libraryOverlay = true;
////            placeHolderLibraryGrid = new QGridLayout(this);
////            placeHolderLibraryGrid->setContentsMargins(0,0,0,0);
////            placeHolderLibraryGrid->setSpacing(0);
////            placeHolderLibraryGrid->addWidget(libraryList);

////            playerLibraryPlaceHolder = new QFrame(this);
////            playerLibraryPlaceHolder->setLayout(placeHolderLibraryGrid);
////            playerLibraryPlaceHolder->setContentsMargins(0,0,0,0);
////            playerLibraryPlaceHolder->installEventFilter(this);
////            playerLibraryPlaceHolder->move(QPoint(0, screenSize.height()));
////            playerLibraryPlaceHolder->setFixedWidth(ui->libraryFrame->width());
////            playerLibraryPlaceHolder->setFixedHeight(screenSize.height() - _playerControlsFrame->height());
////            ui->fullScreenGrid->addWidget(playerLibraryPlaceHolder, 0, 0, rows - 1, 2);
////            libraryList->setProperty("cssClass", "fullScreenLibrary");
////            _playerControlsFrame->style()->unpolish(libraryList);
////            _playerControlsFrame->style()->polish(libraryList);
////            hideLibrary();
////        }

////        _playerControlsFrame->setProperty("cssClass", "fullScreenControls");
////        _playerControlsFrame->style()->unpolish(_playerControlsFrame);
////        _playerControlsFrame->style()->polish(_playerControlsFrame);
//        ui->mainStackedWidget->setCurrentIndex(1);
//        QMainWindow::centralWidget()->layout()->setMargin(0);
//        QMainWindow::showFullScreen();
//        LogHandler::Debug("After full VideoSize: width: "+QString::number(videoHandler->size().width()) + " height: " + QString::number(videoHandler->size().height()));
//        videoHandler->layout()->setMargin(0);
//        ////ui->mainStackedWidget->move(QPoint(0, 0));
//        //hideControls();
//        ui->menubar->hide();
//        ui->statusbar->hide();
//        QMainWindow::setFocus();
//    }
//    else
//    {
//        ui->mainStackedWidget->setCurrentIndex(0);
//        LogHandler::Debug("Before Normal VideoSize: width: "+QString::number(videoHandler->size().width()) + " height: " + QString::number(videoHandler->size().height()));
//        ui->fullScreenGrid->removeWidget(videoHandler);
//        _mediaGrid->addWidget(videoHandler, 0, 0, 3, 5);
////        playerControlsPlaceHolder->layout()->removeWidget(_playerControlsFrame);
////        ui->fullScreenGrid->removeWidget(playerControlsPlaceHolder);
////        _playerControlsFrame->setWindowFlags(Qt::Widget);
////        _controlsHomePlaceHolderGrid->addWidget(_playerControlsFrame);
////        _playerControlsFrame->setProperty("cssClass", "windowedControls");
////        _playerControlsFrame->style()->unpolish(_playerControlsFrame);
////        _playerControlsFrame->style()->polish(_playerControlsFrame);
////        libraryList->setProperty("cssClass", "windowedLibrary");
////        libraryList->style()->unpolish(libraryList);
////        libraryList->style()->polish(libraryList);

////        if(libraryOverlay)
////        {
////            placeHolderLibraryGrid->removeWidget(libraryList);
////            ui->fullScreenGrid->removeWidget(playerLibraryPlaceHolder);
////            libraryList->setMinimumSize(QSize(0, 0));
////            libraryList->setMaximumSize(QSize(16777215, 16777215));
////            ui->libraryGrid->addWidget(libraryList, 0, 0, 20, 12);
////            windowedLibraryButton->raise();
////            randomizeLibraryButton->raise();
////            libraryOverlay = false;
////            delete placeHolderLibraryGrid;
////            delete playerLibraryPlaceHolder;
////        }

//        videoHandler->layout()->setMargin(9);
//        QMainWindow::centralWidget()->layout()->setMargin(9);


//        if(_isMaximized)
//        {
//            QMainWindow::showMaximized();
//        }
//        else
//        {
//            QMainWindow::showNormal();
//        }
//        LogHandler::Debug("After Normal VideoSize: width: "+QString::number(videoHandler->size().width()) + " height: " + QString::number(videoHandler->size().height()));
//        ui->menubar->show();
//        ui->statusbar->show();
//        //_playerControlsFrame->show();
//        //libraryList->show();
//        _isFullScreen = false;
//        //QMainWindow::setWindowFlags(Qt::WindowFlags());
////        delete placeHolderControlsGrid;
////        delete playerControlsPlaceHolder;
//    }
}

void MainWindow::hideControls()
{
    if (_isFullScreen)
    {
        _playerControlsFrame->hide();
    }
}

void MainWindow::showControls()
{
    if (_isFullScreen)
    {
        _playerControlsFrame->show();
    }
}

void MainWindow::hideLibrary()
{
    if (_isFullScreen)
    {
        libraryList->hide();
    }
}

void MainWindow::showLibrary()
{
    if (_isFullScreen)
    {
        libraryList->show();
    }
}

void MainWindow::on_VolumeSlider_valueChanged(int value)
{
    videoHandler->setVolume(value);
    SettingsHandler::setPlayerVolume(value);
}

void MainWindow::on_PlayBtn_clicked()
{
    if (libraryList->count() > 0)
    {
        LibraryListItem27* selectedItem;
//        if(libraryList->selectedItems().length() == 0)
//        {
//            selectedItem = setCurrentLibraryRow(0);
//        }
//        else
//        {
//            selectedItem = libraryList->selectedItem();
//        }
        LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
        if(selectedFileListItem.type == LibraryListItemType::PlaylistInternal)
        {
            loadPlaylistIntoLibrary(selectedFileListItem.nameNoExtension, true);
        }
        else if((videoHandler->isPlaying() && selectedFileListItem.path != videoHandler->file()) || (!videoHandler->isPlaying() && !xtEngine.syncHandler()->isPlayingStandAlone()))
        {
            stopAndPlayMedia(selectedFileListItem);
        }
        else if(videoHandler->isPaused() || videoHandler->isPlaying())
        {
            videoHandler->togglePause();
            if(xtEngine.syncHandler()->isPlayingStandAlone())
            {
                xtEngine.syncHandler()->togglePause();
            }
        }
        else if(xtEngine.syncHandler()->isPlayingStandAlone())
        {
            xtEngine.syncHandler()->togglePause();
        }
    }
}

LibraryListItem27 MainWindow::setCurrentLibraryRow(int row)
{
    if(libraryList->count() > 0)
    {
        libraryList->setCurrentRow(row);
        auto item = libraryList->item(row);
        on_LibraryList_itemClicked(libraryList->itemIndex(row));
        return item;
    }
    return LibraryListItem27();
}

void MainWindow::on_togglePaused(bool paused)
{
    _playerControlsFrame->setPlayIcon(!paused);
    if(paused)
        deviceSwitchedHome();
}

void MainWindow::stopMedia()
{
    if(videoHandler->isPlaying())
    {
        videoHandler->stop();
    }
    if(xtEngine.syncHandler()->isPlayingStandAlone())
    {
        xtEngine.syncHandler()->stopStandAloneFunscript();
    }
}

void MainWindow::on_MuteBtn_toggled(bool checked)
{
    videoHandler->toggleMute();
}

void MainWindow::on_fullScreenBtn_clicked()
{
    toggleFullScreen();
}

void MainWindow::on_seekslider_hover(int position, qint64 sliderValue)
{
    //qint64 sliderValueTime = XMath::mapRange(static_cast<qint64>(sliderValue), (qint64)0, (qint64)100, (qint64)0, videoHandler->duration());
//    if (!videoPreviewWidget)
//        videoPreviewWidget = new VideoPreviewWidget();
//        LogHandler::Debug("sliderValue: "+QString::number(sliderValue));
//        LogHandler::Debug("time: "+QString::number(sliderValueTime));
    //LogHandler::Debug("position: "+QString::number(position));
    QPoint gpos;
    if(_isFullScreen)
    {
        gpos = mapToGlobal(playerControlsPlaceHolder->pos() + _playerControlsFrame->getTimeSliderPosition() + QPoint(position, 0));
        QToolTip::showText(gpos, QTime(0, 0, 0).addMSecs(sliderValue).toString(QString::fromLatin1("HH:mm:ss")));
    }
    else
    {
        auto tootipPos = mapToGlobal(QPoint(ui->medialAndControlsFrame->pos().x(), 0) + _controlsHomePlaceHolderFrame->pos() + _playerControlsFrame->getTimeSliderPosition() + QPoint(position, 0));
        QToolTip::showText(tootipPos, QTime(0, 0, 0).addMSecs(sliderValue).toString(QString::fromLatin1("HH:mm:ss")));
        gpos = QPoint(ui->medialAndControlsFrame->pos().x(), 0) + _controlsHomePlaceHolderFrame->pos() + _playerControlsFrame->getTimeSliderPosition() + QPoint(position, 0);
    }

//        LogHandler::Debug("medialAndControlsFrame x: " + QString::number(ui->medialAndControlsFrame->pos().x()));
//        LogHandler::Debug("SeekSlider x: " + QString::number(ui->SeekSlider->pos().x()));
//        LogHandler::Debug("SeekSlider y: " + QString::number(ui->SeekSlider->pos().y()));
//        LogHandler::Debug("controlsHomePlaceHolder x: " + QString::number(ui->controlsHomePlaceHolder->pos().x()));
//        LogHandler::Debug("controlsHomePlaceHolder y: " + QString::number(ui->controlsHomePlaceHolder->pos().y()));
//        LogHandler::Debug("gpos x: " + QString::number(gpos.x()));
//        LogHandler::Debug("gpos y: " + QString::number(gpos.y()));
//        LogHandler::Debug("gpos - QPoint(176/2, 250) x: " + QString::number((gpos - QPoint(176/2, 250)).x()));
//        LogHandler::Debug("gpos - QPoint(176/2, 250) y: " + QString::number((gpos - QPoint(176/2, 250)).y()));

    //    if (!Config::instance().previewEnabled())
    //        return;

    if(!XTPSettings::getDisableTimeLinePreview() && playingLibraryListItem.type == LibraryListItemType::Video && !_playerControlsFrame->getTimeLineMousePressed() && (videoHandler->isPlaying() || videoHandler->isPaused()))
    {
        //const int w = Config::instance().previewWidth();
        //const int h = Config::instance().previewHeight();
        //videoPreviewWidget->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        _videoPreviewWidget->setTimestamp(sliderValue);
        _videoPreviewWidget->preview(gpos);
        //videoPreviewWidget->raise();
        //videoPreviewWidget->activateWindow();
    }
    else if(_videoPreviewWidget && _videoPreviewWidget->isVisible() && _playerControlsFrame->getTimeLineMousePressed()) {
        _videoPreviewWidget->close();
    }
}

void MainWindow::on_seekslider_leave()
{
    if (!_videoPreviewWidget || !_videoPreviewWidget->isVisible())
    {
        return;
    }
    _videoPreviewWidget->close();
//    delete videoPreviewWidget;
//    videoPreviewWidget = NULL;
}

void MainWindow::on_timeline_currentTimeMove(qint64 position)
{
    LogHandler::Debug("position: "+ QString::number(position));
    bool isStandAloneFunscriptPlaying = playingLibraryListItem.type == LibraryListItemType::FunscriptType;
    //qint64 duration = isStandAloneFunscriptPlaying ? xtEngine.syncHandler()->getFunscriptMax() : videoHandler->duration();

    LogHandler::Debug("playerPosition: "+ QString::number(position));
    if(position <= 0)
        position = 50;
    isStandAloneFunscriptPlaying ? xtEngine.syncHandler()->setFunscriptTime(position) : videoHandler->setPosition(position);
}


void MainWindow::onLoopRange_valueChanged(qint64 currentTime, qint64 startLoop, qint64 endLoop)
{
    bool isStandAloneFunscriptPlaying = playingLibraryListItem.type == LibraryListItemType::FunscriptType;
    //qint64 duration = isStandAloneFunscriptPlaying ? xtEngine.syncHandler()->getFunscriptMax() : videoHandler->duration();
    qint64 mediaPosition = isStandAloneFunscriptPlaying ? xtEngine.syncHandler()->getFunscriptTime() : videoHandler->position();

    //qint64 currentVideoPositionPercentage = XMath::mapRange(mediaPosition,  (qint64)0, duration, (qint64)0, (qint64)100);
    //qint64 destinationVideoPosition = XMath::mapRange((qint64)position, (qint64)0, (qint64)100,  (qint64)0, duration);

    //_playerControlsFrame->setSeekSliderToolTip(currentTime);

    if(endLoop > 0) {
        if(mediaPosition < startLoop || mediaPosition >= endLoop)
        {
            isStandAloneFunscriptPlaying ? xtEngine.syncHandler()->setFunscriptTime(startLoop) : videoHandler->setPosition(startLoop);
        }
    }
//    else if (mediaPosition >= endLoop)
//    {
//        //qint64 startLoopVideoPosition = XMath::mapRange((qint64)startLoop, (qint64)0, (qint64)100,  (qint64)0, duration);
//        if(startLoop <= 0)
//            startLoop = 50;
//        if (mediaPosition != startLoop)
//            isStandAloneFunscriptPlaying ? xtEngine.syncHandler()->setFunscriptTime(startLoop) : videoHandler->setPosition(startLoop);
//    }
}

void MainWindow::on_media_positionChanged(qint64 position)
{
    bool isStandAloneFunscriptPlaying = playingLibraryListItem.type == LibraryListItemType::FunscriptType;
    qint64 duration = isStandAloneFunscriptPlaying ? xtEngine.syncHandler()->getFunscriptMax() : videoHandler->duration();
   // qint64 videoToSliderPosition = XMath::mapRange(position,  (qint64)0, duration, (qint64)0, (qint64)100);
//    if (!_playerControlsFrame->getAutoLoop())
//    {
//        if (duration > 0)
//        {
//            _playerControlsFrame->setTime(position);
//        }
//    }
    if(_playerControlsFrame->getAutoLoop())
    {
        qint64 endLoopToVideoPosition = _playerControlsFrame->getEndLoop();
        //qint64 endLoopToVideoPosition = XMath::mapRange((qint64)endLoop, (qint64)0, (qint64)100,  (qint64)0, duration);
        if (position >= endLoopToVideoPosition || (endLoopToVideoPosition == duration && position >= duration - 2000))
        {
            qint64 startLoopVideoPosition = _playerControlsFrame->getStartLoop();
            //qint64 startLoopVideoPosition = XMath::mapRange((qint64)startLoop, (qint64)0, (qint64)100,  (qint64)0, duration);
            if (position != startLoopVideoPosition)
                isStandAloneFunscriptPlaying ? xtEngine.syncHandler()->setFunscriptTime(startLoopVideoPosition) : videoHandler->seek(startLoopVideoPosition);

//TODO create a marker that doesnt intrude on the viewer experience.
//        QPoint gpos;
//        qint64 videoToSliderPosition = XMath::mapRange(position,  (qint64)0, duration, (qint64)0, (qint64)100);
//        int hoverposition = XMath::mapRange((int)videoToSliderPosition,  (int)0, (int)100, (int)0, _playerControlsFrame->getSeekSliderWidth()) - 15;
//        if(_isFullScreen)
//        {
//            gpos = mapToGlobal(playerControlsPlaceHolder->pos() + _playerControlsFrame->getTimeSliderPosition() + QPoint(hoverposition, 0));
//            QToolTip::showText(gpos, QTime(0, 0, 0).addMSecs(position).toString(QString::fromLatin1("HH:mm:ss")), this);
//        }
//        else
//        {
//            auto tootipPos = mapToGlobal(QPoint(ui->medialAndControlsFrame->pos().x(), 0) + _controlsHomePlaceHolderFrame->pos() + _playerControlsFrame->getTimeSliderPosition() + QPoint(hoverposition, 0));
//            QToolTip::showText(tootipPos, QTime(0, 0, 0).addMSecs(position).toString(QString::fromLatin1("HH:mm:ss")), this);
//            gpos = QPoint(ui->medialAndControlsFrame->pos().x(), 0) + _controlsHomePlaceHolderFrame->pos() + _playerControlsFrame->getTimeSliderPosition() + QPoint(hoverposition, 0);
//        }
        }
    }
    _playerControlsFrame->setTimeDuration(position, duration);
    //    QString timeCurrent = QTime(0, 0, 0).addMSecs(position).toString(QString::fromLatin1("HH:mm:ss"));
    //    QString timeDuration = QTime(0, 0, 0).addMSecs(duration).toString(QString::fromLatin1("HH:mm:ss"));
    //    QString timeStamp = timeCurrent.append("/").append(timeDuration);
    //    ui->lblCurrentDuration->setText(timeStamp);
}


void MainWindow::on_standaloneFunscript_start()
{
    LogHandler::Debug("Enter on_standaloneFunscript_start");
//    if(xtEngine.connectionHandler()->getSelectedInputDevice())
//        xtEngine.connectionHandler()->getSelectedInputDevice()->dispose();
    videoHandler->setLoading(false);
    _playerControlsFrame->resetMediaControlStatus(true);
}

void MainWindow::on_standaloneFunscript_stop()
{
    LogHandler::Debug("Enter on_standaloneFunscript_stop");
    videoHandler->setLoading(false);
    _playerControlsFrame->resetMediaControlStatus(false);
}

void MainWindow::on_media_start()
{
    LogHandler::Debug("Enter on_media_start");
    if(xtEngine.connectionHandler()->getSelectedInputDevice())
        xtEngine.connectionHandler()->getSelectedInputDevice()->dispose();
    xtEngine.syncHandler()->on_other_media_state_change(XMediaState::Playing);
    if (xtEngine.syncHandler()->isLoaded())
    {
        xtEngine.syncHandler()->syncOtherMediaFunscript([this] () -> qint64 { return videoHandler->position(); });
    }
    videoHandler->setLoading(false);
    _playerControlsFrame->resetMediaControlStatus(true);
    _mediaStopped = false;
}

void MainWindow::on_media_stop()
{
    LogHandler::Debug("Enter on_media_stop");
    if(!playingLibraryListItem.ID.isEmpty())
        updateMetaData(playingLibraryListItem);
    xtEngine.syncHandler()->on_other_media_state_change(XMediaState::Stopped);
    videoHandler->setLoading(false);
    _playerControlsFrame->resetMediaControlStatus(false);
    xtEngine.syncHandler()->stopOtherMediaFunscript();
    _mediaStopped = true;
}

void MainWindow::on_scriptNotFound(QString message)
{
    xtEngine.syncHandler()->reset();
    if(!SettingsHandler::getDisableNoScriptFound())
        NoMatchingScriptDialog::show(this, message);
}

//void MainWindow::on_noScriptsFound(QString message)
//{
//    xtEngine.syncHandler()->reset();
//    if(!SettingsHandler::getDisableNoScriptFound())
//        DialogHandler::MessageBox(this, message, XLogLevel::Critical);
//}

void MainWindow::onFunscriptSearchResult(QString mediaPath, QString funscriptPath, qint64 mediaDuration)
{
//    if(xtEngine.syncHandler()->isPlaying())
//        return;

    if (!funscriptFileSelectorOpen && xtEngine.connectionHandler()->isOutputDeviceConnected())
    {
        bool saveLinkedScript = false;

        //If the above locations fail ask the user to select a file manually.
        if (funscriptPath.isEmpty())
        {
            if(!SettingsHandler::getDisableVRScriptSelect())
            {
                LogHandler::Debug("onFunscriptSearchResult Enter no scripts found. Ask user");
                onText_to_speech("Script for video playing in VR not found. Please check your computer to select a script.");
                funscriptFileSelectorOpen = true;
                funscriptPath = QFileDialog::getOpenFileName(this, "Choose script for video: " + mediaPath, SettingsHandler::getLastSelectedLibrary(), "Script Files (*.funscript);;Zip (*.zip)");
                funscriptFileSelectorOpen = false;
                saveLinkedScript = true;
                //LogHandler::Debug("funscriptPath: "+funscriptPath);
            }
            if(funscriptPath.isEmpty())
            {
                LogHandler::Debug("Funscript selector canceled");
                vrScriptSelectorCanceled = true;
            }
        }

        if(!funscriptPath.isEmpty())
        {
            LogHandler::Debug("Starting sync: "+funscriptPath);
            auto fileName = QUrl(mediaPath).fileName();
            auto itemRef = xtEngine.mediaLibraryHandler()->findItemByName(QUrl(mediaPath).fileName());
            if(!itemRef) {
                LogHandler::Error("NO vr item found in media library");
                processVRMetaData(mediaPath, funscriptPath, mediaDuration);
            } else {
                playingLibraryListItem = LibraryListItem27(itemRef);
            }
            xtEngine.syncHandler()->syncInputDeviceFunscript(funscriptPath);
            if(saveLinkedScript)
            {
                LogHandler::Debug("Saving script into data: "+funscriptPath);
                //Store the location of the file so the user doesnt have to select it again.
                SettingsHandler::setLinkedVRFunscript(mediaPath, funscriptPath);
                SettingsHandler::SaveLinkedFunscripts();
            }
        }
    }
}

void MainWindow::processVRMetaData(QString videoPath, QString funscriptPath, qint64 duration)
{
    QFileInfo videoFile(videoPath);
    QString fileNameTemp = videoFile.fileName();
    QString videoPathTemp = videoFile.fileName();
    QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));
    QString scriptNoExtension = videoPathTemp.remove(videoPathTemp.lastIndexOf('.'), videoPathTemp.length() - 1);
    QString zipFile;
    if(funscriptPath.endsWith(".zip"))
        zipFile = funscriptPath;
    LibraryListItem27 vrItem ;
    vrItem.type = LibraryListItemType::VR;
    vrItem.path = videoPath; // path
    vrItem.name = videoFile.fileName(); // name
    vrItem.nameNoExtension = scriptNoExtension; //nameNoExtension
    vrItem.script = funscriptPath; // script
    vrItem.scriptNoExtension = scriptNoExtension;
    vrItem.mediaExtension = mediaExtension;
    vrItem.zipFile = zipFile;
    vrItem.modifiedDate = videoFile.birthTime().isValid() ? videoFile.birthTime().date() : videoFile.created().date();
    vrItem.duration = (unsigned)duration;
    xtEngine.mediaLibraryHandler()->setLiveProperties(vrItem);
    //playingLibraryListItem = new LibraryListWidgetItem(item);

    auto itemRef = xtEngine.mediaLibraryHandler()->findItemByName(QUrl(videoPath).fileName());
    processMetaData(itemRef ? itemRef : vrItem);
    playingLibraryListItem = itemRef ? itemRef : vrItem;
}

void MainWindow::on_sendTCode(QString value)
{
    if(xtEngine.connectionHandler()->isOutputDeviceConnected())
    {
        if(SettingsHandler::getFunscriptLoaded( TCodeChannelLookup::Stroke()) &&
                xtEngine.syncHandler()->isPlaying() &&
                ((videoHandler->isPlaying() && !videoHandler->isPaused())
                    || (xtEngine.connectionHandler()->getSelectedInputDevice() && xtEngine.connectionHandler()->getSelectedInputDevice()->isPlaying())))
        {
            QRegularExpression rx("L0[^\\s]*\\s?");
            value = value.remove(rx);
        }

        if((value.contains(TCodeChannelLookup::Suck()) && value.contains(TCodeChannelLookup::SuckPosition())))
        {
            QRegularExpression rx("A1[^\\s]*\\s?");
            value = value.remove(rx);
        }
        xtEngine.connectionHandler()->sendTCode(value);
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

void MainWindow::on_media_statusChanged(XMediaStatus status)
{
    switch (status) {
    case XMediaStatus::EndOfMedia:
        if (!_playerControlsFrame->getAutoLoop())
            skipForward();
    break;
    case XMediaStatus::NoMedia:
        //status = tr("No media");
        break;
    case XMediaStatus::InvalidMedia:
        //status = tr("Invalid meida");
        break;
    case XMediaStatus::BufferingMedia:
        videoHandler->setLoading(true);
        break;
    case XMediaStatus::BufferedMedia:
        videoHandler->setLoading(false);
        break;
    case XMediaStatus::LoadingMedia:
        videoHandler->setLoading(true);
        break;
    case XMediaStatus::LoadedMedia:
        videoHandler->setLoading(false);
        break;
    case XMediaStatus::StalledMedia:

        break;
    default:
        //status = QString();
        //onStopPlay();
        break;
    }
}

void MainWindow::skipForward()
{
    if (libraryList->count() > 0)
    {
        LibraryListItem27 libraryListItem;
        int index = libraryList->selectedRow() + 1;
        if(index < libraryList->count())
        {
            libraryListItem = setCurrentLibraryRow(index);
        }
        else
        {
            libraryListItem = setCurrentLibraryRow(0);
        }

        if(libraryListItem.type == LibraryListItemType::PlaylistInternal || (SettingsHandler::getSkipPlayingStandAloneFunscriptsInLibrary() && libraryListItem.type == LibraryListItemType::FunscriptType))
        {
            skipForward();
        }
        else
            stopAndPlayMedia(libraryListItem);
    }
}

void MainWindow::skipBack()
{
    if (libraryList->count() > 0)
    {
        if(!videoHandler->isPlaying() || videoHandler->position() < 5000)
        {
            LibraryListItem27 libraryListItem;
            int index = libraryList->selectedRow() - 1;
            if(index >= 0)
            {
                libraryListItem = setCurrentLibraryRow(index);
            }
            else
            {
                libraryListItem = setCurrentLibraryRow(libraryList->count() - 1);
            }

            if(libraryListItem.type == LibraryListItemType::PlaylistInternal)
            {
                skipBack();
            }
            else
                stopAndPlayMedia(libraryListItem);
        }
        else
            videoHandler->seek(50);
    }
}

void MainWindow::rewind()
{
    qint64 position = videoHandler->position();
    qint64 videoIncrement = SettingsHandler::getVideoIncrement() * 1000;
    if (position > videoIncrement)
        if(xtEngine.syncHandler()->isPlayingStandAlone())
            xtEngine.syncHandler()->setFunscriptTime(xtEngine.syncHandler()->getFunscriptTime() - videoIncrement);
        else
            videoHandler->seek(videoHandler->position() - videoIncrement);
    else
        skipBack();
}

void MainWindow::fastForward()
{
    qint64 position = videoHandler->position();
    qint64 videoIncrement = SettingsHandler::getVideoIncrement() * 1000;
    if (position < videoHandler->duration() - videoIncrement)
        if(xtEngine.syncHandler()->isPlayingStandAlone())
            xtEngine.syncHandler()->setFunscriptTime(xtEngine.syncHandler()->getFunscriptTime() + videoIncrement);
        else
            videoHandler->seek(videoHandler->position() + videoIncrement);
    else
        skipForward();
}

void MainWindow::media_double_click_event(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton )
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

void MainWindow::on_output_device_connectionChanged(ConnectionChangedSignal event)
{
    auto selectedOutputDevice = xtEngine.connectionHandler()->getSelectedOutputDevice();
    if(event.type == DeviceType::Output && (!selectedOutputDevice || selectedOutputDevice->name() == event.deviceName))
    {
        xtEngine.syncHandler()->on_output_device_change(selectedOutputDevice);
        deviceConnected = event.status == ConnectionStatus::Connected;
        if(deviceConnected)
        {
            connectionStatusLabel->setProperty("cssClass", "connectionStatusConnected");
            deviceHome();
        }
        else if(event.status == ConnectionStatus::Connecting)
            connectionStatusLabel->setProperty("cssClass", "connectionStatusConnecting");
        QString message = "";
        if (event.deviceName == DeviceName::Serial)
        {
            message += "Serial: ";
        }
        else if(event.deviceName == DeviceName::Network)
        {
            message += "Network: ";
        }
        message += " " + event.message;
        connectionStatusLabel->setText(message);
        if(event.status == ConnectionStatus::Error || event.status == ConnectionStatus::Disconnected)
        {
            retryConnectionButton->show();
            connectionStatusLabel->setProperty("cssClass", "connectionStatusDisconnected");
        }
        else
        {
            retryConnectionButton->hide();
        }
        connectionStatusLabel->style()->unpolish(connectionStatusLabel);
        connectionStatusLabel->style()->polish(connectionStatusLabel);
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


void MainWindow::on_input_device_connectionChanged(ConnectionChangedSignal event)
{
    auto selectedInputDevice = xtEngine.connectionHandler()->getSelectedInputDevice();
    if(event.type == DeviceType::Input && (!selectedInputDevice || selectedInputDevice->name() == event.deviceName))
    {
        xtEngine.syncHandler()->on_input_device_change(selectedInputDevice);
        QString message = "";
        if(event.deviceName == DeviceName::None) {
            ui->actionChange_current_deo_script->setEnabled(false);
            vrConnectionStatusLabel->hide();
            vrRetryConnectionButton->hide();
        }
        else if(event.deviceName == DeviceName::XTPWeb)
            message += "XTP Web: ";
        else
            message += event.deviceName == DeviceName::Whirligig ? "Whirligig: " : "HereSphere: ";
        message += " " + event.message;
        vrConnectionStatusLabel->setText(message);

        if(event.status == ConnectionStatus::Error) {
            DialogHandler::MessageBox(this, "Input connection error: "+event.message, XLogLevel::Critical);
        } else if(event.status == ConnectionStatus::Connected)
        {
            ui->actionChange_current_deo_script->setEnabled(event.deviceName != DeviceName::XTPWeb);
            vrRetryConnectionButton->hide();
            stopMedia();
            xtEngine.syncHandler()->stopAll();

        }
        else if(event.status == ConnectionStatus::Connecting)
        {
            vrConnectionStatusLabel->show();
        }
        else
        {
            ui->actionChange_current_deo_script->setEnabled(false);
            vrConnectionStatusLabel->hide();
            vrRetryConnectionButton->hide();
        }
    }
}

void MainWindow::on_actionAbout_triggered()
{
    DialogHandler::ShowAboutDialog(this, XTPSettings::XTPVersionTimeStamp, SettingsHandler::XTEVersionTimeStamp, TCodeChannelLookup::getSelectedTCodeVersionName());
}

void MainWindow::on_actionDonate_triggered()
{
    QDesktopServices::openUrl(QUrl(QString::fromLatin1("https://www.patreon.com/Khrull")));
}

void MainWindow::on_actionSettings_triggered()
{
    _xSettings->initLive();
    _xSettings->show();
}

void MainWindow::on_actionThumbnail_triggered()
{
    SettingsHandler::setLibraryView(LibraryView::Thumb);
    changeLibraryDisplayMode(LibraryView::Thumb);
}

void MainWindow::on_actionList_triggered()
{
    SettingsHandler::setLibraryView(LibraryView::List);
    changeLibraryDisplayMode(LibraryView::List);
}

void MainWindow::changeLibraryDisplayMode(LibraryView value)
{
    LibraryView converted = LibraryView::Thumb;
    if(libraryList->viewMode() == QListView::ViewMode::ListMode) {
        converted = LibraryView::List;
    }

    if(converted != value)
    {
        switch(value)
        {
            case LibraryView::List:
                libraryList->setResizeMode(QListView::Fixed);
                libraryList->setFlow(QListView::TopToBottom);
                libraryList->setViewMode(QListView::ListMode);
                libraryList->setSpacing(0);
            break;
            case LibraryView::Thumb:
                libraryList->setResizeMode(QListView::Adjust);
                libraryList->setFlow(QListView::LeftToRight);
                libraryList->setViewMode(QListView::IconMode);
                libraryList->setSpacing(2);
            break;
        }

        libraryList->updateGeometries();
        _librarySortFilterProxyModel->setLibraryViewMode(value);
        _playListViewModel->setLibraryViewMode(value);
        setThumbSize(SettingsHandler::getThumbSize());
    }
//    if(selectedPlaylistItems.length() > 0)
//    {
//        libraryList->setDragEnabled(true);
//        libraryList->setDragDropMode(QAbstractItemView::DragDrop);
//        libraryList->setDefaultDropAction(Qt::MoveAction);
//        libraryList->setMovement(QListView::Movement::Snap);
//    }
}

void MainWindow::updateThumbSizeUI(int size)
{
    switch(size)
    {
        case 75:
            action75_Size->setChecked(true);
        break;
        case 100:
            action100_Size->setChecked(true);
        break;
        case 125:
            action125_Size->setChecked(true);
        break;
        case 150:
            action150_Size->setChecked(true);
        break;
        case 175:
            action175_Size->setChecked(true);
        break;
        case 200:
            action200_Size->setChecked(true);
        break;
        default:
            actionCustom_Size->setChecked(true);
        break;
    }
}

void MainWindow::on_action75_triggered()
{
    SettingsHandler::setThumbSize(75);
    setThumbSize(75);
}

void MainWindow::on_action100_triggered()
{
    SettingsHandler::setThumbSize(100);
    setThumbSize(100);
}

void MainWindow::on_action125_triggered()
{
    SettingsHandler::setThumbSize(125);
    setThumbSize(125);
}

void MainWindow::on_action150_triggered()
{
    SettingsHandler::setThumbSize(150);
    setThumbSize(150);
}

void MainWindow::on_action175_triggered()
{
    SettingsHandler::setThumbSize(175);
    setThumbSize(175);
}

void MainWindow::on_action200_triggered()
{
    SettingsHandler::setThumbSize(200);
    setThumbSize(200);
}

void MainWindow::on_actionCustom_triggered()
{
    bool ok;
    int size = QInputDialog::getInt(this, tr("Custom size"), "Size (Max:"+QString::number(SettingsHandler::getMaxThumbnailSize().height()) + ")",
                                         SettingsHandler::getThumbSize(), 1, SettingsHandler::getMaxThumbnailSize().height(), 50, &ok);
    if (ok && size > 0)
    {
        SettingsHandler::setThumbSize(size);
        setThumbSize(size);
    }
}

void MainWindow::setThumbSize(int size)
{
    //resizeThumbs(size);
    ImageFactory::clearCache();
    videoHandler->setMinimumHeight(size);
    videoHandler->setMinimumWidth(size);


//    if(SettingsHandler::getLibraryView() == LibraryView::List)
//        libraryList->setViewMode(QListView::ListMode);
//    else
//        libraryList->setViewMode(QListView::IconMode);

}
bool MainWindow::isLibraryLoading()
{
    return xtEngine.mediaLibraryHandler()->isLibraryLoading() || loadingLibraryFuture.isRunning();
}
void MainWindow::resizeThumbs(int size)
{
//    if(!isLibraryLoading())
//    {
//        toggleLibraryLoading(true);
//        onLibraryLoadingStatusChange("Resizing thumbs...");
//        loadingLibraryFuture = QtConcurrent::run([this, size]() {
//            QSize newSize = {size, size};
//            for(int i = 0; i < libraryList->count(); i++)
//            {
//                if(loadingLibraryStop)
//                    return;
//                ((LibraryListWidgetItem*)libraryList->item(i))->updateThumbSize(newSize);
//            }
//            emit libraryIconResized(newSize);
//        });
//    }
}

//void MainWindow::libraryListSetIconSize(QSize newSize)
//{
//    libraryList->setIconSize(newSize);
//    toggleLibraryLoading(false);
//}

void MainWindow::updateLibrarySortUI(LibrarySortMode mode)
{
    switch(mode)
    {
        case LibrarySortMode::NAME_ASC:
            actionNameAsc_Sort->setChecked(true);
        break;
        case LibrarySortMode::NAME_DESC:
            actionNameDesc_Sort->setChecked(true);
        break;
        case LibrarySortMode::CREATED_ASC:
            actionCreatedAsc_Sort->setChecked(true);
        break;
        case LibrarySortMode::CREATED_DESC:
            actionCreatedDesc_Sort->setChecked(true);
        break;
        case LibrarySortMode::RANDOM:
            actionRandom_Sort->setChecked(true);
        break;
        case LibrarySortMode::TYPE_ASC:
            actionTypeAsc_Sort->setChecked(true);
        break;
        case LibrarySortMode::TYPE_DESC:
            actionTypeDesc_Sort->setChecked(true);
        break;
    }
}

void MainWindow::on_actionNameAsc_triggered()
{
    SettingsHandler::setLibrarySortMode(LibrarySortMode::NAME_ASC);
    sortLibraryList(LibrarySortMode::NAME_ASC);
}
void MainWindow::on_actionNameDesc_triggered()
{
    SettingsHandler::setLibrarySortMode(LibrarySortMode::NAME_DESC);
    sortLibraryList(LibrarySortMode::NAME_DESC);
}
void MainWindow::on_actionRandom_triggered()
{
    SettingsHandler::setLibrarySortMode(LibrarySortMode::RANDOM);
    sortLibraryList(LibrarySortMode::RANDOM);
}
void MainWindow::on_actionCreatedAsc_triggered()
{
    SettingsHandler::setLibrarySortMode(LibrarySortMode::CREATED_ASC);
    sortLibraryList(LibrarySortMode::CREATED_ASC);
}
void MainWindow::on_actionCreatedDesc_triggered()
{
    SettingsHandler::setLibrarySortMode(LibrarySortMode::CREATED_DESC);
    sortLibraryList(LibrarySortMode::CREATED_DESC);
}
void MainWindow::on_actionTypeAsc_triggered()
{
    SettingsHandler::setLibrarySortMode(LibrarySortMode::TYPE_ASC);
    sortLibraryList(LibrarySortMode::TYPE_ASC);
}
void MainWindow::on_actionTypeDesc_triggered()
{
    SettingsHandler::setLibrarySortMode(LibrarySortMode::TYPE_DESC);
    sortLibraryList(LibrarySortMode::TYPE_DESC);
}

void MainWindow::sortLibraryList(LibrarySortMode sortMode)
{
    if(sortMode == LibrarySortMode::RANDOM)
    {
//        if(!isLibraryLoading())
//        {
//            toggleLibraryLoading(true);
//            onLibraryLoadingStatusChange("Randomizing...");
//            //libraryList->clear();

//            loadingLibraryFuture = QtConcurrent::run([this]() {
//                //Fisher and Yates algorithm
//                auto cachedItems = xtEngine.mediaLibraryHandler()->getLibraryCache();
//                int n = cachedItems.count() - 1;

//                QList<LibraryListItem27> randomized;
//                int index_arr[n];
//                int index;

//                for (int i = 0; i < n; i++)
//                {
//                    if(loadingLibraryStop)
//                        return;
//                    index_arr[i] = 0;
//                }

//                for (int i = 0; i < n; i++)
//                {
//                    if(loadingLibraryStop)
//                        return;
//                    do
//                    {
//                        if(loadingLibraryStop)
//                            return;
//                        index = XMath::rand(0, n);
//                    }
//                    while (index_arr[index] != 0);
//                    index_arr[index] = 1;
//                    auto item = cachedItems.value(index);
//                    if((item.type == LibraryListItemType::VR && SettingsHandler::getShowVRInLibraryView()) ||
//                        (item.type != LibraryListItemType::VR && item.type != LibraryListItemType::PlaylistInternal))
//                        randomized.push_back(cachedItems.value(index));
//                }
//                _playListViewModel->populate(randomized);
//                libraryList->setModel(_playListViewModel);
//                emit randomizeComplete();
//            });
//        }
    }

//    if(sortMode != LibrarySortMode::NONE)
//    {
//        if(libraryList->model() == _playListViewModel)
//            libraryList->setModel(_librarySortFilterProxyModel);
//    }

    _librarySortFilterProxyModel->setSortMode(sortMode);
    if(sortMode != LibrarySortMode::RANDOM) {
        randomizeLibraryButton->hide();
    } else if(!isPlaylistMode()) {
        randomizeLibraryButton->show();
    }
    toggleLibraryLoading(false);
    setCurrentLibraryRow(0);
}

//void MainWindow::onRandomizeComplete() {
    //randomizeLibraryButton->show();
    //toggleLibraryLoading(false);
    //setCurrentLibraryRow(0);
//}

void MainWindow::on_actionChange_theme_triggered()
{
    QFileInfo selectedThemeInfo(XTPSettings::getSelectedTheme());
    QString selectedTheme = QFileDialog::getOpenFileName(this, "Choose XTP theme", selectedThemeInfo.absoluteDir().absolutePath(), "CSS Files (*.css)");
    if(!selectedTheme.isEmpty())
    {
        XTPSettings::setSelectedTheme(selectedTheme);
        loadTheme(selectedTheme);
    }
}

void MainWindow::loadTheme(QString cssFilePath)
{
    QFile file(cssFilePath);
    if(file.exists())
    {
        file.open(QFile::ReadOnly);
        QString styleSheet = QLatin1String(file.readAll());
        if(!styleSheet.isEmpty())
            setStyleSheet(styleSheet);
        else
            setStyleSheet("");
    }
    else
        setStyleSheet("");
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
    xtEngine.syncHandler()->setStandAloneLoop(false);
    if (checked)
    {
        connect(_playerControlsFrame, &PlayerControls::loopRangeChanged, this, &MainWindow::onLoopRange_valueChanged);
        videoHandler->setRepeat(-1);
        auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(playingLibraryListItem.path);
        if(libraryListItemMetaData.lastLoopStart > -1 && libraryListItemMetaData.lastLoopEnd > libraryListItemMetaData.lastLoopStart)
        {
            QTimer::singleShot(250, this, [this, libraryListItemMetaData]() {
                _playerControlsFrame->setStartLoop(libraryListItemMetaData.lastLoopStart);
                _playerControlsFrame->setEndLoop(libraryListItemMetaData.lastLoopEnd);
            });
//            qint64 sliderToVideoPosition = XMath::mapRange(libraryListItemMetaData.lastLoopStart,  (qint64)0, (qint64)100, (qint64)0, videoHandler->duration());
//            videoHandler->setPosition(sliderToVideoPosition +100);
        }
        else
        {
            //qint64 videoToSliderPosition = XMath::mapRange(videoHandler->position(),  (qint64)0, videoHandler->duration(), (qint64)0, (qint64)100);
            updateMetaData(playingLibraryListItem);
            _playerControlsFrame->setStartLoop(videoHandler->position());
        }
        //_playerControlsFrame->setLoopMinimumRange(1000);
    }
    else
    {
        on_media_positionChanged(videoHandler->position());
        disconnect(_playerControlsFrame, &PlayerControls::loopRangeChanged, this, &MainWindow::onLoopRange_valueChanged);
        qint64 position = videoHandler->position();
        videoHandler->setRepeat();
        videoHandler->setPosition(position);
        //_playerControlsFrame->setLoopMinimumRange(0);
    }
}

QString MainWindow::getPlaylistName(bool newPlaylist)
{
    bool ok;
    QString playlistName = nullptr;
    if(newPlaylist)
        playlistName = PlaylistDialog::getNewPlaylist(this, &ok);
    else
    {
        LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
        playlistName = PlaylistDialog::renamePlaylist(this, selectedFileListItem.nameNoExtension, &ok);
    }

    if(newPlaylist && ok)
    {
        auto item = xtEngine.mediaLibraryHandler()->findItemByNameNoExtension(playlistName);
        if(!item)
        {
            SettingsHandler::addNewPlaylist(playlistName);
            xtEngine.mediaLibraryHandler()->setupPlaylistItem(playlistName);
        }
        else
        {
            DialogHandler::MessageBox(this, (tr("Playlist '") + playlistName + tr("' already exists.\nPlease choose another name.")), XLogLevel::Critical);
        }
    }
    return ok ? playlistName : nullptr;
}

void MainWindow::addSelectedLibraryItemToPlaylist(QString playlistName, LibraryListItem27 item)
{
    SettingsHandler::addToPlaylist(playlistName, item);
}
bool MainWindow::isPlaylistMode() {
    return !selectedPlaylistName.isEmpty();
}
void MainWindow::loadPlaylistIntoLibrary(QString playlistName, bool autoPlay)
{
    if(!thumbProcessIsRunning)
    {
            if(isLibraryLoading())
            {
                xtEngine.mediaLibraryHandler()->stopLibraryLoading();
            }
            toggleLibraryLoading(true);
            onLibraryLoadingStatusChange("Loading playlist...");

            loadingLibraryFuture = QtConcurrent::run([this, playlistName, autoPlay]() {
                selectedPlaylistName = playlistName;
                auto playlist = xtEngine.mediaLibraryHandler()->getPlaylist(selectedPlaylistName);
                _playListViewModel->populate(playlist);
                libraryList->setModel(_playListViewModel);
                if(autoPlay)
                {
                    emit playlistLoaded(playlist.first());
                }
                else
                {
                    emit playlistLoaded();
                }
            });
    }
    else
        DialogHandler::MessageBox(this, tr("Please wait for thumbnails to fully load!"), XLogLevel::Warning);
}
void MainWindow::onPlaylistLoaded(LibraryListItem27 autoPlayItem) {
    backLibraryButton->show();
    editPlaylistButton->show();
    librarySortGroup->setEnabled(false);
    libraryFilterLineEdit->hide();
    randomizeLibraryButton->hide();
    //changeLibraryDisplayMode(SettingsHandler::getLibraryView());
    //resizeThumbs(SettingsHandler::getThumbSize());
    //sortLibraryList(LibrarySortMode::NONE);
    toggleLibraryLoading(false);
    setCurrentLibraryRow(0);
    if(!autoPlayItem.path.isEmpty())
    {
        emit stopAndPlayVideo(autoPlayItem);
    }
}
void MainWindow::backToMainLibrary()
{
    if(_editPlaylistMode)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, tr("WARNING!"), tr("You are currently editing a playlist. Cancel all changes?"),
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes)
            cancelEditPlaylist();
        else
            return;
    }
    selectedPlaylistName = nullptr;
//    if(cachedLibraryWidgetItems.length() == 0)
//        xtEngine.mediaLibraryHandler()->loadLibraryAsync();
//    else
//    {
        if(!isLibraryLoading())
        {
            //disconnect(libraryList, &QListWidget::itemChanged, 0, 0);
            toggleLibraryLoading(true);
            onLibraryLoadingStatusChange("Loading library...");
            loadingLibraryFuture = QtConcurrent::run([this]() {
                _playListViewModel->dePopulate();
                libraryList->setModel(_librarySortFilterProxyModel);
                emit backFromPlaylistLoaded();
            });
        }
    //}
}

void MainWindow::onBackFromPlaylistLoaded() {
    backLibraryButton->hide();
    editPlaylistButton->hide();
    savePlaylistButton->hide();
    cancelEditPlaylistButton->hide();
    libraryFilterLineEdit->show();
    if(SettingsHandler::getLibrarySortMode() == LibrarySortMode::RANDOM)
        randomizeLibraryButton->show();
    libraryList->setDragEnabled(false);
    //sortLibraryList(SettingsHandler::getLibrarySortMode());
    toggleLibraryLoading(false);
    setCurrentLibraryRow(0);
}

void MainWindow::savePlaylist()
{
    QScroller::grabGesture(libraryList->viewport(), QScroller::LeftMouseButtonGesture);
    QList<LibraryListItem27> libraryItems = _playListViewModel->getPopulated();
    SettingsHandler::updatePlaylist(selectedPlaylistName, libraryItems);
    savePlaylistButton->hide();
    editPlaylistButton->show();
    cancelEditPlaylistButton->hide();
    _editPlaylistMode = false;
    changeLibraryDisplayMode(SettingsHandler::getLibraryView());
    _playListViewModel->clearOverRideThumbSize();
}
void MainWindow::editPlaylist()
{
    QScroller::grabGesture(libraryList->viewport(), QScroller::MiddleMouseButtonGesture);
    _editPlaylistMode = true;
    savePlaylistButton->show();
    cancelEditPlaylistButton->show();
    editPlaylistButton->hide();
//    if(SettingsHandler::getThumbSize() > 75)
//    {
//        changeLibraryDisplayMode(LibraryView::List);
//        //_playListViewModel->overRideThumbSize(75);
//    }
//    else
//    {
        changeLibraryDisplayMode(LibraryView::List);
    //}
    _playListViewModel->setDragEnabled(true);
    libraryList->setDragEnabled(true);
    libraryList->setDragDropMode(QAbstractItemView::InternalMove);
    libraryList->setDefaultDropAction(Qt::MoveAction);
    libraryList->setMovement(QListWidget::Movement::Snap);
    libraryList->setDragDropOverwriteMode(false);
    libraryList->setDropIndicatorShown(true);

}
void MainWindow::cancelEditPlaylist()
{
    _editPlaylistMode = false;
    QScroller::grabGesture(libraryList->viewport(), QScroller::LeftMouseButtonGesture);
    savePlaylistButton->hide();
    cancelEditPlaylistButton->hide();
    editPlaylistButton->show();
    changeLibraryDisplayMode(SettingsHandler::getLibraryView());
    _playListViewModel->clearOverRideThumbSize();
    _playListViewModel->setDragEnabled(false);
    libraryList->setDragEnabled(false);
    libraryList->setMovement(QListWidget::Movement::Static);
    auto playlist = xtEngine.mediaLibraryHandler()->getPlaylist(selectedPlaylistName);
    _playListViewModel->populate(playlist);
}

void MainWindow::removeFromPlaylist()
{
    auto selectedItem = libraryList->selectedItem();
    _playListViewModel->removeItem(selectedItem);
    savePlaylistButton->show();
    editPlaylistButton->hide();
    cancelEditPlaylistButton->show();
}
void MainWindow::renamePlaylist()
{
    QString renamedPlaylistName = getPlaylistName(false);
    if(renamedPlaylistName != nullptr)
    {
        LibraryListItem27 playlist = libraryList->selectedItem();
        if(!playlist.ID.isEmpty())
        {
            auto playlists = SettingsHandler::getPlaylists();
            auto storedPlaylist = playlists.value(playlist.nameNoExtension);
            deleteSelectedPlaylist();
            SettingsHandler::updatePlaylist(renamedPlaylistName, storedPlaylist);
            xtEngine.mediaLibraryHandler()->setupPlaylistItem(renamedPlaylistName);
        }
        else if(playlist.nameNoExtension != renamedPlaylistName)
        {
            DialogHandler::MessageBox(this, (tr("Playlist '") + renamedPlaylistName + tr("' already exists.\nPlease choose another name.")), XLogLevel::Critical);
        }
    }

}
void MainWindow::deleteSelectedPlaylist()
{
    LibraryListItem27 selectedFileListItem = libraryList->selectedItem();
    SettingsHandler::deletePlaylist(selectedFileListItem.nameNoExtension);

    xtEngine.mediaLibraryHandler()->removeFromCache(selectedFileListItem);
}

void MainWindow::showInGraphicalShell(QString path)
{
    const QFileInfo fileInfo(path);
    // Mac, Window, linux support folder or file.
#if defined(Q_OS_WIN)
    QStringList args;
    if (!fileInfo.isDir())
        args << "/select,";
    args << QDir::toNativeSeparators(path);
    if (QProcess::startDetached("explorer", args))
        return;
#elif defined(Q_OS_MAC)
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
               << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                                     .arg(fileInfo.canonicalFilePath());
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
               << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
#elif defined(Q_OS_LINUX)
    // we cannot select a file here, because no file browser really supports it...
    const QString folder = fileInfo.isDir() ? fileInfo.absoluteFilePath() : fileInfo.filePath();
     QDesktopServices::openUrl(fileInfo.path());
//    QProcess browserProc;
//    bool success = browserProc.startDetached("gtk-launch \"$(xdg-mime query default inode/directory)\"", QStringList() << folder );
//    const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
//    success = success && error.isEmpty();
//    if (!success)
//        DialogHandler::Dialog(error, XLogLevel::Critical);
#endif
}

void MainWindow::onSetMoneyShot(LibraryListItem27 libraryListItem, qint64 currentPosition, bool userSet)
{
    auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(libraryListItem.path);
    if(!userSet && libraryListItemMetaData.moneyShotMillis > 0)
        return;
    libraryListItemMetaData.moneyShotMillis = currentPosition;
    SettingsHandler::updateLibraryListItemMetaData(libraryListItemMetaData);
}
void MainWindow::onAddBookmark(LibraryListItem27 libraryListItem, QString name, qint64 currentPosition)
{
    auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(libraryListItem.path);
    libraryListItemMetaData.bookmarks.append({name, currentPosition});
    SettingsHandler::updateLibraryListItemMetaData(libraryListItemMetaData);
}

void MainWindow::skipToMoneyShot()
{
    xtEngine.syncHandler()->skipToMoneyShot();
    if(SettingsHandler::getEnableHttpServer())
        xtEngine.httpHandler()->sendWebSocketTextMessage("skipToMoneyShot");

    if(SettingsHandler::getSkipToMoneyShotPlaysFunscript() && !SettingsHandler::getSkipToMoneyShotSkipsVideo())
        return;
    if(videoHandler->isPlaying() ||  (xtEngine.connectionHandler()->getSelectedInputDevice() && xtEngine.connectionHandler()->getSelectedInputDevice()->isPlaying()))
    {
        if(videoHandler->isPlaying()) {
            if(_playerControlsFrame->getAutoLoop())
                _playerControlsFrame->SetLoop(false);
            LibraryListItem27 selectedLibraryListItem27 = playingLibraryListItem;
            auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(selectedLibraryListItem27.path);
            if (libraryListItemMetaData.moneyShotMillis > -1 && libraryListItemMetaData.moneyShotMillis < videoHandler->duration())
            {
                videoHandler->setPosition(libraryListItemMetaData.moneyShotMillis);
            }
            else
            {
                videoHandler->setPosition(videoHandler->duration() - (videoHandler->duration() * .1));
            }
        } else if(xtEngine.connectionHandler()->getSelectedInputDevice() && xtEngine.connectionHandler()->getSelectedInputDevice()->isPlaying()) {
//            InputDevicePacket currentPacket = xtEngine.connectionHandler()->getSelectedInputDevice()->getCurrentPacket();
//            auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(currentPacket.path);
//            InputDevicePacket packet =
//            {
//                nullptr,
//                0,
//                libraryListItemMetaData.moneyShotMillis,
//                1,
//                0
//           };
//            if (libraryListItemMetaData.moneyShotMillis > -1 && libraryListItemMetaData.moneyShotMillis < currentPacket.duration)
//            {
//                xtEngine.connectionHandler()->getSelectedInputDevice()->sendPacket(packet);
//            }
//            else
//            {
//                packet.currentTime = currentPacket.duration - (currentPacket.duration * .1);
//                xtEngine.connectionHandler()->getSelectedInputDevice()->sendPacket(packet);
//            }
        }
    }
}

void MainWindow::skipToNextAction()
{
    if(xtEngine.syncHandler()->isLoaded())
    {
        if(_playerControlsFrame->getAutoLoop())
            _playerControlsFrame->SetLoop(false);
        qint64 nextActionMillis = xtEngine.syncHandler()->getFunscriptNext();
        if(nextActionMillis > 1500)
        {
            if(videoHandler->isPlaying())
            {
                videoHandler->setPosition(nextActionMillis - 1000);
            }
            else if(xtEngine.syncHandler()->isPlayingStandAlone())
            {
                xtEngine.syncHandler()->setFunscriptTime(nextActionMillis - 1000);
            }
            else
            {
                if(SettingsHandler::getEnableHttpServer())
                    xtEngine.httpHandler()->sendWebSocketTextMessage("skipToNextAction", QString::number((nextActionMillis / 1000) - 1, 'f', 1));
            }
        }
    }
}

void MainWindow::on_actionReload_theme_triggered()
{
    loadTheme(XTPSettings::getSelectedTheme());
}

void MainWindow::on_actionStored_DLNA_links_triggered()
{
    _dlnaScriptLinksDialog->showDialog();
}

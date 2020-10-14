#ifndef SETTINGSHANDLER_H
#define SETTINGSHANDLER_H
#include <QSettings>
#include <QMutex>
#include <QHash>
#include "../lookup/enum.h"
#include "../lookup/AxisNames.h"
#include "../lookup/GamepadAxisNames.h"
#include "../lookup/MediaActions.h"
#include "../tool/xmath.h"
#include "../struct/ChannelModel.h"
#include "loghandler.h"
#include "devicehandler.h"

class SettingsHandler
{
public:
    static const QString TCodeVersion;
    static const QString XTPVersion;
    static const float XTPVersionNum;
    static QString getDeoDnlaFunscript(QString key);
    static QString getSelectedTheme();
    static QString getSelectedLibrary();
    static QString getSelectedFunscriptLibrary();
    static int getSelectedDevice();
    static QString getSerialPort();
    static QString getServerAddress();
    static QString getServerPort();
    static int getPlayerVolume();
    static int getoffSet();

    static int getChannelUserMin(QString channel);
    static int getChannelUserMax(QString channel);
    static void setChannelUserMin(QString channel, int value);
    static void setChannelUserMax(QString channel, int value);

    static int getLibraryView();
    static int getThumbSize();
    static int getThumbSizeList();
    static int getVideoIncrement();

    static bool getGamepadEnabled();
    static QMap<QString, QString> getGamePadMap();
    static QString getGamePadMapButton(QString gamepadButton);
    static QMap<QString, ChannelModel> getAvailableAxis();
    static ChannelModel getAxis(QString axis);
    static bool getInverseTcXL0();
    static bool getInverseTcXRollR2();
    static bool getInverseTcYRollR1();
    static int getGamepadSpeed();
    static int getLiveGamepadSpeed();
    static int getGamepadSpeedIncrement();

    static bool getDisableSpeechToText();
    static void setDisableSpeechToText(bool value);

    static void setDeoDnlaFunscript(QString key, QString value);
    static void setSelectedTheme(QString value);
    static void setSelectedLibrary(QString value);
    static void setSelectedFunscriptLibrary(QString value);
    static void setSelectedDevice(int value);
    static void setSerialPort(QString value);
    static void setServerAddress(QString value);
    static void setServerPort(QString value);

    static QString getDeoAddress();
    static QString getDeoPort();
    static bool getDeoEnabled();
    static void setDeoAddress(QString value);
    static void setDeoPort(QString value);
    static void setDeoEnabled(bool value);

    static QString getWhirligigAddress();
    static QString getWhirligigPort();
    static bool getWhirligigEnabled();
    static void setWhirligigAddress(QString value);
    static void setWhirligigPort(QString value);
    static void setWhirligigEnabled(bool value);

    static void setPlayerVolume(int value);
    static void setoffSet(int value);

    static float getMultiplierValue(QString channel);
    static bool getMultiplierChecked(QString channel);
    static void setMultiplierChecked(QString channel, bool value);
    static void setMultiplierValue(QString channel, float value);

    static void setLibraryView(int value);
    static void setThumbSize(int value);
    static void setThumbSizeList(int value);
    static void setVideoIncrement(int value);

    static void setGamepadEnabled(bool value);
    static void setGamePadMapButton(QString gamePadButton, QString axis);
    static void setAxis(QString axis, ChannelModel channel);
    static void setInverseTcXL0(bool value);
    static void setInverseTcXRollR2(bool value);
    static void setInverseTcYRollR1(bool value);
    static void setGamepadSpeed(int value);
    static void setGamepadSpeedStep(int value);
    static void setLiveGamepadSpeed(int value);

    static void setXRangeStep(int value);
    static int getXRangeStep();


    static void setLiveXRangeMin(int value);
    static void setLiveXRangeMax(int value);
    static int getLiveXRangeMin();
    static int getLiveXRangeMax();
    static void resetLiveXRange();
    static bool getMultiplierEnabled();
    static void setMultiplierEnabled(bool value);
    static void setLiveMultiplierEnabled(bool value);
    static bool getLiveGamepadConnected();
    static void setLiveGamepadConnected(bool value);

    static void Load();
    static void Save();
    static void Default();
    static void Clear();

private:
    SettingsHandler();
    ~SettingsHandler();
    static void SetMapDefaults();
    static void SetupAvailableAxis();
    static void SetupGamepadButtonMap();
    static void MigrateTo17();
    static GamepadAxisNames gamepadAxisNames;
    static AxisNames axisNames;
    static MediaActions mediaActions;

    static QHash<QString, QVariant> deoDnlaFunscriptLookup;
    static QString selectedTheme;
    static QString selectedLibrary;
    static QString selectedFunscriptLibrary;
    static QString selectedFile;
    static int selectedDevice;
    static QString serialPort;
    static QString serverAddress;
    static QString serverPort;
    static QString deoAddress;
    static QString deoPort;
    static bool deoEnabled;
    static QString whirligigAddress;
    static QString whirligigPort;
    static bool whirligigEnabled;
    static int playerVolume;
    static int offSet;

    static bool yMultiplierChecked;
    static float yMultiplierValue;
    static bool yRollMultiplierChecked;
    static float yRollMultiplierValue;
    static bool xRollMultiplierChecked;
    static float xRollMultiplierValue;
    static bool zMultiplierChecked;
    static float zMultiplierValue;
    static bool twistMultiplierChecked;
    static float twistMultiplierValue;
    static bool vibMultiplierChecked;
    static float vibMultiplierValue;

    static bool _gamePadEnabled;
    static QMap<QString, QString> _gamepadButtonMap;
    static QMap<QString, ChannelModel> _availableAxis;
    static bool _inverseTcXL0;
    static bool _inverseTcXRollR2;
    static bool _inverseTcYRollR1;
    static int _gamepadSpeed;
    static int _gamepadSpeedStep;
    static int _liveGamepadSpeed;
    static bool _liveGamepadConnected;

    static int _xRangeStep;
    static int _liveXRangeMax;
    static int _liveXRangeMin;
    static bool _liveMultiplierEnabled;
    static bool _multiplierEnabled;

    static int libraryView;
    static int thumbSize;
    static int thumbSizeList;
    static int videoIncrement;

    static bool disableSpeechToText;
    static bool defaultReset;

    static QSettings* settings;
    static QMutex mutex;
};

#endif // SETTINGSHANDLER_H

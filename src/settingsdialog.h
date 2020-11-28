#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include<ui_settings.h>
#include <QDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QSpinBox>
#include "CustomControls/rangeslider.h"
#include "lib/struct/SerialComboboxItem.h"
#include "lib/struct/channeltableviewmodel.h"
#include "lib/struct/DecoderModel.h"
#include "lib/lookup/MediaActions.h"
#include "lib/handler/settingshandler.h"
#include "lib/handler/serialhandler.h"
#include "lib/handler/videohandler.h"
#include "lib/handler/udphandler.h"
#include "lib/handler/deohandler.h"
#include "lib/handler/whirligighandler.h"
#include "lib/handler/gamepadhandler.h"
#include "lib/handler/funscripthandler.h"
#include "addchanneldialog.h"

extern void initSerial(SerialHandler* serialHandler, SerialComboboxItem serialInfo);
extern void initNetwork(UdpHandler* serialHandler, NetworkAddress address);
extern void initDeo(DeoHandler* deoHandler, NetworkAddress address);

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

    void init(VideoHandler* videoHandler);
    void initLive();
    UdpHandler* getNetworkHandler();
    SerialHandler* getSerialHandler();
    DeoHandler* getDeoHandler();
    WhirligigHandler* getWhirligigHandler();
    GamepadHandler* getGamepadHandler();
    void setSelectedDeviceHandler(DeviceHandler* device);
    DeviceHandler* getSelectedDeviceHandler();
    bool isConnected();
    void initDeviceRetry();
    void initDeoRetry();
    void dispose();

signals:
    void deviceError(QString error);
    void deviceConnectionChange(ConnectionChangedSignal event);
    void deoDeviceError(QString error);
    void deoDeviceConnectionChange(ConnectionChangedSignal event);
    void whirligigDeviceError(QString error);
    void whirligigDeviceConnectionChange(ConnectionChangedSignal event);
    void gamepadConnectionChange(ConnectionChangedSignal event);

private slots:
    void on_serialOutputRdo_clicked();

    void on_networkOutputRdo_clicked();

    void on_serialRefreshBtn_clicked();

    void onRange_valueChanged(QString name, int value);
    void onRange_mouseRelease(QString name);
    void onOffSet_valueChanged(int value);
    void on_device_connectionChanged(ConnectionChangedSignal event);
    void on_device_error(QString error);
    void on_deo_connectionChanged(ConnectionChangedSignal event);
    void on_deo_error(QString error);
    void on_whirligig_connectionChanged(ConnectionChangedSignal event);
    void on_whirligig_error(QString error);
    void on_gamepad_connectionChanged(ConnectionChangedSignal event);

    void on_SerialOutputCmb_currentIndexChanged(int index);

    void on_networkAddressTxt_editingFinished();

    void on_networkPortTxt_editingFinished();

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_xRollMultiplierCheckBox_clicked();

    void on_xRollMultiplierSpinBox_valueChanged(double arg1);

    void on_yRollMultiplierCheckBox_clicked();

    void on_yRollMultiplierSpinBox_valueChanged(double arg1);

    void on_twistMultiplierCheckBox_clicked();

    void on_twistMultiplierSpinBox_valueChanged(double arg1);

    void on_vibMultiplierSpinBox_valueChanged(double arg1);

    void on_serialConnectButton_clicked();

    void on_networkConnectButton_clicked();

    void on_deoConnectButton_clicked();

    void on_deoAddressTxt_editingFinished();

    void on_deoPortTxt_editingFinished();

    void on_deoCheckbox_clicked(bool checked);

    void on_checkBox_clicked(bool checked);

    void on_resetAllButton_clicked();

    void on_gamePadCheckbox_clicked(bool checked);

    void on_videoIncrementSpinBox_valueChanged(int value);

    void on_inverseTcXL0_valueChanged(bool checked);
    void on_inverseTcXRollR2_valueChanged(bool checked);
    void on_inverseTcYRollR1_valueChanged(bool checked);
    void on_speedInput_valueChanged(int value);
    void on_speedIncrementInput_valueChanged(int value);
    void xRangeStepInput_valueChanged(int value);

    void on_disableTextToSpeechCheckBox_clicked(bool checked);

    void on_invertFunscriptXCheckBox_clicked(bool checked);

    void on_yMultiplierCheckBox_clicked(bool checked);

    void on_yMultiplierSpinBox_valueChanged(double arg1);

    void on_zMuliplierSpinBox_valueChanged(double arg1);

    void on_whirligigCheckBox_clicked(bool checked);

    void on_whirligigConnectButton_clicked();

    void on_enableMultiplierCheckbox_clicked(bool checked);
    void on_dialogButtonboxClicked(QAbstractButton* button);

    void on_channelAddButton_clicked();

    void on_channelDeleteButton_clicked();

    void on_axisDefaultButton_clicked();

    void on_suckMultiplierCheckBox_clicked(bool checked);

    void on_suckMultiplierSpinBox_valueChanged(double arg1);

    void on_zMultiplierCheckBox_clicked(bool checked);

    void on_vibMultiplierCheckBox_clicked(bool checked);

    void on_savePriorityButton_clicked();

    void on_cancelPriorityButton_clicked();

    void on_defaultPriorityButton_clicked();

private:
    Ui::SettingsDialog ui;
    void loadSerialPorts();
    void initSerialEvent();
    void initNetworkEvent();
    void initDeoEvent();
    void initWhirligigEvent();
    void setupUi();
    void setupGamepadMap();

    AxisNames axisNames;
    bool _interfaceInitialized = false;
    ConnectionStatus _outConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _deoConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _whirligigConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _gamepadConnectionStatus = ConnectionStatus::Disconnected;
    QList<SerialComboboxItem> serialPorts;
    SerialComboboxItem selectedSerialPort;
    DeviceHandler* selectedDeviceHandler;
    VideoHandler* _videoHandler;
    SerialHandler* _serialHandler;
    UdpHandler* _udpHandler;
    DeoHandler* _deoHandler;
    WhirligigHandler* _whirligigHandler;
    GamepadHandler* _gamepadHandler;
    QFuture<void> _initFuture;
    ChannelTableViewModel* channelTableViewModel;

    QMap<QString, QLabel*> rangeMinLabels;
    QMap<QString, QLabel*> rangeMaxLabels;
    QMap<QString, RangeSlider*> rangeSliders;

    QLabel* offSetLabel;
    QSpinBox* offSetSpinBox;

    void setDeviceStatusStyle(ConnectionStatus status, DeviceType deviceType, QString message = "");
};

#endif // SETTINGSDIALOG_H

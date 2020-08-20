#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H
#include <QVector>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTime>
#include <QMutex>
#include "../struct/SerialComboboxItem.h"
#include "../struct/ConnectionChangedSignal.h"
#include "../handler/settingshandler.h"
#include "../handler/loghandler.h"
#include "../lookup/enum.h"

class SerialHandler : public QObject
{

    Q_OBJECT

signals:
    void errorOccurred(QString error);
    void connectionChange(ConnectionChangedSignal status);

public:
    SerialHandler();
    ~SerialHandler();
    QVector<SerialComboboxItem> getPorts();
    void sendTCode(QString tcode);
    void init(SerialComboboxItem portInfo);
    void start();
    void stop();
    void dispose();

protected:
    void readData();
    void handleError(QSerialPort::SerialPortError error);

private:
    QString m_portName;
    QSerialPort* m_serial;
    static QMutex mutex;
};

#endif // SERIALHANDLER_H

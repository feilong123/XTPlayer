#include "udphandler.h"

UdpHandler::UdpHandler(QObject *parent) :
    QThread(parent)
{
}
UdpHandler::~UdpHandler()
{
    _mutex.lock();
    _stop = true;
    _cond.wakeOne();
    _mutex.unlock();
    wait();
}

void UdpHandler::init(NetworkAddress address, int waitTimeout)
{
    //qRegisterMetaType<ConnectionChangedSignal>();
    //qRegisterMetaType<QAbstractSocket::SocketState>();

    QMutexLocker locker(&_mutex);
    emit connectionChange({DeviceType::Network, ConnectionStatus::Connecting, "Connecting..."});
    _isSelected = true;
    _waitTimeout = waitTimeout;
    _address = address;
    _addressObj.setAddress(_address.address);
    locker.unlock();
    int timeouttracker = 0;
    while(!_isConnected && !_stop && timeouttracker <= 3)
    {
        sendTCode("D1");
        sleep(_waitTimeout / 1000 + 1);
        ++timeouttracker;
    }
}

void UdpHandler::sendTCode(QString tcode)
{
    const QMutexLocker locker(&_mutex);
    _tcode = tcode + "\n";
    LogHandler::Debug("Sending TCode UDP: "+tcode);
    if (!isRunning())
        start();
    else
        _cond.wakeOne();
}

void UdpHandler::run()
{
    bool currentAddressChanged = false;

    _mutex.lock();
    QString currentRequest = _tcode;
    _tcode = "";
    QByteArray data;
    data.append(currentRequest);

    QString currentAddress;
    int currentPort = 0;
    if (currentAddress != _address.address || currentPort != _address.port)
    {
        currentAddress = _address.address;
        currentPort = _address.port;
        currentAddressChanged = true;
        _isConnected = false;
    }

    _mutex.unlock();

    QScopedPointer<QUdpSocket> udpSocketSend(new QUdpSocket(this));
    udpSocketSend->connectToHost(_addressObj, _address.port);

    QScopedPointer<QUdpSocket> udpSocketRecieve(new QUdpSocket(this));
    if (!udpSocketRecieve->bind(QHostAddress::Any, 54000))
    {
        emit connectionChange({DeviceType::Network, ConnectionStatus::Error, "Error opening handshake"});
    }
    while (!_stop)
    {
        if(udpSocketSend->isWritable())
        {
            udpSocketSend->writeDatagram(data, _addressObj, _address.port);
            if (udpSocketSend->waitForBytesWritten(_waitTimeout))
            {
                if (currentAddressChanged)
                {
                    if(udpSocketRecieve->waitForReadyRead(_waitTimeout))
                    {
                        QNetworkDatagram datagram = udpSocketRecieve->receiveDatagram();

                        QString response = QString(datagram.data());
                        if (response.startsWith(SettingsHandler::TCodeVersion))
                        {
                            emit connectionChange({DeviceType::Network, ConnectionStatus::Connected, "Connected"});
                            _mutex.lock();
                            _isConnected = true;
                            _mutex.unlock();
                        }
                        else
                        {
                            emit connectionChange({DeviceType::Network, ConnectionStatus::Error, "No TCode"});
                        }
                    }
                }
            }
            else
            {
                emit connectionChange({DeviceType::Network, ConnectionStatus::Error, "Error sending"});
            }
        }


        _mutex.lock();
        if (currentAddress != _address.address || currentPort != _address.port || !_isConnected)
        {
            currentAddress = _address.address;
            currentPort = _address.port;
            currentAddressChanged = true;
            _isConnected = false;
        }

        QString currentRequest = _tcode;
        _tcode = "";
        data.clear();
        data.append(_tcode);
        _mutex.unlock();
    }
}

void UdpHandler::dispose()
{
    QMutexLocker locker(&_mutex);
    _isConnected = false;
    _stop = true;
    emit connectionChange({DeviceType::Network, ConnectionStatus::Disconnected, "Disconnected"});
}

bool UdpHandler::isConnected()
{
    QMutexLocker locker(&_mutex);
    return _isConnected;
}


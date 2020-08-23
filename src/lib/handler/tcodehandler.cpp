#include "tcodehandler.h"

TCodeHandler::TCodeHandler()
{

}

QString TCodeHandler::funscriptToTCode(qint64 position, int speed)
{
    QMutexLocker locker(&mutex);
    QString buffer = "";
    int tcodValue = XMath::mapRange(position, 50, 100, 500, 999);
    char tcodeValueString[4];
    sprintf(tcodeValueString, "%03d", calculateRange("L0", tcodValue));
    buffer += tcodeValueString;
    if (speed != 0) {//|| SettingsHandler::speed > 0
      //sprintf(speed_string, "%04d", speed != 0 ? speed : SettingsHandler::speed);
      buffer += "I";
      buffer += QString::number(speed);
    }
    return "L0" + buffer;
}

int TCodeHandler::calculateRange(const char* channel, int value)
{
    return XMath::constrain(value, getchannelMin(channel), getchannelMax(channel));
}

int TCodeHandler::getchannelMin(const char* channel)
{
    if (strcmp(channel, "L0") == 0)
    {
        return SettingsHandler::getXMin();
    }
    else if (strcmp(channel, "R1") == 0)
    {
        return SettingsHandler::getYRollMin();
    }
    else if (strcmp(channel, "R2") == 0)
    {
        return SettingsHandler::getXRollMin();
    }
    return 1;
}

int TCodeHandler::getchannelMax(const char* channel)
{
    if (strcmp(channel, "L0") == 0)
    {
        return SettingsHandler::getXMax();
    }
    else if (strcmp(channel, "R1") == 0)
    {
        return SettingsHandler::getYRollMax();
    }
    else if (strcmp(channel, "R2") == 0)
    {
        return SettingsHandler::getXRollMax();
    }
    return 1000;
}

QMutex TCodeHandler::mutex;

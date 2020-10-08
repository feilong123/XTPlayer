#include "funscripthandler.h"


Funscript* funscript = new Funscript();

FunscriptHandler::FunscriptHandler()
{
}
FunscriptHandler::~FunscriptHandler()
{
    delete(funscript);
}

void FunscriptHandler::load(QString funscriptString)
{
    QMutexLocker locker(&mutex);
    if (funscriptString == nullptr)
    {
        _loaded = false;
        return;
    }
    LogHandler::Debug("Funscript load: "+funscriptString);
    QFile loadFile(funscriptString);

    lastActionIndex = -1;
    nextActionIndex = 1;
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open funscript file.");
        _loaded = false;
        return;
    }

    //LogHandler::Debug("funscriptHandler->load "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
    QByteArray funData = loadFile.readAll();
    QJsonParseError* error = new QJsonParseError();
    QJsonDocument doc = QJsonDocument::fromJson(funData, error);

    if(doc.isNull()) {
        emit errorOccurred("loading funscript failed: " + error->errorString());
        delete error;
        _loaded = false;
        return;
    }
    delete error;
    JSonToFunscript(doc.object());

    _loaded = true;
}
Funscript* FunscriptHandler::currentFunscript()
{
    QMutexLocker locker(&mutex);
    return funscript;
}

bool FunscriptHandler::isLoaded()
{
    QMutexLocker locker(&mutex);
    return _loaded;
}
void FunscriptHandler::setLoaded(bool value)
{
    QMutexLocker locker(&mutex);
    _loaded = value;
}
void FunscriptHandler::JSonToFunscript(QJsonObject json)
{
    if (json.contains("range"))
        funscript->range = json["range"].toInt();
    if (json.contains("version") && json["version"].isString())
        funscript->version = json["version"].toString();
    if (json.contains("inverted") && json["inverted"].isBool())
    {
        _inverted = json["inverted"].toBool();
        funscript->inverted = json["inverted"].toBool();
    }
    if (json.contains("actions") && json["actions"].isArray())
    {
        funscript->actions.clear();
        QJsonArray actionArray = json["actions"].toArray();
        foreach(QJsonValue value, actionArray)
        {
            QJsonObject obj = value.toObject();
            if (obj.contains("at") && obj.contains("pos"))
            {
                funscript->actions[(qint64)obj["at"].toDouble()] = obj["pos"].toInt();
            }
        }
        posList = funscript->actions.keys();
        std::sort(posList.begin(), posList.end());
        n = posList.length() / sizeof(posList.first());
    }
    if (json.contains("creator") && json["creator"].isString())
        funscript->metadata.creator = json["creator"].toString();
    if (json.contains("original_name") && json["original_name"].isString())
        funscript->metadata.original_name = json["original_name"].toString();
    if (json.contains("url") && json["url"].isString())
        funscript->metadata.url = json["url"].toString();
    if (json.contains("url_video") && json["url_video"].isString())
        funscript->metadata.url_video = json["url_video"].toString();
    if (json.contains("tags") && json["tags"].isArray())
    {
        QJsonArray tags = json["tags"].toArray();;
        foreach(QJsonValue tag, tags)
            funscript->metadata.tags.append(tag.toString());
    }
    if (json.contains("performers") && json["performers"].isArray())
    {
        QJsonArray performers = json["performers"].toArray();;
        foreach(QJsonValue performer, performers)
            funscript->metadata.performers.append(performer.toString());
    }
    if (json.contains("paid") && json["paid"].isBool())
        funscript->metadata.paid = json["paid"].toBool();
    if (json.contains("comment") && json["comment"].isString())
        funscript->metadata.comment = json["comment"].toString();
    if (json.contains("original_total_duration_ms"))
        funscript->metadata.original_total_duration_ms = json["original_total_duration_ms"].toString().toLongLong();
}

bool FunscriptHandler::exists(QString path)
{
    QFile fpath(path);
    return fpath.exists();
}

std::unique_ptr<FunscriptAction> FunscriptHandler::getPosition(qint64 millis)
{
    QMutexLocker locker(&mutex);
    millis += SettingsHandler::getoffSet();
    qint64 closestMillis = findClosest(millis, posList);
    nextActionIndex = posList.indexOf(closestMillis) + 1;
    if(nextActionIndex >= posList.length())
    {
        return nullptr;
    }
    qint64 nextMillis = posList[nextActionIndex];
    //LogHandler::Debug("millis: "+ QString::number(millis));
    //LogHandler::Debug("closestMillis: "+ QString::number(closestMillis));
    //LogHandler::Debug("lastActionIndex: "+ QString::number(lastActionIndex));
    //LogHandler::Debug("nextActionIndex: "+ QString::number(nextActionIndex));
//    LogHandler::Debug("nextMillis: "+ QString::number(nextMillis));
    if ((lastActionIndex != nextActionIndex && millis >= closestMillis) ||lastActionIndex == -1)
    {
        int speed = lastActionIndex == -1 ? closestMillis : nextMillis - closestMillis;
        //LogHandler::Debug("offSet: "+ QString::number(SettingsHandler::getoffSet()));
        //LogHandler::Debug("speed: "+ QString::number(speed));
        //LogHandler::Debug("millis: "+ QString::number(millis));
//        LogHandler::Debug("closestMillis: "+ QString::number(closestMillis));
//        LogHandler::Debug("nextMillis: "+ QString::number(nextMillis));
//        LogHandler::Debug("lastActionIndex: "+ QString::number(lastActionIndex));
//        LogHandler::Debug("nextActionIndex: "+ QString::number(nextActionIndex));
//        LogHandler::Debug("nextActionPos: "+ QString::number(funscript->actions.value(nextMillis)));
        qint64 executionMillis = lastActionIndex == -1 ? closestMillis : nextMillis;
        std::unique_ptr<FunscriptAction> nextAction(new FunscriptAction { executionMillis, funscript->actions.value(executionMillis), speed });
        //LogHandler::Debug("nextAction.speed: "+ QString::number(nextAction->speed));
        lastActionIndex = nextActionIndex;
        return nextAction;
    }
    return nullptr;
}

qint64 FunscriptHandler::findClosest(qint64 value, QList<qint64> a) {

      if(value < a[0]) {
          return a[0];
      }
      if(value > a[a.length()-1]) {
          return a[a.length()-1];
      }

      int lo = 0;
      int hi = a.length() - 1;

      while (lo <= hi) {
          int mid = (hi + lo) / 2;

          if (value < a[mid]) {
              hi = mid - 1;
          } else if (value > a[mid]) {
              lo = mid + 1;
          } else {
              return a[mid];
          }
      }
      // lo == hi + 1
      return (a[lo] - value) < (value - a[hi]) ? a[lo] : a[hi];
  }

//static
bool FunscriptHandler::getInverted()
{
    QMutexLocker locker(&mutex);
    return _inverted;
}
void FunscriptHandler::setInverted(bool value)
{
    QMutexLocker locker(&mutex);
    _inverted = value;
}

bool FunscriptHandler::_inverted = false;
QMutex FunscriptHandler::mutex;

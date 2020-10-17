#include "librarylistwidgetitem.h"
#include <QApplication>
#include <QFileInfo>
#include "../handler/settingshandler.h"
#include "xmath.h"

LibraryListWidgetItem::LibraryListWidgetItem(LibraryListItem data) :
    QListWidgetItem(data.nameNoExtension)
{
    QFileInfo scriptInfo(data.script);
    if (!scriptInfo.exists())
    {
        setToolTip(data.path + "\nNo script file of the same name found.\nRight click and Play with funscript.");
        setForeground(QColorConstants::Gray);
    }
    else
    {
        setToolTip(data.path);
    }

    QFileInfo thumbInfo(data.thumbFile);
    QString thumbString = data.thumbFile;
    if (!thumbInfo.exists())
    {
        thumbString = QApplication::applicationDirPath() + "/themes/loading.png";
    }
    QIcon thumb;
    QPixmap bgPixmap(thumbString);
    QSize size = {SettingsHandler::getThumbSize(), SettingsHandler::getThumbSize()};
    QPixmap scaled = bgPixmap.scaled(size, Qt::AspectRatioMode::KeepAspectRatio);
    thumb.addPixmap(scaled);
    setIcon(thumb);
    setSizeHint(size);
    setText(data.nameNoExtension);
    QVariant listItem;
    listItem.setValue(data);
    setData(Qt::UserRole, listItem);
}


LibraryListItem LibraryListWidgetItem::getLibraryListItem()
{
    return data(Qt::UserRole).value<LibraryListItem>();
}

bool LibraryListWidgetItem::operator< (const QListWidgetItem & other) const
{
    LibraryListItem otherData = other.data(Qt::UserRole).value<LibraryListItem>();
    LibraryListItem thisData = data(Qt::UserRole).value<LibraryListItem>();
    switch(_sortMode)
    {
        case LibrarySortMode::RANDOM:
        {
//            qint64 randomValue = XMath::rand(0, 100);
//            if(randomValue > 50)
//                return thisData.modifiedDate < otherData.modifiedDate;
            return thisData.modifiedDate > otherData.modifiedDate;
        }
        case LibrarySortMode::CREATED_ASC:
        {
            return thisData.modifiedDate < otherData.modifiedDate;
//            auto d1 = thisData.modifiedDate;
//            auto d2 = otherData.modifiedDate;
//            if (d1.year() < d2.year())
//                return true;
//            if (d1.year() == d2.year() && d1.month() < d2.month())
//                return true;
//            if (d1.year() == d2.year() && d1.month() == d2.month() && d1.day() < d2.day())
//                return true;

//            return false;
        }
        case LibrarySortMode::CREATED_DESC:
        {
            return thisData.modifiedDate > otherData.modifiedDate;
//            auto d1 = thisData.modifiedDate;
//            auto d2 = otherData.modifiedDate;
//            if (d1.year() > d2.year())
//                return true;
//            if (d1.year() == d2.year() && d1.month() > d2.month())
//                return true;
//            if (d1.year() == d2.year() && d1.month() == d2.month() && d1.day() > d2.day())
//                return true;

//            return false;
        }
        case LibrarySortMode::NAME_DESC:
        {
            return thisData.name.localeAwareCompare(otherData.name) > 0;
        }
        case LibrarySortMode::NAME_ASC:
        {
            break;
        }
    }
    // otherwise just return the comparison result from the base class
    return QListWidgetItem::operator < (other);
}

void LibraryListWidgetItem::setSortMode(LibrarySortMode sortMode)
{
    _sortMode = sortMode;
}

LibrarySortMode LibraryListWidgetItem::_sortMode = LibrarySortMode::NAME_ASC;

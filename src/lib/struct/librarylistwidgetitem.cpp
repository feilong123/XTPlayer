#include "librarylistwidgetitem.h"

LibraryListWidgetItem::LibraryListWidgetItem(LibraryListItem27 &data, QListWidget* parent) :
    QListWidgetItem(data.nameNoExtension, parent)
{
    setToolTip(data.toolTip);
    if (data.isMFS)
    {
        setForeground(QColorConstants::Green);
        setText("(MFS) " + data.nameNoExtension);
    }
    else
        setText(data.nameNoExtension);
    QVariant listItem;

    listItem.setValue(data);
    setData(Qt::UserRole, listItem);
    int thumbSize = SettingsHandler::getThumbSize();
    _thumbSize = {thumbSize, thumbSize};

    QString thumbPath = data.thumbFile;
    QFileInfo thumbFile = QFileInfo(thumbPath);
    if(!thumbFile.exists())
        setThumbFileLoading(true);
    else
        setThumbFile(thumbPath);
}

LibraryListWidgetItem::~LibraryListWidgetItem()
{

}

LibraryListItem27 LibraryListWidgetItem::getLibraryListItem()
{
    return data(Qt::UserRole).value<LibraryListItem27>();
}

LibraryListItemType LibraryListWidgetItem::getType()
{
    return getLibraryListItem().type;
}

bool LibraryListWidgetItem::operator< (const QListWidgetItem & other) const
{
    LibraryListItem27 otherData = other.data(Qt::UserRole).value<LibraryListItem27>();
    LibraryListItem27 thisData = data(Qt::UserRole).value<LibraryListItem27>();

    if(_sortMode != NONE)
    {
        if(thisData.type == LibraryListItemType::PlaylistInternal && otherData.type != LibraryListItemType::PlaylistInternal)
        {
            return true;
        }
        else if(thisData.type != LibraryListItemType::PlaylistInternal && otherData.type == LibraryListItemType::PlaylistInternal)
        {
            return false;
        }
        else if(thisData.type == LibraryListItemType::PlaylistInternal && otherData.type == LibraryListItemType::PlaylistInternal)
        {
            return otherData.nameNoExtension.localeAwareCompare(thisData.nameNoExtension) > 0;
        }
        else if(_sortMode == LibrarySortMode::RANDOM)
        {
    //            qint64 randomValue = XMath::rand(0, 100);
    //            if(randomValue > 50)
    //                return thisData.modifiedDate < otherData.modifiedDate;
            return false;
        }
        else if(_sortMode == LibrarySortMode::CREATED_ASC)
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
        else if(_sortMode == LibrarySortMode::CREATED_DESC)
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
        else if(_sortMode == LibrarySortMode::NAME_DESC)
        {
            return thisData.name.localeAwareCompare(otherData.name) > 0;
        }
        else if(_sortMode == LibrarySortMode::NAME_ASC)
        {

        }
        else if(_sortMode == LibrarySortMode::TYPE_ASC)
        {
            if (thisData.type == otherData.type)
              return otherData.name.localeAwareCompare(thisData.name) > 0;
            return thisData.type < otherData.type;
        }
        else if(_sortMode == LibrarySortMode::TYPE_DESC)
        {
            if (thisData.type == otherData.type)
              return thisData.name.localeAwareCompare(otherData.name) > 0;
            return thisData.type > otherData.type;
        }
    }
    // otherwise just return the comparison result from the base class
    return QListWidgetItem::operator < (other);
}

bool LibraryListWidgetItem::operator == (const LibraryListWidgetItem & other) const
{
    return other.text() == text();
}

void LibraryListWidgetItem::setSortMode(LibrarySortMode sortMode)
{
    _sortMode = sortMode;
}

void LibraryListWidgetItem::updateThumbSize(QSize thumbSize)
{
    _thumbSize = thumbSize;
    QFileInfo thumbFile = QFileInfo(_thumbFile);
    QString thumbFilePath = _thumbFile;
    if(!thumbFile.exists())
    {
        thumbFilePath = "://images/icons/loading.png";
    }
    QPixmap bgPixmap = QPixmap(thumbFilePath);
    QIcon thumb;
    //QSize maxThumbSize = SettingsHandler::getMaxThumbnailSize();
    //int newHeight = round((float)bgPixmap.height() / bgPixmap.width() * 1080);
    //QSize newSize = calculateSize(thumbSize);
    QPixmap scaled = bgPixmap.scaled(thumbSize, Qt::AspectRatioMode::KeepAspectRatio);
    QSize maxHeight = calculateMaxSize(thumbSize);

    if(scaled.height() > maxHeight.height())
    {
        scaled = bgPixmap.scaled(maxHeight, Qt::AspectRatioMode::KeepAspectRatio);
//        QRect rect(0,0,scaled.width(), newHeight);
//        scaled = scaled.copy(rect);
    }
    thumb.addPixmap(scaled);
    setIcon(thumb);
    setSizeHint(thumbSize);
    setTextAlignment(Qt::AlignmentFlag::AlignTop | Qt::AlignmentFlag::AlignHCenter);
}

void LibraryListWidgetItem::setThumbFile(QString filePath)
{
    _thumbFile = filePath;
    auto data = getLibraryListItem();
    data.thumbFile = _thumbFile;
    QVariant listItem;
    listItem.setValue(data);
    setData(Qt::UserRole, listItem);
    updateThumbSize(_thumbSize);
}

void LibraryListWidgetItem::setThumbFileLoading(bool waiting)
{
    setThumbFile(waiting ? "://images/icons/loading.png" : "://images/icons/loading_current.png");
}

void LibraryListWidgetItem::setThumbFileLoaded(QString message, QString path)
{
    if(!path.isEmpty())
        _thumbFile = path;
    setThumbFile(!message.isEmpty() ? "://images/icons/error.png" : _thumbFile);
    if(!message.isEmpty())
        setToolTip(toolTip() + "\n"+ message);
}

LibraryListWidgetItem* LibraryListWidgetItem::clone() const
{
   return new LibraryListWidgetItem(*this);
}

QSize LibraryListWidgetItem::calculateMaxSize(QSize size)
{
    return {size.width(), (int)round(size.height()-size.height()/3.5)};
}

QSize LibraryListWidgetItem::calculateHintSize(QSize size)
{
    return {size.width(), size.height()-size.height()/7};
}
LibrarySortMode LibraryListWidgetItem::_sortMode = LibrarySortMode::NAME_ASC;

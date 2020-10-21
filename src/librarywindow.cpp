#include "librarywindow.h"

LibraryWindow::LibraryWindow(QMainWindow* parent) :
    QFrame(parent)
{
    setWindowFlags(Qt::Window);
    setLayout(new QGridLayout(this));
    resize({675,500});
}

void LibraryWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    emit close();
}

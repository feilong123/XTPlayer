QT       += core gui multimedia multimediawidgets serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    CustomControls/RangeSlider.cpp \
    lib/handler/funscripthandler.cpp \
    lib/handler/loghandler.cpp \
    lib/handler/serialhandler.cpp \
    lib/handler/settingshandler.cpp \
    lib/handler/tcodehandler.cpp \
    lib/handler/udphandler.cpp \
    lib/tool/xmath.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    CustomControls/RangeSlider.h \
    CustomControls/xmediaplayer.h \
    CustomControls/xvideowidget.h \
    lib/enum/enum.h \
    lib/handler/funscripthandler.h \
    lib/handler/loghandler.h \
    lib/handler/loghandler.h \
    lib/handler/serialhandler.h \
    lib/handler/settingshandler.h \
    lib/handler/tcodehandler.h \
    lib/handler/udphandler.h \
    lib/lookup/enum.h \
    lib/struct/ConnectionChangedSignal.h \
    lib/struct/FunscriptStruct.h \
    lib/struct/LibraryListItem.h \
    lib/struct/SerialComboboxItem.h \
    lib/tool/xmath.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

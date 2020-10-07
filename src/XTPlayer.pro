QT += core gui serialport network gamepad texttospeech

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets avwidgets
} else {
  CONFIG += avwidgets
}
#rpath for apple
mac {
  RPATHDIR *= @loader_path/../Frameworks
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}
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
    CustomControls/rangeslider.cpp \
    lib/handler/deohandler.cpp \
    lib/handler/devicehandler.cpp \
    lib/handler/funscripthandler.cpp \
    lib/handler/gamepadhandler.cpp \
    lib/handler/loghandler.cpp \
    lib/handler/serialhandler.cpp \
    lib/handler/settingshandler.cpp \
    lib/handler/tcodehandler.cpp \
    lib/handler/udphandler.cpp \
    lib/handler/videohandler.cpp \
    lib/tool/tcodefactory.cpp \
    lib/tool/xmath.cpp \
    lib/tool/xtimer.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp

HEADERS += \
    CustomControls/rangeslider.h \
    lib/handler/deohandler.h \
    lib/handler/devicehandler.h \
    lib/handler/gamepadhandler.h \
    lib/handler/videohandler.h \
    lib/lookup/AxisNames.h \
    lib/lookup/GamepadAxisNames.h \
    lib/lookup/MediaActions.h \
    lib/lookup/enum.h \
    lib/handler/funscripthandler.h \
    lib/handler/loghandler.h \
    lib/handler/loghandler.h \
    lib/handler/serialhandler.h \
    lib/handler/settingshandler.h \
    lib/handler/tcodehandler.h \
    lib/handler/udphandler.h \
    lib/lookup/enum.h \
    lib/struct/ChannelModel.h \
    lib/struct/ConnectionChangedSignal.h \
    lib/struct/DeoPacket.h \
    lib/struct/Funscript.h \
    lib/struct/GamepadState.h \
    lib/struct/LibraryListItem.h \
    lib/struct/NetworkAddress.h \
    lib/struct/SerialComboboxItem.h \
    lib/tool/boolinq.h \
    lib/tool/tcodefactory.h \
    lib/tool/xmath.h \
    lib/tool/xtimer.h \
    mainwindow.h \
    settingsdialog.h

FORMS += \
    mainwindow.ui \
    settings.ui

# LIBS       += -lVLCQtCore -lVLCQtWidgets
unix:!mac {
    LIBS += -L$$QT.core.libs -lQtAV -lQtAVWidgets
    QMAKE_RPATHDIR += ../lib
}
unix:mac {
    INCLUDEPATH += $$QT.core.libs/QtAV.framework/Versions/1/Headers
    QMAKE_LFLAGS += -F$$QT.core.libs
    QMAKE_RPATHDIR += @executable_path/../lib /usr/lib
    LIBS += -framework QtAV
}
unix {
    DESTDIR = $$shell_path($$OUT_PWD)
}
win32{
    LIBS += -L$$QT.core.libs -lQtAV1 -lQtAVWidgets1
    build_pass: CONFIG(debug, debug|release) {
        DESTDIR = $$shell_path($$OUT_PWD/debug)
    }
    else: build_pass {
        DESTDIR = $$shell_path($$OUT_PWD/release)
    }
    RC_FILE = XTPlayer.rc
}

#mkspecs_features.files    = $$PWD/qss/default.qss
#mkspecs_features.path     = $$OUT_PWD/qss
#INSTALLS                  += mkspecs_features

#https://stackoverflow.com/questions/19066593/copy-a-file-to-build-directory-after-compiling-project-with-qt
#copydata.commands = $(COPY_DIR) \"$$shell_path($$PWD\\qss\\)\" \"$$shell_path($$OUT_PWD)\"
#first.depends = $(first) copydata
#export(first.depends)
#export(copydata.commands)
#QMAKE_EXTRA_TARGETS += first copydata

#execute script
#mypackagerule.target = mypackagerule
#mypackagerule.command = exec my_package_script.sh
#QMAKE_EXTRA_TARGETS += mypackagerule

copydata.commands = $(COPY_DIR) $$shell_path($$PWD/themes) $$shell_path($$DESTDIR/themes)
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

#QMAKE_EXTRA_TARGETS += foo bar
#foo.target = $$shell_path($$DESTDIR/themes)
#foo.commands = $(MKDIR) $$shell_path($$DESTDIR/themes)
#bar.target = $$OUT_PWD/newFolder/file
#bar.commands = $(COPY_DIR) $$shell_path($$PWD/themes/default.qss)
#bar.depends = foo

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    qss/default.qss

RESOURCES += \
    icons.qrc
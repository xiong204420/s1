QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets mqtt

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += ./include
LIBS += $$PWD/libs/HCCore.lib
LIBS += $$PWD/libs/HCNetSDK.lib
LIBS += $$PWD/libs/PlayCtrl.lib
LIBS += $$PWD/libs/HCNetSDKCom/HCAlarm.lib
LIBS += $$PWD/libs/HCNetSDKCom/HCGeneralCfgMgr.lib
LIBS += $$PWD/libs/HCNetSDKCom/HCPreview.lib

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ico.rc

RESOURCES += \
    res.qrc

RC_FILE += ico.rc

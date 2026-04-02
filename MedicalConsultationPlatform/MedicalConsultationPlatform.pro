# 医疗咨询平台（医生端）— Qt 6 qmake 工程
QT += core gui network websockets sql widgets charts
CONFIG += c++17
CONFIG -= app_bundle

TARGET = MedicalConsultationPlatform
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    logindialog.cpp \
    mainwindow.cpp \
    chatwidget.cpp \
    prescriptiondialog.cpp \
    scheduledialog.cpp \
    settingsdialog.cpp \
    earningsdialog.cpp \
    httpclient.cpp \
    websocketclient.cpp \
    localdatabase.cpp \
    logger.cpp \
    cryptohelper.cpp

HEADERS += \
    appconfig.h \
    logindialog.h \
    mainwindow.h \
    chatwidget.h \
    prescriptiondialog.h \
    scheduledialog.h \
    settingsdialog.h \
    earningsdialog.h \
    httpclient.h \
    websocketclient.h \
    localdatabase.h \
    logger.h \
    cryptohelper.h

# 默认部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

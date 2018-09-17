#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

# Emit compiler warnings when using deprecated Qt features
#DEFINES += QT_DEPRECATED_WARNINGS

QT += core gui widgets
TARGET = QtClient
TEMPLATE = app

INCLUDEPATH += $$PWD/ui

SOURCES += \
    src/CandlestickChart/candledataset.cpp \
    src/CandlestickChart/candleplot.cpp \
    src/CandlestickChart/candleplotpicker.cpp \
    src/chartdialog.cpp \
    src/logindialog.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/orderbookdialog.cpp \
    src/orderbookmodel.cpp \
    src/portfoliomodel.cpp \
    src/overviewmodel.cpp \
    src/stocklistfiltermodel.cpp \
    src/navpicker.cpp \
    src/navigationplot.cpp \
    src/waitinglistmodel.cpp \
    src/qtcoreclient.cpp \
    src/app.cpp \
    global.cpp

HEADERS += \
    include/CandlestickChart/candledataset.h \
    include/CandlestickChart/candleplot.h \
    include/CandlestickChart/candleplotpicker.h \
    include/chartdialog.h \
    include/logindialog.h \
    include/mainwindow.h \
    include/orderbookdialog.h \
    include/orderbookmodel.h \
    include/portfoliomodel.h \
    include/overviewmodel.h \
    include/stocklistfiltermodel.h \
    include/navpicker.h \
    include/navigationplot.h \
    include/waitinglistmodel.h \
    include/qtcoreclient.h \
    include/app.h \
    global.h

FORMS += \
    ui/chartdialog.ui \
    ui/logindialog.ui \
    ui/mainwindow.ui \
    ui/orderbookdialog.ui

DISTFILES += \
    config/initiator.cfg

# Ubuntu only
unix:!mac {
    # QWT
    LIBS += -L/usr/local/qwt-6.1.3/lib -lqwt
}

# macOS only
mac: {
    # pkg-config
    PKG_CONFIG = /usr/local/bin/pkg-config
}

# UNIX
unix: {
    # pkg-config
    CONFIG += link_pkgconfig

    # QWT
    include (/usr/local/qwt-6.1.3/features/qwt.prf)
    INCLUDEPATH += /usr/local/qwt-6.1.3/include

    # QuickFIX
    PKGCONFIG += quickfix

    # LibCoreClient
    CONFIG(debug, debug | release): PKGCONFIG += libshift_coreclient-d
    CONFIG(release, debug | release): PKGCONFIG += libshift_coreclient

    # Copy ./config to build directory
    config.files = $$PWD/config/
    config.path = $$OUT_PWD
    INSTALLS += config
}

# Windows
win32 {
    # QWT
    include ($$(QWT_ROOT)/features/qwt.prf)

    # cURL
    Debug:DEPENDPATH += $$(CURL_LIB)/libcurl-vc14-x64-debug-static-ipv6-sspi-winssl/include/curl
    Debug:INCLUDEPATH += $$(CURL_LIB)/libcurl-vc14-x64-debug-static-ipv6-sspi-winssl/include/curl

    Debug:LIBS += -L$$(CURL_LIB)/libcurl-vc14-x64-debug-static-ipv6-sspi-winssl/lib/ -llibcurl_a_debug
    Debug:PRE_TARGETDEPS += $$(CURL_LIB)/libcurl-vc14-x64-debug-static-ipv6-sspi-winssl/lib/libcurl_a_debug.lib

    Release:DEPENDPATH += $$(CURL_LIB)/libcurl-vc14-x64-release-static-ipv6-sspi-winssl/include/curl
    Release:INCLUDEPATH += $$(CURL_LIB)/libcurl-vc14-x64-release -static-ipv6-sspi-winssl/include/curl

    Release:LIBS += -L$$(CURL_LIB)/libcurl-vc14-x64-release-static-ipv6-sspi-winssl/lib/ -llibcurl_a
    Release:PRE_TARGETDEPS += $$(CURL_LIB)/libcurl-vc14-x64-release-static-ipv6-sspi-winssl/lib/libcurl_a.lib

    # QuickFIX
    DEPENDPATH += $$(QuickFIX_VS2017)/include
    INCLUDEPATH += $$(QuickFIX_VS2017)/include

    Debug:LIBS += -L$$(QuickFIX_VS2017)/x64/Debug/ -lquickfix
    Debug:PRE_TARGETDEPS += $$(QuickFIX_VS2017)/x64/Debug/quickfix.lib

    Release:LIBS += -L$$(QuickFIX_VS2017)/x64/Release -lquickfix
    Release:PRE_TARGETDEPS += $$(QuickFIX_VS2017)/x64/Release/quickfix.lib

    # LibMiscUtils
    DEPENDPATH += $$PWD/../../LibMiscUtils/include
    INCLUDEPATH += $$PWD/../../LibMiscUtils/include

    Debug:LIBS += -L$$PWD/../../LibMiscUtils/build/Debug/ -lshift_miscutils-d
    Debug:PRE_TARGETDEPS += $$PWD/../../LibMiscUtils/build/Debug/shift_miscutils-d.lib

    Release:LIBS += -L$$PWD/../../LibMiscUtils/build/Release/ -lshift_miscutils
    Release:PRE_TARGETDEPS += $$PWD/../../LibMiscUtils/build/Release/shift_miscutils.lib

    # LibCoreClient
    DEPENDPATH += $$PWD/../../LibCoreClient/include
    INCLUDEPATH += $$PWD/../../LibCoreClient/include

    Debug:LIBS += -L$$PWD/../../LibCoreClient/build/Debug/ -lshift_coreclient-d
    Debug:PRE_TARGETDEPS += $$PWD/../../LibCoreClient/build/Debug/shift_coreclient-d.lib

    Release:LIBS += -L$$PWD/../../LibCoreClient/build/Release/ -lshift_coreclient
    Release:PRE_TARGETDEPS += $$PWD/../../LibCoreClient/build/Release/shift_coreclient.lib

    # Copy ./config to build directory
    copydata.commands = $(COPY_DIR) \"$$shell_path($$PWD\\config)\" \"$$shell_path($$OUT_PWD\\config)\"
    first.depends = $(first) copydata
    export(first.depends)
    export(copydata.commands)
    QMAKE_EXTRA_TARGETS += first copydata
#   See discussion in
#   https://stackoverflow.com/questions/19066593/copy-a-file-to-build-directory-after-compiling-project-with-qt
#   for a detailed explanation on how it works.
}

RESOURCES += \
    src/theme.qrc

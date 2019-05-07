QT -= gui

include(../JASP.pri)

CONFIG += c++11
TARGET = $$JASP_R_INTERFACE_NAME
DESTDIR = ..
TEMPLATE = lib
unix:CONFIG += staticlib

QMAKE_CLEAN += $$OUT_PWD/$$DESTDIR/'lib'$$JASP_R_INTERFACE_TARGET'*.a'

#comment this out if you do not want helpertraces for development of jaspResults and such
#CONFIG(debug, debug|release) {  DEFINES+=JASP_RESULTS_DEBUG_TRACES }

windows:QMAKE_CLEAN += $$OUT_PWD/$$DESTDIR/$$JASP_R_INTERFACE_TARGET'*.lib' $$OUT_PWD/$$DESTDIR/$$JASP_R_INTERFACE_TARGET'*.dll'

macx: QMAKE_CLEAN +=$$OUT_PWD/$$DESTDIR/'lib'$$JASP_R_INTERFACE_TARGET'*.dylib'

include(../R_HOME.pri)
INCLUDEPATH += ../JASP-Common

win32: LIBS += -L$$_R_HOME/bin/$$ARCH -lR

DEFINES += JASP_R_INTERFACE_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    jasprcpp.cpp \
    RInside/MemBuf.cpp \
    RInside/RInside.cpp \
    jaspResults/src/jaspHtml.cpp \
    jaspResults/src/jaspObject.cpp \
    jaspResults/src/jaspJson.cpp \
    jaspResults/src/jaspContainer.cpp \
    jaspResults/src/jaspPlot.cpp \
    jaspResults/src/jaspResults.cpp \
    jaspResults/src/jaspTable.cpp \
    jaspResults/src/jaspState.cpp

HEADERS += \
    jasprcpp_interface.h \
    jasprcpp.h \
    RInside/Callbacks.h \
    RInside/MemBuf.h \
    RInside/RInside.h \
    RInside/RInsideAutoloads.h \
    RInside/RInsideCommon.h \
    RInside/RInsideConfig.h \
    RInside/RInsideEnvVars.h \
    jaspResults/src/jaspHtml.h \
    jaspResults/src/jaspObject.h \
    jaspResults/src/jaspJson.h \
    jaspResults/src/jaspList.h \
    jaspResults/src/jaspContainer.h \
    jaspResults/src/jaspPlot.h \
    jaspResults/src/jaspResults.h \
    jaspResults/src/jaspTable.h \
    jaspResults/src/jaspModuleRegistration.h \
    jaspResults/src/jaspState.h


windows{
  QMAKE_CXXFLAGS += -Og #for big object files
  SOURCE_LIBFILE = $$OUT_PWD/$$DESTDIR/'lib'$$JASP_R_INTERFACE_NAME'.a'
   SOURCE_LIBFILE ~= s,/,\\,g
	DEST_LIBFILE = $$OUT_PWD/$$DESTDIR/$$JASP_R_INTERFACE_NAME'.lib'
  DEST_LIBFILE ~= s,/,\\,g

  QMAKE_POST_LINK     += $$quote(cmd /c copy /Y $$SOURCE_LIBFILE $$DEST_LIBFILE)
}

### making sure that writeImage.R and zzzWrappers.R are available to jaspEngine:
SRC_WRITE_IMAGE = $${PWD}/jaspResults/R/writeImage.R
SRC_WRAPPERS    = $${PWD}/jaspResults/R/zzzWrappers.R
DEST_DIR_AUX_R  = $$OUT_PWD/$$DESTDIR

auxillaryRFiles.path = $$INSTALLPATH
auxillaryRFiles.files = $${PWD}/jaspResults/R/writeImage.R
auxillaryRFiles.files += $${PWD}/jaspResults/R/zzzWrappers.R
INSTALLS += auxillaryRFiles

win32 {
    SRC_WRITE_IMAGE ~= s,/,\\,g
    SRC_WRAPPERS ~= s,/,\\,g
    DEST_DIR_AUX_R ~= s,/,\\,g

    copyRFiles.commands  += $$quote(cmd /c xcopy /I /Y $${SRC_WRITE_IMAGE} $${DEST_DIR_AUX_R}) $$escape_expand(\n\t)
    copyRFiles.commands  += $$quote(cmd /c xcopy /I /Y $${SRC_WRAPPERS}    $${DEST_DIR_AUX_R})
}

unix {
    copyRFiles.commands += $(MKDIR) $$DEST_DIR_AUX_R ;
    copyRFiles.commands += cp $$SRC_WRITE_IMAGE $$DEST_DIR_AUX_R ;
    copyRFiles.commands += cp $$SRC_WRAPPERS $$DEST_DIR_AUX_R ;
}


! equals(PWD, $${OUT_PWD}) {
    QMAKE_EXTRA_TARGETS += copyRFiles
    POST_TARGETDEPS     += copyRFiles
}

DISTFILES += \
    jaspResults/R/RcppExports.R \
    jaspResults/R/zzaLoadModule.R \
    jaspResults/R/zzzWrappers.R \
    jaspResults/R/writeImage.R \
    jaspResults/DESCRIPTION \
    jaspResults/NAMESPACE \
    jaspResults/man/jaspList.Rd \
    jaspResults/man/jaspTable.Rd \
    jaspResults/man/jaspResultsClass.Rd \
    jaspResults/man/jaspPlot.Rd \
    jaspResults/man/jaspObject.Rd \
    jaspResults/man/jaspHtml.Rd \
    jaspResults/man/jaspContainer.Rd \
    jaspResults/man/jaspState.Rd \
    jaspResults/man/jaspResults-package.Rd

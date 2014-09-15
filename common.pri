!isEmpty(COMMON_PRI_INCLUDED):error("common.pri already included")
COMMON_PRI_INCLUDED = 1

# enable c++11
isEqual(QT_MAJOR_VERSION, 5) {
    CONFIG += c++11
} else {
    macx {
        !macx-clang*: error("You need to use the macx-clang or macx-clang-libc++ mkspec to compile Qt Creator (call qmake with '-spec unsupported/macx-clang')")
        QMAKE_CFLAGS += -mmacosx-version-min=10.7
        QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.7
        QMAKE_OBJECTIVE_CXXFLAGS += -std=c++11 -stdlib=libc++ -mmacosx-version-min=10.7
        QMAKE_LFLAGS += -stdlib=libc++ -mmacosx-version-min=10.7
    } else:linux-g++* {
        QMAKE_CXXFLAGS += -std=c++0x
    } else:linux-icc* {
        QMAKE_CXXFLAGS += -std=c++11
    } else:linux-clang* {
        QMAKE_CXXFLAGS += -std=c++11
        QMAKE_LFLAGS += -stdlib=libc++ -lc++abi
    } else:win32-g++* {
        QMAKE_CXXFLAGS += -std=c++0x
    }
    # nothing to do for MSVC10+
}

isEqual(QT_MAJOR_VERSION, 5) {

defineReplace(cleanPath) {
    return($$clean_path($$1))
}

defineReplace(targetPath) {
    return($$shell_path($$1))
}

} else {

defineReplace(cleanPath) {
    win32:1 ~= s|\\\\|/|g
    contains(1, ^/.*):pfx = /
    else:pfx =
    segs = $$split(1, /)
    out =
    for(seg, segs) {
        equals(seg, ..):out = $$member(out, 0, -2)
        else:!equals(seg, .):out += $$seg
    }
    return($$join(out, /, $$pfx))
}

defineReplace(targetPath) {
    return($$replace(1, /, $$QMAKE_DIR_SEP))
}

}

defineReplace(qtLibraryName) {
   unset(LIBRARY_NAME)
   LIBRARY_NAME = $$1
   CONFIG(debug, debug|release) {
      !debug_and_release|build_pass {
          mac:RET = $$member(LIBRARY_NAME, 0)_debug
              else:win32:RET = $$member(LIBRARY_NAME, 0)d
      }
   }
   isEmpty(RET):RET = $$LIBRARY_NAME
   return($$RET)
}

isEqual(QT_MAJOR_VERSION, 5) {

# For use in custom compilers which just copy files
defineReplace(stripSrcDir) {
    return($$relative_path($$absolute_path($$1, $$OUT_PWD), $$_PRO_FILE_PWD_))
}

} else {

# For use in custom compilers which just copy files
win32:i_flag = i
defineReplace(stripSrcDir) {
    win32 {
        !contains(1, ^.:.*):1 = $$OUT_PWD/$$1
    } else {
        !contains(1, ^/.*):1 = $$OUT_PWD/$$1
    }
    out = $$cleanPath($$1)
    out ~= s|^$$re_escape($$PWD/)||$$i_flag
    return($$out)
}

}

isEmpty(LIBRARY_BASENAME) {
    LIBRARY_BASENAME = lib
}

SOURCE_TREE = $$PWD
isEmpty(BUILD_TREE) {
    sub_dir = $$_PRO_FILE_PWD_
    sub_dir ~= s,^$$re_escape($$SOURCE_TREE),,
    BUILD_TREE = $$cleanPath($$OUT_PWD)
    BUILD_TREE ~= s,$$re_escape($$sub_dir)$,,
}

APP_PATH = $$BUILD_TREE/bin

contains(TEMPLATE, vc.*):vcproj = 1
LIBRARY_PATH = $$BUILD_TREE/$$LIBRARY_BASENAME
!isEqual(SOURCE_TREE, $$BUILD_TREE):copydata = 1

INCLUDEPATH += $$SOURCE_TREE/src
DEPENDPATH += $$INCLUDEPATH

LIBS += -L$$LIBRARY_PATH

!isEmpty(vcproj) {
    DEFINES += LIBRARY_BASENAME=\"$$LIBRARY_BASENAME\"
} else {
    DEFINES += LIBRARY_BASENAME=\\\"$$LIBRARY_BASENAME\\\"
}

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII
!macx:DEFINES += QT_USE_FAST_OPERATOR_PLUS QT_USE_FAST_CONCATENATION

unix {
    CONFIG(debug, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    CONFIG(release, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared

    CONFIG(debug, debug|release):MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    CONFIG(release, debug|release):MOC_DIR = $${OUT_PWD}/.moc/release-shared

    RCC_DIR = $${OUT_PWD}/.rcc
    UI_DIR = $${OUT_PWD}/.uic
}

win32-msvc* {
    #Don't warn about sprintf, fopen etc being 'unsafe'
    DEFINES += _CRT_SECURE_NO_WARNINGS
    # Speed up startup time when debugging with cdb
    QMAKE_LFLAGS_DEBUG += /INCREMENTAL:NO
}

qt:greaterThan(QT_MAJOR_VERSION, 4) {
    contains(QT, core): QT += concurrent
    contains(QT, gui): QT += widgets
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x040900
}

win32 {
    BUILD_PATH = $$BUILD_TREE/build
} else {
    BUILD_PATH = $$BUILD_TREE/.build
}

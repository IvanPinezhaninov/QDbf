#=========================================================================
#
# Copyright (C) 2020 Ivan Pinezhaninov <ivan.pinezhaninov@gmail.com>
#
# This file is part of the QDbf - Qt DBF library.
#
# The QDbf is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# The QDbf is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with the QDbf.  If not, see <http://www.gnu.org/licenses/>.
#
#=========================================================================


TARGET = QDbf

QT -= gui
TEMPLATE = lib
CONFIG += shared dll c++11

include(../common.pri)

DEFINES += QDBF_LIBRARY
TARGET = $$qtLibraryName($$TARGET)
DESTDIR = $$LIBRARY_PATH
win32 {
    DLLDESTDIR = $$BUILD_TREE/bin
}
contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

HEADERS += \
    $$SOURCE_TREE/include/qdbf_compat.h \
    $$SOURCE_TREE/include/qdbf_global.h \
    $$SOURCE_TREE/include/qdbffield.h \
    $$SOURCE_TREE/include/qdbfrecord.h \
    $$SOURCE_TREE/include/qdbftable.h \
    $$SOURCE_TREE/include/qdbftablemodel.h

SOURCES += \
    $$SOURCE_TREE/src/qdbffield.cpp \
    $$SOURCE_TREE/src/qdbfrecord.cpp \
    $$SOURCE_TREE/src/qdbftable.cpp \
    $$SOURCE_TREE/src/qdbftablemodel.cpp

!macx {
    win32 {
        dlltarget.path = /bin
        INSTALLS += dlltarget
    } else {
        target.path = /$$LIBRARY_BASENAME
        INSTALLS += target
    }
}

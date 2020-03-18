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


TARGET = QDbfExample
TEMPLATE = app
CONFIG += c++11

include(../common.pri)
include(rpath.pri)

DESTDIR = $$BUILD_TREE/bin

LIBS *= -l$$qtLibraryName(QDbf)

HEADERS += \
    mainwindow.h

SOURCES += \
    main.cpp \
    mainwindow.cpp

target.path = /bin
INSTALLS += target

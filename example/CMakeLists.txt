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


cmake_minimum_required(VERSION 2.8.11)

project(QDbfExample CXX)

if(QT_VERSION_MAJOR MATCHES 5)
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5Widgets REQUIRED)
  add_definitions(${Qt5Gui_DEFINITIONS} ${Qt5Widgets_DEFINITIONS})
else()
  find_package(Qt4 COMPONENTS QtGui REQUIRED)
endif()

set(HEADERS
  mainwindow.h
)

set(SOURCES
  mainwindow.cpp
  main.cpp
)

set(MOC_HEADERS
  mainwindow.h
)

if(QT_VERSION_MAJOR MATCHES 5)
  qt5_wrap_cpp(SOURCES ${MOC_HEADERS})
  qt5_generate_moc(mainwindow.h ${CMAKE_CURRENT_BINARY_DIR}/moc_mainwindow.cpp)
else()
  qt4_wrap_cpp(SOURCES ${MOC_HEADERS})
  qt4_generate_moc(mainwindow.h ${CMAKE_CURRENT_BINARY_DIR}/moc_mainwindow.cpp)
ENDIF()

set_source_files_properties(mainwindow.cpp PROPERTIES
  OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_mainwindow.cpp
)

set(TARGET QDbfExample)

add_executable(${TARGET}
  ${HEADERS}
  ${SOURCES}
  ${MOC}
)

if(QT_VERSION_MAJOR MATCHES 5)
  target_link_libraries(${TARGET}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    QDbf
  )
else()
  target_link_libraries(${TARGET}
    ${QT_LIBRARIES}
    QDbf
  )
endif()

install(TARGETS ${TARGET} RUNTIME DESTINATION bin)

/***************************************************************************
**
** Copyright (C) 2020 Ivan Pinezhaninov <ivan.pinezhaninov@gmail.com>
**
** This file is part of the QDbf - Qt DBF library.
**
** The QDbf is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** The QDbf is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with the QDbf.  If not, see <http://www.gnu.org/licenses/>.
**
***************************************************************************/


import qbs.base 1.0

Project {
    property bool buildExample: false

    DynamicLibrary {
        name: "QDbf"

        Group {
            name: "publicHeaders"
            prefix: "include/"
            files:[ "*.h" ]
            qbs.install: true
            qbs.installDir: "include"
        }

        Group {
            name: "sources";
            prefix: "src/"
            files:[ "*.cpp" ]
        }

        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: [ "core" ] }

        cpp.defines: [ "QDBF_LIBRARY" ]
        cpp.positionIndependentCode: true
        cpp.cxxLanguageVersion: "c++11"
        cpp.includePaths: "include"
        cpp.visibility: "minimal"

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: "include"
            cpp.cxxLanguageVersion: "c++11"
        }

        Group {
            name: "Install library"
            qbs.install: true
            qbs.installDir: "lib"
            fileTagsFilter: product.type
        }
    }

    SubProject {
        filePath: "example/example.qbs"

        Properties {
            condition: buildExample
        }
    }
}

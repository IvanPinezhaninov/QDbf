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


#ifndef QDBF_COMPAT_H
#define QDBF_COMPAT_H

#include <QtGlobal>

#if QT_VERSION < 0x050000
# ifndef Q_CC_MSVC
#   define Q_DECL_NOEXCEPT noexcept
# else
#   define Q_DECL_NOEXCEPT
# endif
#endif

#endif // QDBF_COMPAT_H

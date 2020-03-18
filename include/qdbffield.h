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

#ifndef QDBFFIELD_H
#define QDBFFIELD_H

#include <QVariant>

#include "qdbf_compat.h"
#include "qdbf_global.h"


namespace QDbf {
namespace Internal {
class QDbfFieldPrivate;
class QDbfTablePrivate;
} // namespace Internal

class QDBF_EXPORT QDbfField
{
public:
    explicit QDbfField(QString fieldName = QString());

    QDbfField(const QDbfField &other);
    QDbfField(QDbfField &&other) Q_DECL_NOEXCEPT;

    QDbfField &operator=(const QDbfField &other);
    QDbfField &operator=(QDbfField &&other) Q_DECL_NOEXCEPT;

    bool operator==(const QDbfField &other) const;
    bool operator!=(const QDbfField &other) const;

    virtual ~QDbfField();

    enum QDbfType {
        Undefined = -1,
        Character,
        Date,
        FloatingPoint,
        Logical,
        Memo,
        Number,
        Integer,
        DateTime
    };

    void setValue(QVariant value);
    inline QVariant value() const { return val; }

    void setName(QString name);
    QString name() const;

    bool isNull() const;

    void setReadOnly(bool readOnly);
    bool isReadOnly() const;

    void clear();

    void setType(QDbfType type);
    QDbfType type() const;

    void setLength(int length);
    int length() const;

    void setPrecision(int precision);
    int precision() const;

    void setOffset(int offset);
    int offset() const;

    void setDefaultValue(QVariant value);
    QVariant defaultValue() const;

    void swap(QDbfField &other) Q_DECL_NOEXCEPT;

private:
    Internal::QDbfFieldPrivate *d;
    QVariant val;
    void detach();

    friend class Internal::QDbfTablePrivate;
};

void swap(QDbfField &lhs, QDbfField &rhs);

} // namespace QDbf

QDebug operator<<(QDebug, const QDbf::QDbfField &);

#endif // QDBFFIELD_H

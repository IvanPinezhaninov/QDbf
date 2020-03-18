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

#ifndef QDBFRECORD_H
#define QDBFRECORD_H

#include "qdbf_compat.h"
#include "qdbf_global.h"

QT_BEGIN_NAMESPACE
class QString;
class QVariant;
QT_END_NAMESPACE


namespace QDbf {
namespace Internal {
class QDbfRecordPrivate;
}

class QDbfField;

class QDBF_EXPORT QDbfRecord
{
public:
    QDbfRecord();

    QDbfRecord(const QDbfRecord &other);
    QDbfRecord(QDbfRecord &&other) Q_DECL_NOEXCEPT;

    QDbfRecord &operator=(const QDbfRecord &other);
    QDbfRecord &operator=(QDbfRecord &&other) Q_DECL_NOEXCEPT;

    bool operator==(const QDbfRecord &other) const;
    bool operator!=(const QDbfRecord &other) const;

    virtual ~QDbfRecord();

    void setRecordIndex(int index);
    int recordIndex() const;

    void setValue(int fieldIndex, QVariant value);
    QVariant value(int fieldIndex) const;

    void setValue(const QString &fieldName, QVariant value);
    QVariant value(const QString &fieldName) const;

    void setNull(int fieldIndex);
    bool isNull(int fieldIndex) const;

    void setNull(const QString &fieldName);
    bool isNull(const QString &fieldName) const;

    int indexOf(const QString &fieldName) const;
    QString fieldName(int fieldIndex) const;

    QDbfField field(int fieldIndex) const;
    QDbfField field(const QString &fieldName) const;

    void append(const QDbfField &field);
    void replace(int pos, const QDbfField &field);
    void insert(int pos, const QDbfField &field);
    void remove(int pos);

    bool isEmpty() const;

    void setDeleted(bool deleted);
    bool isDeleted() const;

    bool contains(int fieldIndex) const;
    bool contains(const QString &fieldName) const;
    void clear();
    void clearValues();
    int count() const;

    void swap(QDbfRecord &other) Q_DECL_NOEXCEPT;

private:
    Internal::QDbfRecordPrivate *d;
    void detach();
};

void swap(QDbfRecord &lhs, QDbfRecord &rhs);

} // namespace QDbf

QDebug operator<<(QDebug, const QDbf::QDbfRecord &);

#endif // QDBFRECORD_H

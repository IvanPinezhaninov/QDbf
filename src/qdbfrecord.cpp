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

#include <QDebug>
#include <QVariant>
#include <QVector>

#include "qdbffield.h"
#include "qdbfrecord.h"


namespace QDbf {
namespace Internal {

class QDbfRecordPrivate final
{
public:
    QDbfRecordPrivate() = default;
    QDbfRecordPrivate(const QDbfRecordPrivate &other);
    QDbfRecordPrivate(QDbfRecordPrivate &&other) = delete;
    QDbfRecordPrivate &operator=(const QDbfRecordPrivate &other) = delete;
    QDbfRecordPrivate &operator=(QDbfRecordPrivate &&other) = delete;
    virtual ~QDbfRecordPrivate() = default;

    QAtomicInt ref = 1;
    int m_index = -1;
    QVector<QDbfField> m_fields;
    bool m_deleted = false;
};


QDbfRecordPrivate::QDbfRecordPrivate(const QDbfRecordPrivate &other) :
    m_index(other.m_index),
    m_fields(other.m_fields),
    m_deleted(other.m_deleted)
{
}

} // namespace Internal


QDbfRecord::QDbfRecord() :
    d(new Internal::QDbfRecordPrivate())
{
}


QDbfRecord::QDbfRecord(const QDbfRecord &other) :
    d(other.d)
{
    d->ref.ref();
}


QDbfRecord::QDbfRecord(QDbfRecord &&other) Q_DECL_NOEXCEPT :
    d(other.d)
{
    other.d = nullptr;
}


QDbfRecord &QDbfRecord::operator=(const QDbfRecord &other)
{
    if (this == &other) {
        return *this;
    }

    qAtomicAssign(d, other.d);
    return *this;
}


QDbfRecord &QDbfRecord::operator=(QDbfRecord &&other) Q_DECL_NOEXCEPT
{
    other.swap(*this);
    return *this;
}


bool QDbfRecord::operator==(const QDbfRecord &other) const
{
    return (recordIndex() == other.recordIndex() &&
            isDeleted() == other.isDeleted() &&
            d->m_fields == other.d->m_fields);
}


bool QDbfRecord::operator!=(const QDbfRecord &other) const
{
    return !operator==(other);
}


QDbfRecord::~QDbfRecord()
{
    if (nullptr != d && !d->ref.deref()) {
        delete d;
        d = nullptr;
    }
}


void QDbfRecord::setRecordIndex(int index)
{
    d->m_index = index;
}


int QDbfRecord::recordIndex() const
{
    return d->m_index;
}


void QDbfRecord::setValue(int fieldIndex, QVariant value)
{
    if (!contains(fieldIndex)) {
        return;
    }

    detach();
    d->m_fields[fieldIndex].setValue(std::move(value));
}


QVariant QDbfRecord::value(int fieldIndex) const
{
    return d->m_fields.value(fieldIndex).value();
}


void QDbfRecord::setValue(const QString &fieldName, QVariant value)
{
    setValue(indexOf(fieldName), std::move(value));
}


QVariant QDbfRecord::value(const QString &fieldName) const
{
    return value(indexOf(fieldName));
}


void QDbfRecord::setNull(int fieldIndex)
{
    if (!contains(fieldIndex)) {
        return;
    }

    detach();
    d->m_fields[fieldIndex].clear();
}


bool QDbfRecord::isNull(int fieldIndex) const
{
    return d->m_fields.value(fieldIndex).isNull();
}


void QDbfRecord::setNull(const QString &fieldName)
{
    setNull(indexOf(fieldName));
}


bool QDbfRecord::isNull(const QString &fieldName) const
{
    return isNull(indexOf(fieldName));
}


int QDbfRecord::indexOf(const QString &fieldName) const
{
    const auto &name = fieldName.toUpper();

    for (auto i = 0; i < count(); ++i) {
        if (d->m_fields.at(i).name().toUpper() == name) {
            return i;
        }
    }

    return -1;
}


QString QDbfRecord::fieldName(int fieldIndex) const
{
    return d->m_fields.value(fieldIndex).name();
}


QDbfField QDbfRecord::field(int fieldIndex) const
{
    return d->m_fields.value(fieldIndex);
}


QDbfField QDbfRecord::field(const QString &fieldName) const
{
    return field(indexOf(fieldName));
}


void QDbfRecord::append(const QDbfField &field)
{
    detach();
    d->m_fields.append(field);
}


void QDbfRecord::replace(int pos, const QDbfField &field)
{
    if (!contains(pos)) {
        return;
    }

    detach();
    d->m_fields[pos] = field;
}


void QDbfRecord::insert(int pos, const QDbfField &field)
{
    detach();
    d->m_fields.insert(pos, field);
}


void QDbfRecord::remove(int pos)
{
    if (!contains(pos)) {
        return;
    }

    detach();
    d->m_fields.remove(pos);
}


bool QDbfRecord::isEmpty() const
{
    return d->m_fields.isEmpty();
}


void QDbfRecord::setDeleted(bool deleted)
{
    detach();
    d->m_deleted = deleted;
}


bool QDbfRecord::isDeleted() const
{
    return d->m_deleted;
}


bool QDbfRecord::contains(int fieldIndex) const
{
    return fieldIndex >= 0 && fieldIndex < d->m_fields.count();
}


bool QDbfRecord::contains(const QString &fieldName) const
{
    return (indexOf(fieldName) >= 0);
}


void QDbfRecord::clear()
{
    detach();
    d->m_fields.clear();
}


void QDbfRecord::clearValues()
{
    detach();
    int count = d->m_fields.count();
    for (auto i = 0; i < count; ++i) {
        d->m_fields[i].clear();
    }
}


int QDbfRecord::count() const
{
    return d->m_fields.count();
}


void QDbfRecord::swap(QDbfRecord &other) Q_DECL_NOEXCEPT
{
    std::swap(d, other.d);
}


void QDbfRecord::detach()
{
    qAtomicDetach(d);
}


void swap(QDbfRecord &lhs, QDbfRecord &rhs)
{
    lhs.swap(rhs);
}

} // namespace QDbf


QDebug operator<<(QDebug debug, const QDbf::QDbfRecord &record)
{
    debug.nospace() << "QDbfRecord(" << record.count() << ')';

    for (auto i = 0; i < record.count(); ++i) {
        debug.nospace() << '\n' << QString::fromLatin1("%1:").arg(i, 2)
                        << record.field(i) << record.value(i).toString();
    }

    return debug.space();
}

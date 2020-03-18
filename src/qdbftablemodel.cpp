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

#include <algorithm>
#include <memory>

#include <QDate>
#include <QDebug>

#include "qdbffield.h"
#include "qdbfrecord.h"
#include "qdbftablemodel.h"

const int DBF_PREFETCH = 255;


namespace QDbf {
namespace Internal {

class QDbfTableModelPrivate final
{
public:
    explicit QDbfTableModelPrivate(QString &&filePath);

    void clear();

    QString m_filePath;
    const std::unique_ptr<QDbfTable> m_dbfTable;
    QDbfRecord m_record;
    QVector<QDbfRecord> m_records;
    QVector<QHash<int, QVariant> > m_headers;
    int m_deletedRecordsCount = 0;
    int m_lastRecordIndex = -1;
    bool m_readOnly = false;
};


QDbfTableModelPrivate::QDbfTableModelPrivate(QString &&filePath) :
    m_filePath(std::move(filePath)),
    m_dbfTable(new QDbfTable())
{
}


void QDbfTableModelPrivate::clear()
{
    m_readOnly = false;
    m_record = QDbfRecord();
    m_records.clear();
    m_headers.clear();
    m_deletedRecordsCount = 0;
    m_lastRecordIndex = -1;
}

} // namespace Internal


QDbfTableModel::QDbfTableModel(QObject *parent) :
    QDbfTableModel(QString(), parent)
{
}


QDbfTableModel::QDbfTableModel(QString filePath, QObject *parent) :
    QAbstractTableModel(parent),
    d(new Internal::QDbfTableModelPrivate(std::move(filePath)))
{
}


QDbfTableModel::QDbfTableModel(QDbfTableModel &&other) Q_DECL_NOEXCEPT :
    d(other.d)
{
    other.d = nullptr;
}


QDbfTableModel &QDbfTableModel::operator=(QDbfTableModel &&other) Q_DECL_NOEXCEPT
{
    other.swap(*this);
    return *this;
}


QDbfTableModel::~QDbfTableModel()
{
    delete d;
}


bool QDbfTableModel::open(QString filePath, bool readOnly)
{
    d->m_filePath = std::move(filePath);
    return open(readOnly);
}


bool QDbfTableModel::open(bool readOnly)
{
    d->clear();
    d->m_readOnly = readOnly;
    auto openMode = d->m_readOnly ? QDbfTable::ReadOnly : QDbfTable::ReadWrite;

    if (!d->m_dbfTable->open(d->m_filePath, openMode)) {
        return false;
    }

    d->m_record = d->m_dbfTable->record();

    if (canFetchMore()) {
        fetchMore();
    }

    return true;
}


void QDbfTableModel::close()
{
    d->clear();
    d->m_dbfTable->close();
}


bool QDbfTableModel::readOnly() const
{
    return d->m_readOnly;
}


QDbfTable::DbfTableError QDbfTableModel::error() const
{
    return d->m_dbfTable->error();
}


QDate QDbfTableModel::lastUpdate() const
{
    return d->m_dbfTable->lastUpdate();
}


int QDbfTableModel::rowCount(const QModelIndex &) const
{
    return d->m_records.count();
}


int QDbfTableModel::columnCount(const QModelIndex &) const
{
    return d->m_record.count();
}


QVariant QDbfTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= rowCount() ||
        index.column() >= columnCount()) {
        return QVariant();
    }

    const auto &value = d->m_records.at(index.row()).value(index.column());

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (value.type()) {
        case QVariant::String:
            return value.toString().trimmed();
        default:
            return value;
        }
     case Qt::CheckStateRole:
        switch (value.type()) {
        case QVariant::Bool:
            return value.toBool() ? Qt::Checked : Qt::Unchecked;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}


bool QDbfTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (Qt::Horizontal != orientation || section < 0 || columnCount() <= section) {
        return false;
    }

    if (d->m_headers.size() <= section) {
        d->m_headers.resize(std::max(section + 1, 16));
    }

    d->m_headers[section][role] = value;

    emit headerDataChanged(orientation, section, section);
    return true;
}


QVariant QDbfTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        auto value = d->m_headers.value(section).value(role);

        if (Qt::DisplayRole == role && !value.isValid()) {
            value = d->m_headers.value(section).value(Qt::EditRole);
        }

        if (value.isValid()) {
            return value;
        }

        if (Qt::DisplayRole == role && d->m_record.count() > section) {
            return d->m_record.fieldName(section);
        }
    }

    if (Qt::DisplayRole == role) {
        return section + 1;
    }

    return QVariant();
}


bool QDbfTableModel::insertRows(int row, int count, const QModelIndex &)
{
    if (row != d->m_records.size()) {
        qWarning() << "Rows can be inserted only into the end of the table";
        return false;
    }

    QVector<QDbfRecord> records;
    records.reserve(count);
    auto result = true;
    for (auto i = 0; i < count; ++i) {
        if (!d->m_dbfTable->addRecord()) {
            result = false;
            break;
        }
        d->m_dbfTable->last();
        records.append(d->m_dbfTable->record());
    }

    auto showRows = (d->m_records.size() + records.size() + d->m_deletedRecordsCount == d->m_dbfTable->size());

    if (showRows) {
        beginInsertRows({}, row, row + records.size() - 1);
        d->m_records.reserve(records.size());
#if QT_VERSION < 0x050500
        for (auto i = 0; i < records.size(); ++i) {
            d->m_records.append(records.at(i));
        }
#else
        d->m_records.append(records);
#endif
        endInsertRows();
    }

    return result;
}


bool QDbfTableModel::insertRow(int row, const QModelIndex &index)
{
    return insertRows(row, 1, index);
}


bool QDbfTableModel::removeRows(int row, int count, const QModelIndex &)
{
    if (row >= d->m_records.size() || row + count <= 0) {
        return false;
    }

    auto beginRow = std::max(0, row);
    auto endRow = std::min(row + count - 1, d->m_records.size() - 1);

    auto lastRemovedRow = -1;
    auto result = true;
    for (auto i = beginRow; i <= endRow; ++i) {
        const int recordIndex = d->m_records.at(i).recordIndex();
        if (!d->m_dbfTable->removeRecord(recordIndex)) {
            result = false;
            break;
        }
        lastRemovedRow = i;
    }

    if (lastRemovedRow > -1) {
        beginRemoveRows({}, beginRow, lastRemovedRow);

        for (auto i = beginRow; i <= lastRemovedRow; ++i) {
            d->m_records.remove(i);
        }

        d->m_deletedRecordsCount += lastRemovedRow - beginRow + 1;
        endRemoveRows();
    }

    return result;
}


bool QDbfTableModel::removeRow(int row, const QModelIndex &index)
{
    return removeRows(row, 1, index);
}


Qt::ItemFlags QDbfTableModel::flags(const QModelIndex &index) const
{
    auto flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (!index.isValid()) {
        return flags;
    }

    const auto &value = d->m_records.at(index.row()).value(index.column());

    if (QVariant::Bool == value.type()) {
        flags |= Qt::ItemIsTristate;
    }

    if (!d->m_readOnly) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}


bool QDbfTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!d->m_dbfTable->isOpen()) {
        return false;
    }

    if (index.isValid() && Qt::EditRole == role) {
        auto recordIndex = d->m_records.at(index.row()).recordIndex();
        if (!d->m_dbfTable->seek(recordIndex)) {
            return false;
        }

        if (!d->m_dbfTable->setValue(index.column(), value)) {
            return false;
        }

        d->m_records[index.row()].setValue(index.column(), value);

        emit dataChanged(index, index);
        return true;
    }

    return false;
}


bool QDbfTableModel::canFetchMore(const QModelIndex &) const
{
    return (d->m_dbfTable->isOpen() && (d->m_records.size() + d->m_deletedRecordsCount < d->m_dbfTable->size()));
}


void QDbfTableModel::fetchMore(const QModelIndex &)
{
    if (!d->m_dbfTable->seek(d->m_lastRecordIndex)) {
        return;
    }

    auto fetchSize = std::min(d->m_dbfTable->size() - d->m_records.size() - d->m_deletedRecordsCount, DBF_PREFETCH);

    QVector<QDbfRecord> records;
    records.reserve(fetchSize);
    auto deletedRecordsCount = 0;
    auto fetchedRecordSize = 0;
    while (d->m_dbfTable->next()) {
        const auto &record = d->m_dbfTable->record();
        if (record.isDeleted()) {
            ++deletedRecordsCount;
            continue;
        }
        records.append(record);
        if (++fetchedRecordSize == fetchSize) {
            break;
        }
    }
    d->m_lastRecordIndex = d->m_dbfTable->at();

    beginInsertRows({}, d->m_records.size(), d->m_records.size() + records.size() - 1);

    d->m_records.reserve(records.count());
#if QT_VERSION < 0x050500
    for (auto i = 0; i < records.size(); ++i) {
        d->m_records.append(records.at(i));
    }
#else
    d->m_records.append(records);
#endif
    d->m_deletedRecordsCount += deletedRecordsCount;

    endInsertRows();
}


void QDbfTableModel::swap(QDbfTableModel &other) Q_DECL_NOEXCEPT
{
    std::swap(d, other.d);
}


void swap(QDbfTableModel &lhs, QDbfTableModel &rhs)
{
    lhs.swap(rhs);
}

} // namespace QDbf

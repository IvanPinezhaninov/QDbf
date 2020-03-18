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

#include "qdbffield.h"

#include "qdbfrecord.h"
#include "qdbftable.h"

#include <QDataStream>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>

const quint16 DBC_LENGTH = 263;
const quint8 TERMINATOR_LENGTH = 1;

const quint8 TABLE_DESCRIPTOR_LENGTH = 32;
const quint8 TABLE_RECORDS_COUNT_OFFSET = 4;
const quint8 TABLE_LAST_UPDATE_OFFSET = 1;
const quint8 TABLE_FIRST_RECORD_POSITION_OFFSET = 8;
const quint8 RECORD_LENGTH_OFFSET = 10;
const quint8 CODEPAGE_OFFSET = 29;

const quint8 FIELD_DESCRIPTOR_LENGTH = 32;
const quint8 FIELD_NAME_LENGTH = 10;
const quint8 FIELD_LENGTH_OFFSET = 16;

const quint8 MEMO_BLOCK_LENGTH_OFFSET = 6;
const quint16 MEMO_DBT_BLOCK_LENGTH = 512;
const quint8 MEMO_SIGNATURE_TEXT = 1;

const quint8 FIELD_TYPE_CHARACTER = 0x43;      // C
const quint8 FIELD_TYPE_DATE = 0x44;           // D
const quint8 FIELD_TYPE_FLOATING_POINT = 0x46; // F
const quint8 FIELD_TYPE_LOGICAL = 0x4C;        // L
const quint8 FIELD_TYPE_MEMO = 0x4D;           // M
const quint8 FIELD_TYPE_NUMBER = 0x4E;         // N
const quint8 FIELD_TYPE_INTEGER = 0x49;        // I
const quint8 FIELD_TYPE_DATE_TIME = 0x54;      // T

const quint8 CODEPAGE_NOT_SET = 0x00;
const quint8 CODEPAGE_US_MSDOS = 0x01;
const quint8 CODEPAGE_INTERNATIONAL_MSDOD = 0x02;
const quint8 CODEPAGE_RUSSIAN_OEM = 0x26;
const quint8 CODEPAGE_RUSSIAN_MSDOS = 0x65;
const quint8 CODEPAGE_EASTERN_EUROPEAN_WINDOWS = 0xC8;
const quint8 CODEPAGE_RUSSIAN_WINDOWS = 0xC9;
const quint8 CODEPAGE_WINDOWS_ANSI_LATIN_1 = 0x03;
const quint8 CODEPAGE_GB18030 = 0x7A;

const quint8 LOGICAL_UNDEFINED = 0x3F; // ?
const quint8 LOGICAL_YES = 0x59;       // Y
const quint8 LOGICAL_NO = 0x4E;        // N
const quint8 LOGICAL_TRUE = 0x54;      // T
const quint8 LOGICAL_FALSE = 0x46;     // F
const quint8 FIELD_DELETED = 0x2A;     // *
const quint8 FIELD_SPACER = 0x20;
const quint8 FIELD_NAME_SPACER = 0x00;
const quint8 END_OF_FILE_MARK = 0x1A;
const char END_OF_DBASE_MEMO_BLOCK[] = { 0x1A, 0x1A };

const quint8 DATE_LENGTH = 8;
const quint8 YEAR_OFFSET = 0;
const quint8 YEAR_LENGTH = 4;
const quint8 MONTH_OFFSET = 4;
const quint8 MONTH_LENGTH = 2;
const quint8 DAY_OFFSET = 6;
const quint8 DAY_LENGTH = 2;

const quint8 TIME_LENGTH = 6;
const quint8 HOUR_OFFSET = 0;
const quint8 HOUR_LENGTH = 2;
const quint8 MINUTE_OFFSET = 2;
const quint8 MINUTE_LENGTH = 2;
const quint8 SECOND_OFFSET = 4;
const quint8 SECOND_LENGTH = 2;

const quint8 DATETIME_LENGTH = 14;
const quint8 DATETIME_DATE_OFFSET = 0;
const quint8 DATETIME_TIME_OFFSET = 8;

const quint8 TIMESTAMP_LENGTH = 8;


namespace QDbf {
namespace Internal {

class QDbfTablePrivate final
{
public:
    explicit QDbfTablePrivate(QString &&dbfFileName);

    enum QDbfMemoType {
        NoMemo,
        DBaseMemo,
        DBaseIVMemo,
        FoxProMemo
    };

    enum Location {
        BeforeFirstRow = -1,
        FirstRow = 0
    };

    void clear();
    bool openMemoFile();
    QVariant memoFieldValue(int index) const;
    QDataStream::ByteOrder memoByteOrder() const;
    bool setCodepage(QDbfTable::Codepage codepage);
    void setDefaultCodepage(QDbfTable::Codepage codepage);
    bool isValueValid(int i, const QVariant &value) const;
    void setTextCodec();
    bool setValue(int fieldIndex, const QVariant &value);
    void setLastUpdate();

    static QDate dateFromByteArray(const QByteArray &byteArray);
    static QTime timeFromByteArray(const QByteArray &byteArray);

    QString m_tableFileName;
    QTextCodec *m_textCodec;
    mutable QFile m_tableFile;
    mutable QFile m_memoFile;
    QDate m_lastUpdate;
    mutable QDbfTable::DbfTableError m_error = QDbfTable::NoError;
    QDbfTable::OpenMode m_openMode = QDbfTable::ReadOnly;
    QDbfMemoType m_memoType = QDbfTablePrivate::NoMemo;
    QDbfTable::Codepage m_codepage = QDbfTable::CodepageNotSet;
    QDbfTable::Codepage m_defaultCodepage = QDbfTable::CodepageNotSet;
    mutable QDbfRecord m_currentRecord;
    QDbfRecord m_record;
    quint16 m_headerLength = 0;
    quint16 m_recordLength = 0;
    quint16 m_fieldsCount = 0;
    qint16 m_memoBlockLength = 0;
    qint32 m_memoNextFreeBlockIndex = 0;
    qint32 m_recordsCount = 0;
    mutable qint32 m_currentIndex = 0;
    mutable bool m_bufered = false;
    bool m_dbc = false;
};


QDbfTablePrivate::QDbfTablePrivate(QString &&dbfFileName) :
    m_tableFileName(std::move(dbfFileName)),
    m_textCodec(QTextCodec::codecForLocale())
{
}


void QDbfTablePrivate::clear()
{
    m_error = QDbfTable::NoError;
    m_openMode = QDbfTable::ReadOnly;
    m_dbc = false;
    m_memoType = QDbfTablePrivate::NoMemo;
    m_codepage = QDbfTable::CodepageNotSet;
    m_headerLength = 0;
    m_recordLength = 0;
    m_fieldsCount = 0;
    m_recordsCount = 0;
    m_memoNextFreeBlockIndex = 0;
    m_memoBlockLength = 0;
    m_currentIndex = 0;
    m_bufered = false;
    m_currentRecord = QDbfRecord();
    m_record = QDbfRecord();
}


bool QDbfTablePrivate::openMemoFile()
{
    QString memoFileExtension;
    switch (m_memoType) {
    case QDbfTablePrivate::DBaseMemo:
    case QDbfTablePrivate::DBaseIVMemo:
        memoFileExtension = QLatin1String("dbt");
        break;
    case QDbfTablePrivate::FoxProMemo:
        memoFileExtension = QLatin1String("fpt");
        break;
    default:
        Q_ASSERT(false);
        m_error = QDbfTable::FileOpenError;
        return false;
    }

    QFileInfo tableFileInfo(m_tableFileName);
    const auto &tableDir = tableFileInfo.dir();
    const auto &baseName = tableFileInfo.baseName();
    const auto &filter = QString(QLatin1String("%1.%2")).arg(baseName, memoFileExtension);
    const auto &entries = tableDir.entryList(QStringList(filter), QDir::Files);
    if (entries.isEmpty()) {
        return false;
    }
    const auto &memoFileName = QString(QLatin1String("%1/%2")).arg(tableDir.canonicalPath(), entries.first());
    m_memoFile.setFileName(memoFileName);

    auto fileOpenMode = (m_openMode == QDbfTable::ReadWrite) ? QIODevice::ReadWrite : QIODevice::ReadOnly;
    if (!QFile::exists(memoFileName) || !m_memoFile.open(fileOpenMode)) {
        m_error = QDbfTable::FileOpenError;
        return false;
    }

    QDataStream stream(&m_memoFile);
    stream.setByteOrder(memoByteOrder());

    stream >> m_memoNextFreeBlockIndex;

    if (QDbfTablePrivate::FoxProMemo == m_memoType) {
        stream.device()->seek(MEMO_BLOCK_LENGTH_OFFSET);
        stream >> m_memoBlockLength;
        if (m_memoBlockLength < 1) {
            m_memoBlockLength = 1;
        }
    } else {
        m_memoBlockLength = MEMO_DBT_BLOCK_LENGTH;
    }

    m_error = QDbfTable::NoError;
    return true;
}


QVariant QDbfTablePrivate::memoFieldValue(int index) const
{
    Q_ASSERT(m_memoFile.isOpen() && m_memoFile.isReadable());

    auto position = qint64(m_memoBlockLength) * index;

    if (QDbfTablePrivate::DBaseMemo == m_memoType) {
        QByteArray data;
        forever {
            if (!m_memoFile.seek(position)) {
                m_error = QDbfTable::FileReadError;
                return QVariant::Invalid;
            }
            const auto &block = m_memoFile.read(m_memoBlockLength);
            if (block.isEmpty()) {
                m_error = QDbfTable::FileReadError;
                return QVariant::Invalid;
            }
            auto endOfBlockPosition = block.indexOf(END_OF_DBASE_MEMO_BLOCK);
            if (endOfBlockPosition == -1) {
                data.append(block);
                position += m_memoBlockLength;
            } else {
                data.append(block.left(endOfBlockPosition));
                break;
            }
        }
        return m_textCodec->toUnicode(data);
    }

    QDataStream stream(&m_memoFile);
    stream.setByteOrder(memoByteOrder());
    if (!stream.device()->seek(position)) {
        m_error = QDbfTable::FileReadError;
        return QVariant::Invalid;
    }

    qint32 signature;
    stream >> signature;

    qint32 dataLength;
    stream >> dataLength;

    const auto &data = m_memoFile.read(dataLength);
    if (0 < dataLength && data.isEmpty()) {
        return QVariant::Invalid;
    }

    if (MEMO_SIGNATURE_TEXT == signature) {
        return m_textCodec->toUnicode(data);
    }

    m_error = QDbfTable::NoError;
    return data;
}


QDataStream::ByteOrder QDbfTablePrivate::memoByteOrder() const
{
    return (m_memoType == QDbfTablePrivate::DBaseIVMemo) ? QDataStream::LittleEndian : QDataStream::BigEndian;
}


void QDbfTablePrivate::setDefaultCodepage(QDbfTable::Codepage codepage)
{
    m_defaultCodepage = codepage;
}


bool QDbfTablePrivate::setCodepage(QDbfTable::Codepage codepage)
{
    if (!m_tableFile.isOpen() || !m_tableFile.isWritable()) {
        m_error = QDbfTable::FileWriteError;
        return false;
    }

    m_tableFile.seek(CODEPAGE_OFFSET);
    quint8 byte;
    switch(codepage) {
    case QDbfTable::CodepageNotSet:
        byte = CODEPAGE_NOT_SET;
        break;
    case QDbfTable::IBM437:
        byte = CODEPAGE_US_MSDOS;
        break;
    case QDbfTable::IBM850:
        byte = CODEPAGE_INTERNATIONAL_MSDOD;
        break;
    case QDbfTable::IBM866:
        byte = CODEPAGE_RUSSIAN_OEM;
        break;
    case QDbfTable::Windows1250:
        byte = CODEPAGE_EASTERN_EUROPEAN_WINDOWS;
        break;
    case QDbfTable::Windows1251:
        byte = CODEPAGE_RUSSIAN_WINDOWS;
        break;
    case QDbfTable::Windows1252:
        byte = CODEPAGE_WINDOWS_ANSI_LATIN_1;
        break;
    case QDbfTable::GB18030:
        byte = CODEPAGE_GB18030;
        break;
    default:
        return false;
    }

    if (1 != m_tableFile.write(reinterpret_cast<char *>(&byte), 1)) {
        m_error = QDbfTable::FileWriteError;
        return false;
    }

    m_codepage = codepage;
    setTextCodec();

    m_error = QDbfTable::NoError;
    return true;
}


bool QDbfTablePrivate::isValueValid(int i, const QVariant &value) const
{
    switch (m_currentRecord.field(i).type()) {
    case QDbfField::Character:
        return value.canConvert(QVariant::String);
    case QDbfField::Date:
        return value.canConvert(QVariant::Date);
    case QDbfField::FloatingPoint:
    case QDbfField::Number:
        return value.canConvert(QVariant::Double);
    case QDbfField::Logical:
        return value.canConvert(QVariant::Bool);
    case QDbfField::Memo:
        return m_memoType != QDbfTablePrivate::NoMemo && value.canConvert(QVariant::String);
    case QDbfField::Integer:
        return value.canConvert(QVariant::Int);
    case QDbfField::DateTime:
        return value.canConvert(QVariant::DateTime);
    default:
        return false;
    }
}


void QDbfTablePrivate::setTextCodec()
{
    switch (m_codepage) {
    case QDbfTable::Windows1250:
        m_textCodec = QTextCodec::codecForName("Windows-1250");
        break;
    case QDbfTable::Windows1251:
        m_textCodec = QTextCodec::codecForName("Windows-1251");
        break;
    case QDbfTable::Windows1252:
        m_textCodec = QTextCodec::codecForName("Windows-1252");
        break;
    case QDbfTable::IBM850:
        m_textCodec = QTextCodec::codecForName("IBM 850");
        break;
    case QDbfTable::IBM866:
        m_textCodec = QTextCodec::codecForName("IBM 866");
        break;
    case QDbfTable::GB18030:
        m_textCodec = QTextCodec::codecForName("GB18030");
        break;
    default:
        m_textCodec = QTextCodec::codecForLocale();
        break;
    }
}


QDate QDbfTablePrivate::dateFromByteArray(const QByteArray &byteArray)
{
    Q_ASSERT(DATE_LENGTH == byteArray.length());

    auto ok = false;

    auto y = byteArray.mid(YEAR_OFFSET, YEAR_LENGTH).toInt(&ok);
    if (!ok) {
        return {};
    }

    auto m = byteArray.mid(MONTH_OFFSET, MONTH_LENGTH).toInt(&ok);
    if (!ok) {
        return {};
    }

    auto d = byteArray.mid(DAY_OFFSET, DAY_LENGTH).toInt(&ok);
    if (!ok) {
        return {};
    }

    return { y, m, d };
}


QTime QDbfTablePrivate::timeFromByteArray(const QByteArray &byteArray)
{
    Q_ASSERT(TIME_LENGTH == byteArray.length());

    auto ok = false;

    auto h = byteArray.mid(HOUR_OFFSET, HOUR_LENGTH).toInt(&ok);
    if (!ok) {
        return {};
    }

    auto m = byteArray.mid(MINUTE_OFFSET, MINUTE_LENGTH).toInt(&ok);
    if (!ok) {
        return {};
    }

    auto s = byteArray.mid(SECOND_OFFSET, SECOND_LENGTH).toInt(&ok);
    if (!ok) {
        return {};
    }

    return { h, m, s };
}


bool QDbfTablePrivate::setValue(int fieldIndex, const QVariant &value)
{
    if (!m_tableFile.isOpen() || !m_tableFile.isWritable()) {
        m_error = QDbfTable::FileWriteError;
        return false;
    }

    if (!m_record.contains(fieldIndex)) {
        m_error = QDbfTable::InvalidIndexError;
        return false;
    }

    if (!isValueValid(fieldIndex, value)) {
        m_error = QDbfTable::InvalidTypeError;
        return false;
    }

    QByteArray data;
    QByteArray memoData;
    qint32 memoBlockCount = 0;

    switch (m_record.field(fieldIndex).type()) {
    case QDbfField::Character:
        data = m_textCodec->fromUnicode(value.toString()
                                        .leftJustified(m_record.field(fieldIndex).length(), QLatin1Char(FIELD_SPACER), true));
        break;
    case QDbfField::Date:
        data = value.toDate().toString(QLatin1String("yyyyMMdd"))
               .leftJustified(m_record.field(fieldIndex).length(), QLatin1Char(FIELD_SPACER), true).toLatin1();
        break;
    case QDbfField::FloatingPoint:
    case QDbfField::Number:
        data = QString::number(value.toDouble(), 'f', m_record.field(fieldIndex).precision())
               .rightJustified(m_record.field(fieldIndex).length(), QLatin1Char(FIELD_SPACER), true).toLatin1();
        break;
    case QDbfField::Logical:
        data = QByteArray(1, value.toBool() ? LOGICAL_TRUE : LOGICAL_FALSE);
        break;
    case QDbfField::Memo: {
        const auto &val = m_textCodec->fromUnicode(value.toString());
        if (val.isEmpty()) {
            data = QString().rightJustified(m_record.field(fieldIndex).length(), QLatin1Char(FIELD_SPACER), true).toLatin1();
        } else {
            switch (m_memoType) {
            case Internal::QDbfTablePrivate::DBaseIVMemo:
            case Internal::QDbfTablePrivate::FoxProMemo: {
                QDataStream stream(&memoData, QIODevice::WriteOnly);
                stream.setByteOrder(memoByteOrder());
                auto signature = quint32(MEMO_SIGNATURE_TEXT);
                stream << signature;
                auto valLength = qint32(val.length());
                stream << valLength;
            }
            case Internal::QDbfTablePrivate::DBaseMemo:
                memoData.append(val);
                break;
            default:
                m_error = QDbfTable::UnsupportedFile;
                return false;
            }

            if (10 == m_record.field(fieldIndex).length()) {
                data = QString::number(m_memoNextFreeBlockIndex)
                       .rightJustified(m_record.field(fieldIndex).length(), QLatin1Char(FIELD_SPACER), true).toLatin1();
            } else if (4 == m_record.field(fieldIndex).length()) {
                QDataStream stream(&data, QIODevice::WriteOnly);
                stream.setByteOrder(QDataStream::LittleEndian);
                stream << m_memoNextFreeBlockIndex;
            } else {
                m_error = QDbfTable::UnsupportedFile;
                return false;
            }

            memoBlockCount = memoData.length() / m_memoBlockLength + (0 < (memoData.length() % m_memoBlockLength) ? 1 : 0);
        }
        break;
    }
    case QDbfField::Integer: {
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        auto val = qint32(value.toInt());
        stream << val;
        break;
    }
    case QDbfField::DateTime: {
        const auto &val = value.toDateTime();
        if (!val.isValid()) {
            m_error = QDbfTable::InvalidValue;
            return false;
        }

        if (DATETIME_LENGTH == m_record.field(fieldIndex).length()) {
            data = val.toString(QLatin1String("yyyyMMddHHmmss"))
                   .leftJustified(m_record.field(fieldIndex).length(), QLatin1Char(FIELD_SPACER), true).toLatin1();
        } else if (m_record.field(fieldIndex).length() == TIMESTAMP_LENGTH) {
            const auto julianDay = val.date().toJulianDay();
            if (std::numeric_limits<qint32>::max() < julianDay) {
                m_error = QDbfTable::InvalidValue;
                return false;
            }
            const auto day = qint32(julianDay);
#if QT_VERSION < 0x050200
            auto msecs = QTime(0, 0, 0, 0).msecsTo(val.time());
#else
            auto msecs = val.time().msecsSinceStartOfDay();
#endif
            QDataStream stream(&data, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream << day;
            stream << msecs;
        } else {
            m_error = QDbfTable::UnsupportedFile;
            return false;
        }
        break;
    }
    default:
        data = QString().leftJustified(m_record.field(fieldIndex).length(), QLatin1Char(FIELD_SPACER), true).toLatin1();
        break;
    }

    if (0 < memoBlockCount) {
        Q_ASSERT(0 < memoData.length() && memoData.length() <= memoBlockCount * m_memoBlockLength);

        if (!m_memoFile.isOpen() || !m_memoFile.isWritable()) {
            m_error = QDbfTable::FileWriteError;
            return false;
        }

        auto position = qint64(m_memoBlockLength) * m_memoNextFreeBlockIndex;
        if (!m_memoFile.seek(position)) {
            m_error = QDbfTable::FileReadError;
            return false;
        }

        if (m_memoFile.write(memoData) != memoData.length()) {
            m_error = QDbfTable::FileWriteError;
            return false;
        }

        QDataStream stream(&m_memoFile);
        stream.setByteOrder(memoByteOrder());
        if (!stream.device()->seek(0)) {
            m_error = QDbfTable::FileReadError;
            return false;
        }
        m_memoNextFreeBlockIndex += memoBlockCount;
        stream << m_memoNextFreeBlockIndex;
    }

    auto position = qint64(m_recordLength) * m_currentIndex +
                    m_headerLength + m_record.field(fieldIndex).offset();

    if (!m_tableFile.seek(position)) {
        m_error = QDbfTable::FileReadError;
        return false;
    }

    if (m_tableFile.write(data) != data.length()) {
        m_error = QDbfTable::FileWriteError;
        return false;
    }

    m_currentRecord.setValue(fieldIndex, value);
    m_error = QDbfTable::NoError;
    return true;
}


void QDbfTablePrivate::setLastUpdate()
{
    const auto &date = QDate::currentDate();
    if (date == m_lastUpdate) {
        return;
    }

    if (!m_tableFile.isOpen() || !m_tableFile.isWritable()) {
        return;
    }

    QDataStream stream(&m_tableFile);
    stream.setByteOrder(QDataStream::LittleEndian);
    if (!stream.device()->seek(TABLE_LAST_UPDATE_OFFSET)) {
        return;
    }

    auto year = quint8(date.year() - (date.year() >= 2000 ? 2000 : 1900));
    stream << year;

    auto month = quint8(date.month());
    stream << month;

    auto day = quint8(date.day());
    stream << day;

    m_lastUpdate = date;
}

} // namespace Internal


QDbfTable::QDbfTable(QString dbfFileName) :
    d(new Internal::QDbfTablePrivate(std::move(dbfFileName)))
{
}


QDbfTable::QDbfTable(QDbfTable &&other) Q_DECL_NOEXCEPT :
    d(other.d)
{
    other.d = nullptr;
}


QDbfTable &QDbfTable::operator=(QDbfTable &&other) Q_DECL_NOEXCEPT
{
    other.swap(*this);
    return *this;
}


QDbfTable::~QDbfTable()
{
    delete d;
    d = nullptr;
}


QString QDbfTable::fileName() const
{
    return d->m_tableFile.fileName();
}


QDbfTable::OpenMode QDbfTable::openMode() const
{
    return d->m_openMode;
}


QDbfTable::DbfTableError QDbfTable::error() const
{
    return d->m_error;
}


bool QDbfTable::open(QString fileName, OpenMode openMode)
{
    d->m_tableFileName = std::move(fileName);
    return open(openMode);
}


void QDbfTable::close()
{
    d->clear();
    d->m_tableFile.close();
    d->m_memoFile.close();
}


bool QDbfTable::open(OpenMode openMode)
{
    close();
    d->m_openMode = openMode;

    QFileInfo fileInfo(d->m_tableFileName);
    d->m_tableFile.setFileName(fileInfo.canonicalFilePath());

    auto fileOpenMode = (d->m_openMode == QDbfTable::ReadWrite) ? QIODevice::ReadWrite : QIODevice::ReadOnly;
    if (!d->m_tableFile.exists() || !d->m_tableFile.open(fileOpenMode)) {
        d->m_error = QDbfTable::FileOpenError;
        return false;
    }

    QDataStream stream(&d->m_tableFile);
    stream.setByteOrder(QDataStream::LittleEndian);

    auto memoType = Internal::QDbfTablePrivate::NoMemo;

    // Table version
    quint8 version;
    stream >> version;
    switch(version) {
    case 0x02:
    case 0x03:
    case 0x04:
        break;
    case 0x30:
    case 0x31:
        memoType = Internal::QDbfTablePrivate::FoxProMemo;
        d->m_dbc = true;
        break;
    case 0x83:
        memoType = Internal::QDbfTablePrivate::DBaseMemo;
        break;
    case 0x8B:
    case 0x8C:
        memoType = Internal::QDbfTablePrivate::DBaseIVMemo;
        break;
    case 0xF5:
        memoType = Internal::QDbfTablePrivate::FoxProMemo;
        break;
    default:
        d->m_error = QDbfTable::UnsupportedFile;
        return false;
    }

    // Last update
    quint8 y;
    stream >> y;
    quint8 month;
    stream >> month;
    quint8 day;
    stream >> day;

    auto year = (y < 80 ? 2000 : 1900) + y;
    d->m_lastUpdate = QDate::fromString(QString(QLatin1String("%1%2%3")).arg(year).arg(month).arg(day), QLatin1String("yyyyMd"));

    // Number of records
    if (!d->m_tableFile.seek(TABLE_RECORDS_COUNT_OFFSET)) {
        d->m_error = QDbfTable::FileReadError;
        return false;
    }

    quint32 recordsCount;
    stream >> recordsCount;
    d->m_recordsCount = qint32(recordsCount);

    // Length of header structure
    if (!d->m_tableFile.seek(TABLE_FIRST_RECORD_POSITION_OFFSET)) {
        d->m_error = QDbfTable::FileReadError;
        return false;
    }
    stream >> d->m_headerLength;

    // Length of each record
    if (!d->m_tableFile.seek(RECORD_LENGTH_OFFSET)) {
        d->m_error = QDbfTable::FileReadError;
        return false;
    }
    stream >> d->m_recordLength;

    // Codepage
    if (!d->m_tableFile.seek(CODEPAGE_OFFSET)) {
        d->m_error = QDbfTable::FileReadError;
        return false;
    }

    quint8 codepage;
    stream >> codepage;
    switch(codepage) {
    case CODEPAGE_NOT_SET:
        d->m_codepage = d->m_defaultCodepage;
        break;
    case CODEPAGE_US_MSDOS:
        d->m_codepage = QDbfTable::IBM437;
        break;
    case CODEPAGE_INTERNATIONAL_MSDOD:
        d->m_codepage = QDbfTable::IBM850;
        break;
    case CODEPAGE_WINDOWS_ANSI_LATIN_1:
        d->m_codepage = QDbfTable::Windows1252;
        break;
    case CODEPAGE_RUSSIAN_OEM:
    case CODEPAGE_RUSSIAN_MSDOS:
        d->m_codepage = QDbfTable::IBM866;
        break;
    case CODEPAGE_EASTERN_EUROPEAN_WINDOWS:
        d->m_codepage = QDbfTable::Windows1250;
        break;
    case CODEPAGE_RUSSIAN_WINDOWS:
        d->m_codepage = QDbfTable::Windows1251;
        break;
    case CODEPAGE_GB18030:
        d->m_codepage = QDbfTable::GB18030;
        break;
    default:
        d->m_codepage = QDbfTable::UnsupportedCodepage;
        break;
    }
    d->setTextCodec();


    auto fieldDescriptorsLength = quint16(d->m_headerLength - TABLE_DESCRIPTOR_LENGTH - TERMINATOR_LENGTH);
    if (d->m_dbc) {
        fieldDescriptorsLength -= DBC_LENGTH;
    }

    d->m_fieldsCount = fieldDescriptorsLength / FIELD_DESCRIPTOR_LENGTH;

    auto fieldOffset = 1;
    for (auto i = 0; i < d->m_fieldsCount; ++i) {
        auto fieldDescriptorOffset = qint32(FIELD_DESCRIPTOR_LENGTH) * i + TABLE_DESCRIPTOR_LENGTH;

        // Field name
        if (!d->m_tableFile.seek(fieldDescriptorOffset)) {
            d->m_error = QDbfTable::FileReadError;
            return false;
        }
        QByteArray fieldName;
        for (auto j = 0; j <= FIELD_NAME_LENGTH; ++j) {
            qint8 fieldNameChar;
            stream >> fieldNameChar;
            if (FIELD_NAME_SPACER != fieldNameChar) {
                fieldName.append(fieldNameChar);
            }
        }

        // Field type
        quint8 fieldTypeChar;
        stream >> fieldTypeChar;
        QDbfField::QDbfType fieldType;
        QVariant defaultValue;
        switch (fieldTypeChar) {
        case FIELD_TYPE_CHARACTER:
            fieldType = QDbfField::Character;
            defaultValue = QString();
            break;
        case FIELD_TYPE_DATE:
            fieldType = QDbfField::Date;
            defaultValue = QDate();
            break;
        case FIELD_TYPE_FLOATING_POINT:
            fieldType = QDbfField::FloatingPoint;
            defaultValue = 0;
            break;
        case FIELD_TYPE_LOGICAL:
            fieldType = QDbfField::Logical;
            defaultValue = false;
            break;
        case FIELD_TYPE_MEMO:
            fieldType = QDbfField::Memo;
            defaultValue = QString();
            d->m_memoType = memoType;
            break;
        case FIELD_TYPE_NUMBER:
            fieldType = QDbfField::Number;
            defaultValue = 0;
            break;
        case FIELD_TYPE_INTEGER:
            fieldType = QDbfField::Integer;
            defaultValue = 0;
            break;
        case FIELD_TYPE_DATE_TIME:
            fieldType = QDbfField::DateTime;
            defaultValue = QDateTime();
            break;
        default:
            fieldType = QDbfField::Undefined;
            defaultValue = QVariant::Invalid;
            break;
        }

        // Field length
        if (!d->m_tableFile.seek(fieldDescriptorOffset + FIELD_LENGTH_OFFSET)) {
            d->m_error = QDbfTable::FileReadError;
            return false;
        }
        quint8 fieldLength;
        stream >> fieldLength;

        // Decimal count
        quint8 fieldPrecision;
        stream >> fieldPrecision;

        // Build field
        QDbfField field(d->m_textCodec->toUnicode(fieldName));
        field.setType(fieldType);
        field.setLength(fieldLength);
        field.setPrecision(fieldPrecision);
        field.setOffset(fieldOffset);
        field.setDefaultValue(defaultValue);
        field.setValue(defaultValue);
        d->m_record.append(field);

        fieldOffset += fieldLength;
    }

    if (Internal::QDbfTablePrivate::NoMemo != d->m_memoType) {
        return d->openMemoFile();
    }

    return true;
}


void QDbfTable::setDefaultCodepage(QDbfTable::Codepage codepage)
{
    d->setDefaultCodepage(codepage);
}


bool QDbfTable::setCodepage(QDbfTable::Codepage codepage)
{
    return d->setCodepage(codepage);
}


QDbfTable::Codepage QDbfTable::codepage() const
{
    return d->m_codepage;
}


bool QDbfTable::isOpen() const
{
    return d->m_tableFile.isOpen();
}


int QDbfTable::size() const
{
    return d->m_recordsCount;
}


int QDbfTable::at() const
{
    return d->m_currentIndex;
}


bool QDbfTable::previous() const
{
    if (at() <= Internal::QDbfTablePrivate::FirstRow) {
        return false;
    }

    if (at() > (d->m_recordsCount - 1)) {
        return last();
    }

    return seek(at() - 1);
}


bool QDbfTable::next() const
{
    if (at() < Internal::QDbfTablePrivate::FirstRow) {
        return first();
    }

    if (at() >= (d->m_recordsCount - 1)) {
        return false;
    }

    return seek(at() + 1);
}


bool QDbfTable::first() const
{
    return seek(Internal::QDbfTablePrivate::FirstRow);
}


bool QDbfTable::last() const
{
    return seek(d->m_recordsCount - 1);
}


bool QDbfTable::seek(int index) const
{
    auto previousIndex = d->m_currentIndex;

    if (index < Internal::QDbfTablePrivate::FirstRow) {
        d->m_currentIndex = Internal::QDbfTablePrivate::BeforeFirstRow;
    } else if (index > (d->m_recordsCount - 1)) {
        d->m_currentIndex = d->m_recordsCount - 1;
    } else {
        d->m_currentIndex = index;
    }

    if (previousIndex != d->m_currentIndex) {
        d->m_bufered = false;
    }

    return true;
}


QDate QDbfTable::lastUpdate() const
{
    return d->m_lastUpdate;
}


bool QDbfTable::setRecord(const QDbfRecord &record)
{
    if (record.isDeleted() && !removeRecord(d->m_currentIndex)) {
        return false;
    }

    for (auto i = 0; i < record.count(); ++i) {
        const auto &field = record.field(i);
        if (!d->setValue(i, field.value())) {
            return false;
        }
    }

    d->setLastUpdate();

    return true;
}


QDbfRecord QDbfTable::record() const
{
    if (d->m_bufered) {
        return d->m_currentRecord;
    }

    d->m_currentRecord = d->m_record;

    if (d->m_currentIndex < Internal::QDbfTablePrivate::FirstRow) {
        return d->m_currentRecord;
    }

    if (!d->m_tableFile.isOpen()) {
        d->m_error = QDbfTable::FileReadError;
        return d->m_currentRecord;
    }

    auto position = qint64(d->m_recordLength) * d->m_currentIndex + d->m_headerLength;

    if (!d->m_tableFile.seek(position)) {
        d->m_error = QDbfTable::FileReadError;
        return d->m_currentRecord;
    }

    d->m_currentRecord.setRecordIndex(d->m_currentIndex);

    const auto &recordData = d->m_tableFile.read(d->m_recordLength);
    if (recordData.length() != d->m_recordLength) {
        d->m_error = QDbfTable::FileReadError;
        return d->m_record;
    }

    d->m_currentRecord.setDeleted(FIELD_DELETED == recordData.at(0));

    for (auto i = 0; i < d->m_currentRecord.count(); ++i) {
        const auto &byteArray = recordData.mid(d->m_currentRecord.field(i).offset(), d->m_currentRecord.field(i).length());
        QVariant value;
        switch (d->m_currentRecord.field(i).type()) {
        case QDbfField::Character:
            value = d->m_textCodec->toUnicode(byteArray);
            break;
        case QDbfField::Date:
            value = QVariant(d->dateFromByteArray(byteArray));
            break;
        case QDbfField::FloatingPoint:
        case QDbfField::Number:
            if (0 == d->m_record.field(i).precision()) {
                value = byteArray.trimmed().toInt();
            } else {
                value = byteArray.trimmed().toDouble();
            }
            break;
        case QDbfField::Logical: {
            const auto &val = QString::fromLatin1(byteArray.toUpper());
            if (val == QString(QLatin1Char(LOGICAL_UNDEFINED))) {
                value = QVariant::Bool;
            } else if (val == QString(QLatin1Char(LOGICAL_TRUE)) || val == QString(QLatin1Char(LOGICAL_YES))) {
                value = true;
            } else if (val == QString(QLatin1Char(LOGICAL_FALSE)) || val == QString(QLatin1Char(LOGICAL_NO))) {
                value = false;
            } else {
                value = QVariant::Invalid;
            }
            break;
        }
        case QDbfField::Memo:
            if (d->m_memoType == Internal::QDbfTablePrivate::NoMemo) {
                value = QVariant::Invalid;
            } else if (10 == byteArray.length()) {
                if (!byteArray.trimmed().isEmpty()) {
                    auto ok = false;
                    auto index = QString::fromLatin1(byteArray).toInt(&ok);
                    value = ok ? d->memoFieldValue(index) : QVariant::Invalid;
                } else {
                    value = QVariant::String;
                }
            } else if (4 == byteArray.length()) {
                QDataStream stream(byteArray);
                stream.setByteOrder(QDataStream::LittleEndian);
                qint32 index;
                stream >> index;
                value = d->memoFieldValue(index);
            } else {
                value = QVariant::Invalid;
            }
            break;
        case QDbfField::Integer: {
            QDataStream stream(byteArray);
            stream.setByteOrder(QDataStream::LittleEndian);
            qint32 val;
            stream >> val;
            value = val;
            break;
        }
        case QDbfField::DateTime: {
            if (DATETIME_LENGTH == byteArray.length()) {
                const auto &date = d->dateFromByteArray(byteArray.mid(DATETIME_DATE_OFFSET, DATE_LENGTH));
                const auto &time = d->timeFromByteArray(byteArray.mid(DATETIME_TIME_OFFSET, TIME_LENGTH));
                value = QVariant(QDateTime(date, time));
            } else if (TIMESTAMP_LENGTH == byteArray.length()) {
                QDataStream stream(byteArray);
                stream.setByteOrder(QDataStream::LittleEndian);
                qint32 day;
                stream >> day;
                qint32 msecs;
                stream >> msecs;
                const auto &date = QDate::fromJulianDay(day);
#if QT_VERSION < 0x050200
                const auto &time = QTime(0, 0, 0, 0).addMSecs(msecs);
#else
                const auto &time = QTime::fromMSecsSinceStartOfDay(msecs);
#endif
                value = QVariant(QDateTime(date, time));
            } else {
                value = QVariant::Invalid;
            }
            break;
        }
        default:
            value = QVariant::Invalid;
            break;
        }

        d->m_currentRecord.setValue(i, value);
    }

    d->m_bufered = true;
    d->m_error = QDbfTable::NoError;
    return d->m_currentRecord;
}


bool QDbfTable::setValue(int fieldIndex, const QVariant &value)
{
    if (d->setValue(fieldIndex, value)) {
        d->setLastUpdate();
        return true;
    }

    return false;
}


QVariant QDbfTable::value(int fieldIndex) const
{
    return record().value(fieldIndex);
}


bool QDbfTable::setValue(const QString &name, const QVariant &value)
{
    return setValue(d->m_record.indexOf(name), value);
}


QVariant QDbfTable::value(const QString &name) const
{
    return record().value(name);
}


bool QDbfTable::addRecord()
{
    QDbfRecord newRecord(record());
    newRecord.clearValues();
    newRecord.setDeleted(false);
    return addRecord(newRecord);
}


bool QDbfTable::addRecord(const QDbfRecord &record)
{
    if (!d->m_tableFile.isOpen() || !d->m_tableFile.isWritable()) {
        d->m_error = QDbfTable::FileWriteError;
        return false;
    }

    // Write new records count
    QDataStream stream(&d->m_tableFile);
    stream.setByteOrder(QDataStream::LittleEndian);
    if (!stream.device()->seek(TABLE_RECORDS_COUNT_OFFSET)) {
        d->m_error = QDbfTable::FileReadError;
        return false;
    }
    stream << ++d->m_recordsCount;

    // Write end of file mark
    auto position = qint64(d->m_recordLength) * d->m_recordsCount + d->m_headerLength;

    if (!d->m_tableFile.seek(position)) {
        d->m_error = QDbfTable::FileReadError;
        return false;
    }

    if (d->m_tableFile.write(QByteArray(1, END_OF_FILE_MARK)) != 1) {
        d->m_error = QDbfTable::FileWriteError;
        return false;
    }

    d->m_currentIndex = d->m_recordsCount;

    return setRecord(record);
}


bool QDbfTable::removeRecord(int index)
{
    if (!d->m_tableFile.isOpen() || !d->m_tableFile.isWritable()) {
        d->m_error = QDbfTable::FileWriteError;
        return false;
    }

    auto position = qint64(d->m_recordLength) * index + d->m_headerLength;

    if (!d->m_tableFile.seek(position)) {
        d->m_error = QDbfTable::FileReadError;
        return false;
    }

    if (d->m_tableFile.write(QByteArray(1, FIELD_DELETED)) != 1) {
        d->m_error = QDbfTable::FileWriteError;
        return false;
    }

    if (index == d->m_currentIndex) {
        d->m_currentRecord.setDeleted(true);
    }

    d->setLastUpdate();

    d->m_error = QDbfTable::NoError;
    return true;
}


bool QDbfTable::removeRecord()
{
    return removeRecord(d->m_currentIndex);
}


void QDbfTable::swap(QDbfTable &other) Q_DECL_NOEXCEPT
{
    std::swap(d, other.d);
}


void swap(QDbfTable &lhs, QDbfTable &rhs)
{
    lhs.swap(rhs);
}

} // namespace QDbf


QDebug operator<<(QDebug debug, const QDbf::QDbfTable &table)
{
    debug.nospace() << "QDbfTable("
                    << qPrintable(table.fileName()) << ", "
                    << "size: " << table.record().count()
                    << " x " << table.size() << ')';

    return debug.space();
}

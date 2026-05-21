// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_BRIDGE_RUSTQTCONVERSION_H
#define KIRIVIEW_BRIDGE_RUSTQTCONVERSION_H

#include "cache/imagebyteaccounting.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <rust/cxx.h>

namespace KiriView::Bridge {
inline rust::Str rustStr(const QByteArray &bytes)
{
    return rust::Str(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

inline rust::Slice<const std::uint8_t> rustBytes(const QByteArray &bytes)
{
    return rust::Slice<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t *>(bytes.constData()),
        static_cast<std::size_t>(bytes.size()));
}

inline QByteArray qtByteArray(const rust::Vec<std::uint8_t> &bytes)
{
    if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        return {};
    }

    return QByteArray(
        reinterpret_cast<const char *>(bytes.data()), static_cast<qsizetype>(bytes.size()));
}

inline QString qtString(const rust::String &value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

inline QStringList qtStringList(const rust::Vec<rust::String> &values)
{
    QStringList list;
    list.reserve(static_cast<qsizetype>(values.size()));
    for (const rust::String &value : values) {
        list.append(qtString(value));
    }

    return list;
}

inline std::int64_t rustByteSize(qsizetype byteSize) { return static_cast<std::int64_t>(byteSize); }

inline qsizetype qtByteSize(std::int64_t byteSize) { return saturatedQtByteSize(byteSize); }

template <typename Function> auto rustResultForQString(const QString &value, Function rustFunction)
{
    const QByteArray bytes = value.toUtf8();
    return rustFunction(rustStr(bytes));
}
}

#endif

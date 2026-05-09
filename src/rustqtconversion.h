// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_RUSTQTCONVERSION_H
#define KIRIVIEW_RUSTQTCONVERSION_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <cstddef>
#include <rust/cxx.h>

namespace KiriView::Bridge {
inline rust::Str rustStr(const QByteArray &bytes)
{
    return rust::Str(bytes.constData(), static_cast<std::size_t>(bytes.size()));
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

template <typename Function> auto rustResultForQString(const QString &value, Function rustFunction)
{
    const QByteArray bytes = value.toUtf8();
    return rustFunction(rustStr(bytes));
}
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageformatregistry.h"

#include "kiriview/src/imageformatregistry.cxx.h"

#include <QByteArray>
#include <QMimeDatabase>
#include <QMimeType>
#include <cstddef>

namespace {
rust::Str rustStringView(const QByteArray &bytes)
{
    return rust::Str(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

QString rustStringToQString(const rust::String &value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

QStringList rustStringsToQStringList(const rust::Vec<rust::String> &values)
{
    QStringList list;
    list.reserve(static_cast<qsizetype>(values.size()));
    for (const rust::String &value : values) {
        list.append(rustStringToQString(value));
    }

    return list;
}

QString rustStringForQString(const QString &value, rust::String (*rustFunction)(rust::Str value))
{
    const QByteArray bytes = value.toUtf8();
    return rustStringToQString(rustFunction(rustStringView(bytes)));
}

bool rustBoolForQString(const QString &value, bool (*rustFunction)(rust::Str value))
{
    const QByteArray bytes = value.toUtf8();
    return rustFunction(rustStringView(bytes));
}
}

namespace KiriView {
QStringList supportedImageExtensions()
{
    return rustStringsToQStringList(rustSupportedImageExtensions());
}

QStringList supportedOpenExtensions()
{
    return rustStringsToQStringList(rustSupportedOpenExtensions());
}

bool isSupportedImageFileName(const QString &name)
{
    return rustBoolForQString(name, rustIsSupportedImageFileName);
}

bool isComicBookArchiveFileName(const QString &name)
{
    return rustBoolForQString(name, rustIsComicBookArchiveFileName);
}

bool isComicBookArchiveUrl(const QUrl &url)
{
    return !comicBookArchiveKioSchemeForUrl(url).isEmpty();
}

QString comicBookArchiveKioSchemeForUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return {};
    }

    const QString extensionScheme
        = rustStringForQString(url.fileName(), rustComicBookArchiveKioSchemeForFileName);
    if (!extensionScheme.isEmpty()) {
        return extensionScheme;
    }

    const QMimeType mimeType
        = QMimeDatabase().mimeTypeForFile(url.toLocalFile(), QMimeDatabase::MatchExtension);
    return rustStringForQString(mimeType.name(), rustComicBookArchiveKioSchemeForMimeTypeName);
}

QString directArchiveOpenKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return rustStringForQString(mimeTypeName, rustDirectArchiveOpenKioSchemeForMimeTypeName);
}

QString directArchiveOpenKioSchemeForUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return {};
    }

    const QString extensionScheme
        = rustStringForQString(url.fileName(), rustDirectArchiveOpenKioSchemeForFileName);
    if (!extensionScheme.isEmpty()) {
        return extensionScheme;
    }

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(url.toLocalFile());
    return directArchiveOpenKioSchemeForMimeTypeName(mimeType.name());
}

QStringList openDialogNameFilters()
{
    const QString extensionFilter = QStringLiteral("*.") + supportedOpenExtensions().join(" *.");
    return {
        QStringLiteral("Image and comic book files (%1)").arg(extensionFilter),
        QStringLiteral("All files (*)"),
    };
}
}

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageformatregistry.h"

#include "archiveformat.h"
#include "kiriview/src/imageformatregistry.cxx.h"

#include <KLocalizedString>
#include <QByteArray>
#include <QMimeDatabase>
#include <QMimeType>
#include <cstddef>

namespace {
using ArchiveSchemeResolver = QString (*)(const QString &);

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

bool rustBoolForQString(const QString &value, bool (*rustFunction)(rust::Str value))
{
    const QByteArray bytes = value.toUtf8();
    return rustFunction(rustStringView(bytes));
}

QString archiveKioSchemeForUrl(const QUrl &url, QMimeDatabase::MatchMode mimeMatchMode,
    ArchiveSchemeResolver schemeForFileName, ArchiveSchemeResolver schemeForMimeTypeName)
{
    if (!url.isLocalFile()) {
        return {};
    }

    const QString extensionScheme = schemeForFileName(url.fileName());
    if (!extensionScheme.isEmpty()) {
        return extensionScheme;
    }

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(url.toLocalFile(), mimeMatchMode);
    return schemeForMimeTypeName(mimeType.name());
}
}

namespace KiriView {
QStringList supportedImageExtensions()
{
    return rustStringsToQStringList(rustSupportedImageExtensions());
}

QStringList supportedImageMimeTypes()
{
    return rustStringsToQStringList(rustSupportedImageMimeTypes());
}

QStringList supportedOpenExtensions()
{
    QStringList extensions = supportedImageExtensions();
    extensions.append(supportedComicBookArchiveExtensions());
    extensions.sort();
    return extensions;
}

bool isSupportedImageFileName(const QString &name)
{
    return rustBoolForQString(name, rustIsSupportedImageFileName);
}

bool isComicBookArchiveFileName(const QString &name)
{
    return !KiriView::comicBookArchiveKioSchemeForFileName(name).isEmpty();
}

bool isComicBookArchiveUrl(const QUrl &url)
{
    return !comicBookArchiveKioSchemeForUrl(url).isEmpty();
}

QString comicBookArchiveKioSchemeForUrl(const QUrl &url)
{
    return archiveKioSchemeForUrl(url, QMimeDatabase::MatchExtension,
        KiriView::comicBookArchiveKioSchemeForFileName,
        KiriView::comicBookArchiveKioSchemeForMimeTypeName);
}

QString directArchiveOpenKioSchemeForUrl(const QUrl &url)
{
    return archiveKioSchemeForUrl(url, QMimeDatabase::MatchDefault,
        KiriView::directArchiveOpenKioSchemeForFileName,
        KiriView::directArchiveOpenKioSchemeForMimeTypeName);
}

QStringList openDialogNameFilters()
{
    const QString extensionFilter = QStringLiteral("*.") + supportedOpenExtensions().join(" *.");
    return {
        ki18nc("@item:inlistbox", "Image and comic book files (%1)")
            .subs(extensionFilter)
            .toString(),
        i18nc("@item:inlistbox", "All files (*)"),
    };
}
}

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
#include <optional>

namespace {
using ArchiveMatchResolver = std::optional<KiriView::ArchiveOpenMatch> (*)(const QString &);

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

QString schemeForArchiveMatch(const std::optional<KiriView::ArchiveOpenMatch> &match)
{
    return match.has_value() ? match->scheme : QString();
}

std::optional<KiriView::ArchiveOpenMatch> archiveMatchForUrl(const QUrl &url,
    QMimeDatabase::MatchMode mimeMatchMode, ArchiveMatchResolver matchForFileName,
    ArchiveMatchResolver matchForMimeTypeName)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    std::optional<KiriView::ArchiveOpenMatch> extensionMatch = matchForFileName(url.fileName());
    if (extensionMatch.has_value()) {
        return extensionMatch;
    }

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(url.toLocalFile(), mimeMatchMode);
    return matchForMimeTypeName(mimeType.name());
}

QString archiveKioSchemeForUrl(const QUrl &url, QMimeDatabase::MatchMode mimeMatchMode,
    ArchiveMatchResolver matchForFileName, ArchiveMatchResolver matchForMimeTypeName)
{
    return schemeForArchiveMatch(
        archiveMatchForUrl(url, mimeMatchMode, matchForFileName, matchForMimeTypeName));
}

std::optional<KiriView::ArchiveOpenMatch> directArchiveOpenMatchForLocalUrl(const QUrl &url)
{
    return archiveMatchForUrl(url, QMimeDatabase::MatchDefault,
        KiriView::directArchiveOpenMatchForFileName,
        KiriView::directArchiveOpenMatchForMimeTypeName);
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
        KiriView::comicBookArchiveMatchForFileName, KiriView::comicBookArchiveMatchForMimeTypeName);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForUrl(const QUrl &url)
{
    return directArchiveOpenMatchForLocalUrl(url);
}

QString directArchiveOpenKioSchemeForUrl(const QUrl &url)
{
    return schemeForArchiveMatch(directArchiveOpenMatchForLocalUrl(url));
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

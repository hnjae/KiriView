// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformat.h"

#include "kiriview/src/imageformatregistry.cxx.h"

#include <QByteArray>
#include <optional>

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

template <typename Result, typename Function>
Result rustResultForQString(const QString &value, Function rustFunction)
{
    const QByteArray bytes = value.toUtf8();
    return rustFunction(rustStringView(bytes));
}

KiriView::ArchiveStorageBackend archiveStorageBackendFromRust(
    KiriView::RustArchiveStorageBackend backend)
{
    switch (backend) {
    case KiriView::RustArchiveStorageBackend::KZip:
        return KiriView::ArchiveStorageBackend::KZip;
    case KiriView::RustArchiveStorageBackend::KTar:
        return KiriView::ArchiveStorageBackend::KTar;
    case KiriView::RustArchiveStorageBackend::K7Zip:
        return KiriView::ArchiveStorageBackend::K7Zip;
    case KiriView::RustArchiveStorageBackend::LibArchive:
        return KiriView::ArchiveStorageBackend::LibArchive;
    case KiriView::RustArchiveStorageBackend::None:
        return KiriView::ArchiveStorageBackend::None;
    }

    return KiriView::ArchiveStorageBackend::None;
}

KiriView::ArchiveOpenMatchKind archiveOpenMatchKindFromRust(KiriView::RustArchiveOpenMatchKind kind)
{
    switch (kind) {
    case KiriView::RustArchiveOpenMatchKind::ComicBook:
        return KiriView::ArchiveOpenMatchKind::ComicBook;
    case KiriView::RustArchiveOpenMatchKind::GeneralArchive:
        return KiriView::ArchiveOpenMatchKind::GeneralArchive;
    }

    return KiriView::ArchiveOpenMatchKind::GeneralArchive;
}

std::optional<KiriView::ArchiveOpenMatch> archiveOpenMatchFromRust(
    const KiriView::RustArchiveOpenMatch &match)
{
    if (!match.found) {
        return std::nullopt;
    }

    return KiriView::ArchiveOpenMatch {
        rustStringToQString(match.scheme),
        archiveOpenMatchKindFromRust(match.kind),
    };
}

std::optional<KiriView::ArchiveOpenMatch> archiveMatchForQString(
    const QString &value, KiriView::RustArchiveOpenMatch (*rustFunction)(rust::Str))
{
    return archiveOpenMatchFromRust(
        rustResultForQString<KiriView::RustArchiveOpenMatch>(value, rustFunction));
}

QString schemeString(const std::optional<KiriView::ArchiveOpenMatch> &match)
{
    return match.has_value() ? match->scheme : QString();
}
}

namespace KiriView {
bool isSupportedArchiveRootScheme(const QString &scheme)
{
    return rustResultForQString<bool>(scheme, rustIsSupportedArchiveRootScheme);
}

ArchiveStorageBackend archiveStorageBackendForRootScheme(const QString &scheme)
{
    return archiveStorageBackendFromRust(rustResultForQString<RustArchiveStorageBackend>(
        scheme, rustArchiveStorageBackendForRootScheme));
}

bool archiveRootSchemeUsesKioFuse(const QString &scheme)
{
    return rustResultForQString<bool>(scheme, rustArchiveRootSchemeUsesKioFuse);
}

QStringList supportedComicBookArchiveExtensions()
{
    return rustStringsToQStringList(rustSupportedComicBookArchiveExtensions());
}

QStringList supportedComicBookArchiveMimeTypes()
{
    return rustStringsToQStringList(rustSupportedComicBookArchiveMimeTypes());
}

std::optional<ArchiveOpenMatch> comicBookArchiveMatchForFileName(const QString &fileName)
{
    return archiveMatchForQString(fileName, rustComicBookArchiveMatchForFileName);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForFileName(const QString &fileName)
{
    return archiveMatchForQString(fileName, rustDirectArchiveOpenMatchForFileName);
}

std::optional<ArchiveOpenMatch> comicBookArchiveMatchForMimeTypeName(const QString &mimeTypeName)
{
    return archiveMatchForQString(mimeTypeName, rustComicBookArchiveMatchForMimeTypeName);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForMimeTypeName(const QString &mimeTypeName)
{
    return archiveMatchForQString(mimeTypeName, rustDirectArchiveOpenMatchForMimeTypeName);
}

QString comicBookArchiveKioSchemeForFileName(const QString &fileName)
{
    return schemeString(comicBookArchiveMatchForFileName(fileName));
}

QString directArchiveOpenKioSchemeForFileName(const QString &fileName)
{
    return schemeString(directArchiveOpenMatchForFileName(fileName));
}

QString comicBookArchiveKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(comicBookArchiveMatchForMimeTypeName(mimeTypeName));
}

QString directArchiveOpenKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    return schemeString(directArchiveOpenMatchForMimeTypeName(mimeTypeName));
}

QString comicBookArchiveMarkerForRootScheme(const QString &scheme)
{
    return rustStringToQString(
        rustResultForQString<rust::String>(scheme, rustComicBookArchiveMarkerForRootScheme));
}

QStringList directArchiveOpenMarkersForRootScheme(const QString &scheme)
{
    return rustStringsToQStringList(rustResultForQString<rust::Vec<rust::String>>(
        scheme, rustDirectArchiveOpenMarkersForRootScheme));
}
}

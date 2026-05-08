// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivepath.h"

#include "imagelocation.h"
#include "kiriview/src/archivepath.cxx.h"

#include <QByteArray>
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

QString rustStringForQString(const QString &value, rust::String (*rustFunction)(rust::Str))
{
    const QByteArray bytes = value.toUtf8();
    return rustStringToQString(rustFunction(rustStringView(bytes)));
}
}

namespace KiriView {
QString normalizedArchiveRootPath(const QUrl &archiveRootUrl)
{
    return rustStringForQString(archiveRootUrl.path(), rustNormalizedArchiveRootPath);
}

QString normalizedArchiveEntryPath(const QString &entryPath)
{
    return rustStringForQString(entryPath, rustNormalizedArchiveEntryPath);
}

QString archiveRelativePathForUrl(const QUrl &archiveRootUrl, const QUrl &url)
{
    const QByteArray archiveRootScheme = archiveRootUrl.scheme().toUtf8();
    const QByteArray archiveRootPath = archiveRootUrl.path().toUtf8();
    const QByteArray urlScheme = url.scheme().toUtf8();
    const QByteArray urlPath = url.path().toUtf8();
    return rustStringToQString(rustArchiveRelativePathForUrl(archiveRootUrl.isEmpty(),
        rustStringView(archiveRootScheme), rustStringView(archiveRootPath), url.isEmpty(),
        rustStringView(urlScheme), rustStringView(urlPath)));
}

QString archiveEntryPathForUrl(const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    if (archiveDocument.isEmpty()) {
        return {};
    }

    return normalizedArchiveEntryPath(
        archiveRelativePathForUrl(archiveDocument.rootUrl(), imageUrl));
}

QUrl archiveEntryUrl(const ArchiveDocumentLocation &archiveDocument, const QString &entryPath)
{
    const QString cleanEntryPath = normalizedArchiveEntryPath(entryPath);
    if (archiveDocument.isEmpty() || cleanEntryPath.isEmpty()) {
        return {};
    }

    QUrl url = archiveDocument.rootUrl();
    url.setPath(normalizedArchiveRootPath(url) + cleanEntryPath);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}
}

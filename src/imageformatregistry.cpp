// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageformatregistry.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <Qt>

namespace {
QString extensionForFileName(const QString &name)
{
    const qsizetype dotIndex = name.lastIndexOf(QLatin1Char('.'));
    if (dotIndex <= 0 || dotIndex == name.size() - 1) {
        return {};
    }

    return name.mid(dotIndex + 1).toCaseFolded();
}

QString comicBookArchiveKioSchemeForExtension(const QString &extension)
{
    if (extension == QStringLiteral("cbz")) {
        return QStringLiteral("zip");
    }
    if (extension == QStringLiteral("cbt")) {
        return QStringLiteral("tar");
    }
    if (extension == QStringLiteral("cb7")) {
        return QStringLiteral("sevenz");
    }
    if (extension == QStringLiteral("cbr")) {
        return QStringLiteral("rar");
    }

    return {};
}

QString directArchiveOpenKioSchemeForExtension(const QString &extension)
{
    const QString comicBookScheme = comicBookArchiveKioSchemeForExtension(extension);
    if (!comicBookScheme.isEmpty()) {
        return comicBookScheme;
    }

    if (extension == QStringLiteral("zip")) {
        return QStringLiteral("zip");
    }
    if (extension == QStringLiteral("tar")) {
        return QStringLiteral("tar");
    }
    if (extension == QStringLiteral("7z")) {
        return QStringLiteral("sevenz");
    }
    if (extension == QStringLiteral("rar")) {
        return QStringLiteral("rar");
    }

    return {};
}

QString comicBookArchiveKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    if (mimeTypeName == QStringLiteral("application/vnd.comicbook+zip")) {
        return QStringLiteral("zip");
    }
    if (mimeTypeName == QStringLiteral("application/x-cbt")) {
        return QStringLiteral("tar");
    }
    if (mimeTypeName == QStringLiteral("application/x-cb7")) {
        return QStringLiteral("sevenz");
    }
    if (mimeTypeName == QStringLiteral("application/vnd.comicbook-rar")
        || mimeTypeName == QStringLiteral("application/x-cbr")) {
        return QStringLiteral("rar");
    }

    return {};
}
}

namespace KiriView {
QStringList supportedImageExtensions()
{
    return {
        QStringLiteral("avif"),
        QStringLiteral("avifs"),
        QStringLiteral("bmp"),
        QStringLiteral("gif"),
        QStringLiteral("heic"),
        QStringLiteral("heif"),
        QStringLiteral("hif"),
        QStringLiteral("jpeg"),
        QStringLiteral("jpg"),
        QStringLiteral("jp2"),
        QStringLiteral("jxl"),
        QStringLiteral("png"),
        QStringLiteral("svg"),
        QStringLiteral("webp"),
    };
}

QStringList supportedOpenExtensions()
{
    QStringList extensions = supportedImageExtensions();
    extensions.append(QStringLiteral("cbz"));
    extensions.append(QStringLiteral("cbt"));
    extensions.append(QStringLiteral("cb7"));
    extensions.append(QStringLiteral("cbr"));
    extensions.sort(Qt::CaseSensitive);
    return extensions;
}

bool isSupportedImageFileName(const QString &name)
{
    const QString extension = extensionForFileName(name);
    return !extension.isEmpty() && supportedImageExtensions().contains(extension);
}

bool isComicBookArchiveFileName(const QString &name)
{
    return !comicBookArchiveKioSchemeForExtension(extensionForFileName(name)).isEmpty();
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
        = comicBookArchiveKioSchemeForExtension(extensionForFileName(url.fileName()));
    if (!extensionScheme.isEmpty()) {
        return extensionScheme;
    }

    const QMimeType mimeType
        = QMimeDatabase().mimeTypeForFile(url.toLocalFile(), QMimeDatabase::MatchExtension);
    return comicBookArchiveKioSchemeForMimeTypeName(mimeType.name());
}

QString directArchiveOpenKioSchemeForMimeTypeName(const QString &mimeTypeName)
{
    const QString comicBookScheme = comicBookArchiveKioSchemeForMimeTypeName(mimeTypeName);
    if (!comicBookScheme.isEmpty()) {
        return comicBookScheme;
    }

    if (mimeTypeName == QStringLiteral("application/zip")) {
        return QStringLiteral("zip");
    }
    if (mimeTypeName == QStringLiteral("application/x-tar")) {
        return QStringLiteral("tar");
    }
    if (mimeTypeName == QStringLiteral("application/x-7z-compressed")) {
        return QStringLiteral("sevenz");
    }
    if (mimeTypeName == QStringLiteral("application/vnd.rar")
        || mimeTypeName == QStringLiteral("application/x-rar")
        || mimeTypeName == QStringLiteral("application/x-rar-compressed")) {
        return QStringLiteral("rar");
    }

    return {};
}

QString directArchiveOpenKioSchemeForUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return {};
    }

    const QString extensionScheme
        = directArchiveOpenKioSchemeForExtension(extensionForFileName(url.fileName()));
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

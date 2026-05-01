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
    return extensionForFileName(name) == QStringLiteral("cbz");
}

bool isComicBookArchiveUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return false;
    }

    if (isComicBookArchiveFileName(url.fileName())) {
        return true;
    }

    const QMimeType mimeType
        = QMimeDatabase().mimeTypeForFile(url.toLocalFile(), QMimeDatabase::MatchExtension);
    return mimeType.name() == QStringLiteral("application/vnd.comicbook+zip");
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

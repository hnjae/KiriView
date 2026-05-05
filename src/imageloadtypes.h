// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADTYPES_H
#define KIRIVIEW_IMAGELOADTYPES_H

#include "imagelocation.h"

#include <QUrl>
#include <QtGlobal>
#include <utility>

namespace KiriView {
struct ImageLoadRequest {
    ImageLocation source;
    ArchiveDocumentLocation displayedArchiveDocument;
    ContainerLocation containerNavigation;

    static ImageLoadRequest fromUrl(QUrl sourceUrl, QUrl containerNavigationUrl = QUrl())
    {
        return ImageLoadRequest { ImageLocation::fromUrl(std::move(sourceUrl)),
            ArchiveDocumentLocation::none(),
            ContainerLocation::fromUrl(std::move(containerNavigationUrl)) };
    }

    static ImageLoadRequest fromLocation(QUrl sourceUrl,
        ArchiveDocumentLocation displayedArchiveDocument, QUrl containerNavigationUrl = QUrl())
    {
        return ImageLoadRequest { ImageLocation::fromUrl(std::move(sourceUrl)),
            std::move(displayedArchiveDocument),
            ContainerLocation::fromUrl(std::move(containerNavigationUrl)) };
    }

    const QUrl &sourceUrl() const { return source.url(); }
    const ArchiveDocumentLocation &archiveDocument() const { return displayedArchiveDocument; }
    const QUrl &containerNavigationUrl() const { return containerNavigation.url(); }
    bool isEmpty() const { return source.isEmpty(); }
    bool isContainerNavigation() const { return !containerNavigation.isEmpty(); }
};

struct ImageLoadSession {
    quint64 id = 0;
    ImageLoadRequest request;
    DisplayedImageLocation location;
};

enum class ImageLoadError {
    Generic,
    EmptyArchive,
};
}

#endif

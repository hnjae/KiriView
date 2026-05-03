// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATEREPOSITORY_H
#define KIRIVIEW_IMAGECANDIDATEREPOSITORY_H

#include "imageiojobs.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <optional>

namespace KiriView {
enum class ImageCandidateContainerType {
    Directory,
    ArchiveDocument,
};

enum class ImageCandidateRepositoryError {
    Generic,
    EmptyContainer,
    InvalidComicBookArchive,
};

struct ImageCandidateListContext {
    QUrl currentUrl;
    QUrl listUrl;
    ArchiveDocumentLocation archiveDocument;
    ImageCandidateContainerType containerType = ImageCandidateContainerType::Directory;
};

struct ImageNavigationCandidateProvider {
    using ImageCandidateLoader
        = std::function<ImageIoJob(QObject *, QUrl, ImageCandidatesCallback, ErrorCallback)>;
    using ArchiveImageCandidateLoader = std::function<ImageIoJob(
        QObject *, ArchiveDocumentLocation, ImageCandidatesCallback, ErrorCallback)>;
    using ContainerCandidateLoader
        = std::function<ImageIoJob(QObject *, QUrl, ContainerCandidatesCallback, ErrorCallback)>;

    ImageCandidateLoader directoryImages;
    ContainerCandidateLoader directoryContainers;
    ArchiveImageCandidateLoader archiveImages;
};

using ContainerImageCallback = std::function<void(const QUrl &, const QUrl &)>;
using CandidateRepositoryErrorCallback
    = std::function<void(const QUrl &, ImageCandidateRepositoryError, const QString &)>;

ImageNavigationCandidateProvider defaultImageNavigationCandidateProvider();
std::optional<ImageCandidateListContext> imageCandidateListContextForDisplayedImage(
    const DisplayedImageLocation &location);

class ImageCandidateRepository
{
public:
    ImageCandidateRepository();
    explicit ImageCandidateRepository(ImageNavigationCandidateProvider provider);

    ImageIoJob loadImages(QObject *receiver, const ImageCandidateListContext &context,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadDirectoryImages(QObject *receiver, const QUrl &directoryUrl,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadArchiveImages(QObject *receiver, ArchiveDocumentLocation archiveDocument,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadContainers(QObject *receiver, const QUrl &directoryUrl,
        ContainerCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadFirstImageInContainer(QObject *receiver,
        const ContainerNavigationCandidate &container, ContainerImageCallback callback,
        CandidateRepositoryErrorCallback errorCallback) const;

private:
    ImageIoJob loadImagesInContainer(QObject *receiver,
        const ContainerNavigationCandidate &container, ImageCandidatesCallback callback,
        CandidateRepositoryErrorCallback errorCallback) const;

    ImageNavigationCandidateProvider m_provider;
};
}

#endif

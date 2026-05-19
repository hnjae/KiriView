// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATEREPOSITORY_H
#define KIRIVIEW_IMAGECANDIDATEREPOSITORY_H

#include "imagecandidatelistsource.h"
#include "imagecandidateprovider.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>

namespace KiriView {
enum class ImageCandidateRepositoryError {
    Generic,
    EmptyContainer,
    InvalidComicBookArchive,
};

using ContainerImageCallback = std::function<void(const QUrl &, const QUrl &)>;
using CandidateRepositoryErrorCallback
    = std::function<void(const QUrl &, ImageCandidateRepositoryError, const QString &)>;

class ImageCandidateRepository
{
public:
    explicit ImageCandidateRepository(ImageNavigationCandidateProvider provider);

    ImageIoJob loadImages(QObject *receiver, const ImageCandidateListSource &source,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadImages(QObject *receiver, const ImageCandidateListContext &context,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadDirectoryImages(QObject *receiver, const QUrl &directoryUrl,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadArchiveImages(QObject *receiver, ArchiveDocumentLocation archiveDocument,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadContainers(QObject *receiver, const QUrl &directoryUrl,
        ContainerCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob watchCandidateChanges(QObject *receiver, const ImageCandidateListSource &source,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob watchCandidateChanges(QObject *receiver, const ImageCandidateListContext &context,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob watchDirectoryImageChanges(QObject *receiver, const QUrl &directoryUrl,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
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

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

private:
    ImageNavigationCandidateProvider m_provider;
};
}

#endif

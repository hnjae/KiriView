// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEREPOSITORY_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEREPOSITORY_H

#include "imagedocumentpagecandidatelistsource.h"
#include "imagedocumentpagecandidateprovider.h"
#include "imagedocumentpagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>

namespace KiriView {
class ImageDocumentPageCandidateRepository
{
public:
    explicit ImageDocumentPageCandidateRepository(ImageDocumentPageCandidateProvider provider);

    ImageIoJob loadImages(QObject *receiver, const ImageDocumentPageCandidateListSource &source,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadImages(QObject *receiver, const ImageDocumentPageCandidateListContext &context,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadDirectoryImages(QObject *receiver, const QUrl &directoryUrl,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadOpenedCollectionCandidates(QObject *receiver,
        OpenedCollectionScopeLocation openedCollectionScope,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadContainers(QObject *receiver, const QUrl &directoryUrl,
        ContainerCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob watchCandidateChanges(QObject *receiver,
        const ImageDocumentPageCandidateListSource &source,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob watchCandidateChanges(QObject *receiver,
        const ImageDocumentPageCandidateListContext &context,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob watchDirectoryImageChanges(QObject *receiver, const QUrl &directoryUrl,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) const;

private:
    ImageDocumentPageCandidateProvider m_provider;
};
}

#endif

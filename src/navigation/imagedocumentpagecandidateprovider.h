// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEPROVIDER_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEPROVIDER_H

#include "async/directorylistingjob.h"
#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "async/imageworkerscheduler.h"
#include "imagedocumentpagecandidatecallbacks.h"
#include "location/imagelocation.h"

#include <QUrl>
#include <functional>

class QObject;

namespace KiriView {
struct ImageDocumentPageCandidateProvider {
    using ImageDocumentPageCandidateLoader = std::function<ImageIoJob(
        QObject *, QUrl, ImageDocumentPageCandidatesCallback, ErrorCallback)>;
    using OpenedCollectionCandidateLoader = std::function<ImageIoJob(QObject *,
        OpenedCollectionScopeLocation, ImageDocumentPageCandidatesCallback, ErrorCallback)>;
    using ContainerCandidateLoader
        = std::function<ImageIoJob(QObject *, QUrl, ContainerCandidatesCallback, ErrorCallback)>;
    using ImageDocumentPageCandidateChangeSubscriber = std::function<ImageIoJob(
        QObject *, QUrl, ImageDocumentPageCandidatesCallback, ErrorCallback)>;

    ImageDocumentPageCandidateLoader directoryImageDocumentPages;
    ContainerCandidateLoader directoryContainers;
    OpenedCollectionCandidateLoader openedCollectionCandidates;
    ImageDocumentPageCandidateChangeSubscriber directoryImageDocumentPageChanges;
};

ImageDocumentPageCandidateProvider defaultImageDocumentPageCandidateProvider(
    ImageWorkerScheduler workerScheduler = {},
    DirectoryItemListProvider directoryItemListProvider = {});
ImageDocumentPageCandidateProvider imageDocumentPageNavigationCandidateProviderWithDefaults(
    ImageDocumentPageCandidateProvider provider, ImageWorkerScheduler workerScheduler = {},
    DirectoryItemListProvider directoryItemListProvider = {});
}

#endif

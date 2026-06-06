// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEIOJOBS_H
#define KIRIVIEW_IMAGEIOJOBS_H

#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "async/imageworkerscheduler.h"
#include "decoding/imagedecoderequest.h"
#include "location/imagelocation.h"
#include "navigation/imagedocumentpagecandidatecallbacks.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QUrl>

class QObject;

namespace KiriView {
ImageIoJob startDirectoryImageDocumentPageCandidateList(QObject *receiver, QUrl directoryUrl,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback);
ImageIoJob startDirectoryContainerCandidateList(QObject *receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback);
ImageIoJob startOpenedCollectionCandidateList(QObject *receiver,
    OpenedCollectionScopeLocation openedCollectionScope,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback);
ImageIoJob startOpenedCollectionCandidateList(QObject *receiver,
    OpenedCollectionScopeLocation openedCollectionScope,
    const ImageWorkerScheduler &workerScheduler, ImageDocumentPageCandidatesCallback callback,
    ErrorCallback errorCallback);
ImageIoJob startStoredImageDataLoad(QObject *receiver, ImageDecodeRequest request,
    ImageDataCallback callback, ErrorCallback errorCallback);
ImageIoJob startStoredImageDataLoad(QObject *receiver, ImageDecodeRequest request,
    const ImageWorkerScheduler &workerScheduler, ImageDataCallback callback,
    ErrorCallback errorCallback);
}

#endif

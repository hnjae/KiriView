// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEIOJOBS_H
#define KIRIVIEW_IMAGEIOJOBS_H

#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "decoding/imagedecoderequest.h"
#include "location/imagelocation.h"
#include "navigation/imagecandidatecallbacks.h"
#include "navigation/imagenavigationtypes.h"

#include <QUrl>

class QObject;

namespace KiriView {
ImageIoJob startDirectoryImageCandidateList(QObject *receiver, QUrl directoryUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback);
ImageIoJob startDirectoryContainerCandidateList(QObject *receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback);
ImageIoJob startArchiveImageCandidateList(QObject *receiver, ImagePageScopeLocation archiveDocument,
    ImageCandidatesCallback callback, ErrorCallback errorCallback);
ImageIoJob startStoredImageDataLoad(QObject *receiver, ImageDecodeRequest request,
    ImageDataCallback callback, ErrorCallback errorCallback);
}

#endif

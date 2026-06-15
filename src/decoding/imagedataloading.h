// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDATALOADING_H
#define KIRIVIEW_IMAGEDATALOADING_H

#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "async/imageworkerscheduler.h"
#include "decoding/imagedecoderequest.h"

class QObject;

namespace kiriview {
ImageIoJob startStoredImageDataLoad(QObject *receiver, ImageDecodeRequest request,
    ImageDataCallback callback, ErrorCallback errorCallback);
ImageIoJob startStoredImageDataLoad(QObject *receiver, ImageDecodeRequest request,
    const ImageWorkerScheduler &workerScheduler, ImageDataCallback callback,
    ErrorCallback errorCallback);
}

#endif

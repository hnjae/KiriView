// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DIRECTORYLISTINGJOB_H
#define KIRIVIEW_DIRECTORYLISTINGJOB_H

#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"

#include <KFileItem>
#include <QUrl>
#include <functional>

class QObject;

namespace kiriview {
using DirectoryItemListCallback = std::function<void(KFileItemList)>;
using DirectoryItemListProvider
    = std::function<ImageIoJob(QObject *, QUrl, DirectoryItemListCallback, ErrorCallback)>;

ImageIoJob startDirectoryItemList(QObject *receiver, QUrl directoryUrl,
    DirectoryItemListCallback callback, ErrorCallback errorCallback);
ImageIoJob startDirectoryItemList(QObject *receiver, QUrl directoryUrl,
    DirectoryItemListCallback callback, ErrorCallback errorCallback,
    DirectoryItemListProvider provider);
DirectoryItemListProvider defaultDirectoryItemListProvider();
}

#endif

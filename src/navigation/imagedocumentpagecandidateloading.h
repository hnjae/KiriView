// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATELOADING_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATELOADING_H

#include "async/directorylistingjob.h"
#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "navigation/imagedocumentpagecandidatecallbacks.h"

#include <QUrl>

class QObject;

namespace kiriview {
ImageIoJob startDirectoryImageDocumentPageCandidateList(QObject* receiver, QUrl directoryUrl,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback);
ImageIoJob startDirectoryImageDocumentPageCandidateList(QObject* receiver, QUrl directoryUrl,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback,
    DirectoryItemListProvider directoryItemListProvider);
ImageIoJob startDirectoryContainerCandidateList(QObject* receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback);
ImageIoJob startDirectoryContainerCandidateList(QObject* receiver, QUrl directoryUrl,
    ContainerCandidatesCallback callback, ErrorCallback errorCallback,
    DirectoryItemListProvider directoryItemListProvider);
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEIOJOBS_H
#define KIRIVIEW_IMAGEIOJOBS_H

#include "kiriimagenavigation.h"

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <functional>
#include <vector>

class QObject;

namespace KiriView {
class AsyncObjectSlot;

using ImageCandidatesCallback = std::function<void(std::vector<ImageNavigationCandidate>)>;
using ContainerCandidatesCallback = std::function<void(std::vector<ContainerNavigationCandidate>)>;
using ImageDataCallback = std::function<void(QByteArray)>;
using ErrorCallback = std::function<void(const QString &)>;

void startDirectoryImageCandidateList(QObject *receiver, AsyncObjectSlot *slot, QUrl directoryUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback);
void startDirectoryContainerCandidateList(QObject *receiver, AsyncObjectSlot *slot,
    QUrl directoryUrl, ContainerCandidatesCallback callback, ErrorCallback errorCallback);
void startArchiveImageCandidateList(QObject *receiver, AsyncObjectSlot *slot, QUrl archiveRootUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback);
void startStoredImageDataLoad(QObject *receiver, AsyncObjectSlot *slot, QUrl imageUrl,
    ImageDataCallback callback, ErrorCallback errorCallback);
}

#endif

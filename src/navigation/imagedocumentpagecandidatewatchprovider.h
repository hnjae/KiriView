// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEWATCHPROVIDER_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEWATCHPROVIDER_H

#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "imagedocumentpagecandidatecallbacks.h"
#include "imagedocumentpagenavigationtypes.h"

#include <QList>
#include <QUrl>
#include <functional>
#include <vector>

class QObject;

namespace kiriview {
using ImageDocumentPageCandidateWatchSnapshotCallback
    = std::function<void(std::vector<ImageDocumentPageCandidate>)>;
using ImageDocumentPageCandidateWatchDeletedCallback = std::function<void(QList<QUrl>)>;
using ImageDocumentPageCandidateWatchProvider
    = std::function<ImageIoJob(QObject *, QUrl, ImageDocumentPageCandidateWatchSnapshotCallback,
        ImageDocumentPageCandidateWatchSnapshotCallback,
        ImageDocumentPageCandidateWatchDeletedCallback, ErrorCallback)>;

ImageDocumentPageCandidateWatchProvider defaultImageDocumentPageCandidateWatchProvider();
}

#endif

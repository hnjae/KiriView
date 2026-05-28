// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediacandidateprovider.h"

#include "async/directorylistingjob.h"
#include "async/imagecallback.h"
#include "imagecandidateitems.h"
#include "navigationlogging.h"

#include <QDebug>
#include <QObject>
#include <utility>
#include <vector>

namespace {
KiriView::ImageIoJob startDirectoryMediaCandidateList(QObject *receiver, const QUrl &directoryUrl,
    KiriView::MediaCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    qCDebug(kiriviewNavigationLog) << "media candidate provider listing directory"
                                   << "directoryUrl" << directoryUrl;
    return KiriView::startDirectoryItemList(
        receiver, directoryUrl,
        [callback = std::move(callback), directoryUrl](KFileItemList items) mutable {
            std::vector<KiriView::MediaNavigationCandidate> candidates
                = KiriView::mediaNavigationCandidates(items);
            qCDebug(kiriviewNavigationLog) << "media candidate provider listed directory"
                                           << "directoryUrl" << directoryUrl << "items"
                                           << items.size() << "candidates" << candidates.size();
            KiriView::invokeIfSet(callback, std::move(candidates));
        },
        [errorCallback = std::move(errorCallback), directoryUrl](const QString &errorString) {
            qCDebug(kiriviewNavigationLog)
                << "media candidate provider listing failed"
                << "directoryUrl" << directoryUrl << "error" << errorString;
            KiriView::invokeIfSet(errorCallback, errorString);
        });
}
}

namespace KiriView {
MediaNavigationCandidateProvider defaultMediaNavigationCandidateProvider()
{
    return MediaNavigationCandidateProvider {
        [](QObject *receiver, QUrl directoryUrl, MediaCandidatesCallback callback,
            ErrorCallback errorCallback) {
            return startDirectoryMediaCandidateList(
                receiver, directoryUrl, std::move(callback), std::move(errorCallback));
        },
    };
}

MediaNavigationCandidateProvider mediaNavigationCandidateProviderWithDefault(
    MediaNavigationCandidateProvider provider)
{
    if (!provider.directoryMedia) {
        provider = defaultMediaNavigationCandidateProvider();
    }
    return provider;
}
}

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "directmedianavigationcandidateprovider.h"

#include "async/directorylistingjob.h"
#include "async/imagecallback.h"
#include "imagedocumentpagecandidateitems.h"
#include "navigationlogging.h"

#include <QDebug>
#include <QObject>
#include <utility>
#include <vector>

namespace {
KiriView::ImageIoJob startDirectoryDirectMediaNavigationCandidateList(QObject *receiver,
    const QUrl &directoryUrl, KiriView::DirectMediaNavigationCandidatesCallback callback,
    KiriView::ErrorCallback errorCallback)
{
    qCDebug(kiriviewNavigationLog) << "direct media navigation candidate provider listing directory"
                                   << "directoryUrl" << directoryUrl;
    return KiriView::startDirectoryItemList(
        receiver, directoryUrl,
        [callback = std::move(callback), directoryUrl](KFileItemList items) mutable {
            std::vector<KiriView::DirectMediaNavigationCandidate> candidates
                = KiriView::directMediaNavigationCandidates(items);
            qCDebug(kiriviewNavigationLog)
                << "direct media navigation candidate provider listed directory"
                << "directoryUrl" << directoryUrl << "items" << items.size() << "candidates"
                << candidates.size();
            KiriView::invokeIfSet(callback, std::move(candidates));
        },
        [errorCallback = std::move(errorCallback), directoryUrl](const QString &errorString) {
            qCDebug(kiriviewNavigationLog)
                << "direct media navigation candidate provider listing failed"
                << "directoryUrl" << directoryUrl << "error" << errorString;
            KiriView::invokeIfSet(errorCallback, errorString);
        });
}
}

namespace KiriView {
DirectMediaNavigationCandidateProvider defaultDirectMediaNavigationCandidateProvider()
{
    return DirectMediaNavigationCandidateProvider {
        [](QObject *receiver, QUrl directoryUrl, DirectMediaNavigationCandidatesCallback callback,
            ErrorCallback errorCallback) {
            return startDirectoryDirectMediaNavigationCandidateList(
                receiver, directoryUrl, std::move(callback), std::move(errorCallback));
        },
    };
}

DirectMediaNavigationCandidateProvider directMediaNavigationCandidateProviderWithDefault(
    DirectMediaNavigationCandidateProvider provider)
{
    if (!provider.directoryCandidateLoader) {
        provider = defaultDirectMediaNavigationCandidateProvider();
    }
    return provider;
}
}

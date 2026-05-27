// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediacandidateprovider.h"

#include "async/directorylistingjob.h"
#include "async/imagecallback.h"
#include "imagecandidateitems.h"

#include <QObject>
#include <utility>

namespace {
KiriView::ImageIoJob startDirectoryMediaCandidateList(QObject *receiver, const QUrl &directoryUrl,
    KiriView::MediaCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return KiriView::startDirectoryItemList(
        receiver, directoryUrl,
        [callback = std::move(callback)](KFileItemList items) mutable {
            KiriView::invokeIfSet(callback, KiriView::mediaNavigationCandidates(items));
        },
        std::move(errorCallback));
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

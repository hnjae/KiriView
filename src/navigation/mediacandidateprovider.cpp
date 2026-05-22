// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediacandidateprovider.h"

#include "async/directorylistingjob.h"
#include "async/imagecallback.h"
#include "mediaformatregistry.h"

#include <KFileItem>
#include <QObject>
#include <cstddef>
#include <utility>

namespace {
std::vector<KiriView::MediaNavigationCandidate> mediaCandidatesFromItems(const KFileItemList &items)
{
    std::vector<KiriView::MediaNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (!item.isFile() || !KiriView::isSupportedOrdinaryMediaFileName(name)) {
            continue;
        }

        candidates.push_back(KiriView::MediaNavigationCandidate { item.url(), name });
    }

    KiriView::sortMediaNavigationCandidates(&candidates);
    return candidates;
}

KiriView::ImageIoJob startDirectoryMediaCandidateList(QObject *receiver, const QUrl &directoryUrl,
    KiriView::MediaCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
{
    return KiriView::startDirectoryItemList(
        receiver, directoryUrl,
        [callback = std::move(callback)](KFileItemList items) mutable {
            KiriView::invokeIfSet(callback, mediaCandidatesFromItems(items));
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

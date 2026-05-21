// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPAGENAVIGATIONREFRESHSTATE_H
#define KIRIVIEW_IMAGEPAGENAVIGATIONREFRESHSTATE_H

#include "async/imageasyncoperationstate.h"
#include "imagecandidatelistsource.h"

#include <QtGlobal>
#include <optional>

namespace KiriView {
struct ImagePageNavigationRefreshPreview {
    bool keepKnownUrls = false;
    quint64 refreshId = 0;
};

class ImagePageNavigationRefreshState final
{
public:
    ImagePageNavigationRefreshPreview beginRefresh(
        const ImageCandidateListContext &context, bool knownUrlsAvailable);
    bool shouldKeepExistingWatcherFor(const ImageCandidateListContext &context) const;
    std::optional<ImageCandidateListContext> acceptedPendingRefreshContext(
        quint64 refreshId, ImageCandidateListSource source) const;
    std::optional<ImageCandidateListContext> acceptedWatchedRefreshContext(
        ImageCandidateListSource source) const;
    bool finishPendingRefresh(quint64 refreshId);
    void finishRefresh(ImageCandidateListContext context);
    void clear();

private:
    std::optional<ImageCandidateListContext> m_knownContext;
    std::optional<ImageCandidateListContext> m_refreshContext;
    ImageAsyncOperationState m_pendingRefresh;
};
}

#endif

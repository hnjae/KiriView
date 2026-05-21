// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPAGENAVIGATIONMODEL_H
#define KIRIVIEW_IMAGEPAGENAVIGATIONMODEL_H

#include "imagecandidatelistsource.h"
#include "imagenavigationtypes.h"
#include "imagepagenavigationrefreshstate.h"

#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
struct ImagePageNavigationRefreshResult {
    bool accepted = false;
    bool changed = false;
    bool currentImageRemoved = false;
    std::optional<ImageCandidateListContext> context;
};

struct ImagePageNavigationRefreshPlan {
    bool changed = false;
    quint64 refreshId = 0;
};

class ImagePageNavigationModel
{
public:
    int currentPageNumber() const;
    int imageCount() const;
    ImagePageNavigationSnapshot snapshot() const;
    bool hasKnownSelection() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;
    std::optional<QUrl> selectPage(int pageNumber);
    std::optional<QUrl> selectAdjacentPage(NavigationDirection direction);

    bool shouldKeepExistingWatcherFor(const ImageCandidateListContext &context) const;
    ImagePageNavigationRefreshPlan beginRefresh(const ImageCandidateListContext &context);
    bool completeRefresh(const std::vector<ImageNavigationCandidate> &candidates,
        const QUrl &currentUrl, ImageCandidateListSource source);
    ImagePageNavigationRefreshResult completePendingRefresh(
        const std::vector<ImageNavigationCandidate> &candidates, quint64 refreshId,
        ImageCandidateListSource source);
    ImagePageNavigationRefreshResult completeWatchedRefreshFromCurrentContext(
        const std::vector<ImageNavigationCandidate> &candidates, ImageCandidateListSource source);
    bool clear();

private:
    ImagePageNavigationRefreshResult completeRefreshFromCurrentContext(
        const std::vector<ImageNavigationCandidate> &candidates, ImageCandidateListContext context);
    bool completeRefresh(std::vector<QUrl> urls, ImageCandidateListContext context);
    bool replaceState(PageNavigationState state, bool forceChanged = false);

    PageNavigationState m_state;
    ImagePageNavigationRefreshState m_refreshState;
};
}

#endif

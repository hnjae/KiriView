// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPAGENAVIGATIONMODEL_H
#define KIRIVIEW_IMAGEPAGENAVIGATIONMODEL_H

#include "imagecandidaterepository.h"
#include "imagenavigationtypes.h"

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
    bool previewRefresh(const ImageCandidateListContext &context);
    bool completeRefresh(const std::vector<ImageNavigationCandidate> &candidates,
        const QUrl &currentUrl, ImageCandidateListSource source);
    ImagePageNavigationRefreshResult completeRefreshFromCurrentContext(
        const std::vector<ImageNavigationCandidate> &candidates, ImageCandidateListSource source);
    bool clear();

private:
    bool completeRefresh(
        std::vector<QUrl> urls, const QUrl &currentUrl, ImageCandidateListSource source);
    bool replaceState(PageNavigationState state, bool forceChanged = false);

    PageNavigationState m_state;
    std::optional<ImageCandidateListContext> m_knownContext;
    std::optional<ImageCandidateListContext> m_refreshContext;
};
}

#endif

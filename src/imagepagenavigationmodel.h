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
class ImagePageNavigationModel
{
public:
    int currentPageNumber() const;
    int imageCount() const;
    bool hasKnownSelection() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;
    std::optional<QUrl> selectPage(int pageNumber);
    std::optional<QUrl> selectAdjacentPage(NavigationDirection direction);

    bool shouldKeepExistingWatcherFor(const ImageCandidateListContext &context) const;
    bool isCurrentRefreshSource(const ImageCandidateListSource &source) const;
    std::optional<ImageCandidateListContext> refreshContext() const;
    bool previewRefresh(const ImageCandidateListContext &context);
    bool completeRefresh(
        std::vector<QUrl> urls, const QUrl &currentUrl, ImageCandidateListSource source);
    bool clear();

private:
    bool replaceState(PageNavigationState state, bool forceChanged = false);

    PageNavigationState m_state;
    std::optional<ImageCandidateListSource> m_source;
    std::optional<ImageCandidateListContext> m_refreshContext;
};
}

#endif

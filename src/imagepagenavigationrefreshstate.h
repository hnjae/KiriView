// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPAGENAVIGATIONREFRESHSTATE_H
#define KIRIVIEW_IMAGEPAGENAVIGATIONREFRESHSTATE_H

#include "imagecandidaterepository.h"

#include <optional>

namespace KiriView {
struct ImagePageNavigationRefreshPreview {
    bool keepKnownUrls = false;
};

class ImagePageNavigationRefreshState final
{
public:
    ImagePageNavigationRefreshPreview beginRefresh(
        const ImageCandidateListContext &context, bool knownUrlsAvailable);
    bool shouldKeepExistingWatcherFor(const ImageCandidateListContext &context) const;
    std::optional<ImageCandidateListContext> acceptedPendingRefreshContext(
        ImageCandidateListSource source) const;
    void finishRefresh(ImageCandidateListContext context);
    void clear();

private:
    std::optional<ImageCandidateListContext> m_knownContext;
    std::optional<ImageCandidateListContext> m_refreshContext;
};
}

#endif

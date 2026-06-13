// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESECONDARYPAGESTATE_H
#define KIRIVIEW_IMAGESECONDARYPAGESTATE_H

#include "location/imagelocation.h"

#include <QSize>
#include <optional>

namespace kiriview {
enum class ImageSecondaryPageLoadResult {
    PrimaryOnly,
    Visible,
    Failed,
};

struct ImageSecondaryPageDisplayState {
    DisplayedImageLocation location;
    QSize imageSize;
};

struct ImageSecondaryPageLoadCompletion {
    ImageSecondaryPageLoadResult result = ImageSecondaryPageLoadResult::Failed;
    DisplayedImageLocation location;
    QSize imageSize;
    bool clearPresentation = false;
};

class ImageSecondaryPageState final
{
public:
    bool visible() const;
    DisplayedImageLocation displayedImageLocation() const;
    QSize imageSize() const;

    void clear();
    ImageSecondaryPageLoadCompletion finishPresentedLoad(
        const DisplayedImageLocation &location, const QSize &imageSize, bool primaryOnly);
    ImageSecondaryPageLoadCompletion finishFailedLoad(const DisplayedImageLocation &location) const;

private:
    std::optional<ImageSecondaryPageDisplayState> m_displayedPage;
};
}

#endif

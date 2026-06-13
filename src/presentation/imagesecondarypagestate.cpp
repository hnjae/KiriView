// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagesecondarypagestate.h"

namespace kiriview {
bool ImageSecondaryPageState::visible() const { return m_displayedPage.has_value(); }

DisplayedImageLocation ImageSecondaryPageState::displayedImageLocation() const
{
    return m_displayedPage.has_value() ? m_displayedPage->location : DisplayedImageLocation {};
}

QSize ImageSecondaryPageState::imageSize() const
{
    return m_displayedPage.has_value() ? m_displayedPage->imageSize : QSize();
}

void ImageSecondaryPageState::clear() { m_displayedPage.reset(); }

ImageSecondaryPageLoadCompletion ImageSecondaryPageState::finishPresentedLoad(
    const DisplayedImageLocation &location, const QSize &imageSize, bool primaryOnly)
{
    if (primaryOnly) {
        clear();
        return ImageSecondaryPageLoadCompletion {
            ImageSecondaryPageLoadResult::PrimaryOnly,
            location,
            imageSize,
            true,
        };
    }

    m_displayedPage = ImageSecondaryPageDisplayState { location, imageSize };
    return ImageSecondaryPageLoadCompletion {
        ImageSecondaryPageLoadResult::Visible,
        location,
        imageSize,
        false,
    };
}

ImageSecondaryPageLoadCompletion ImageSecondaryPageState::finishFailedLoad(
    const DisplayedImageLocation &location) const
{
    return ImageSecondaryPageLoadCompletion {
        ImageSecondaryPageLoadResult::Failed,
        location,
        QSize(),
        false,
    };
}
}

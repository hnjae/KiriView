// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADSESSIONTRACKER_H
#define KIRIVIEW_IMAGELOADSESSIONTRACKER_H

#include "imagedecoderequest.h"
#include "imageloadplan.h"
#include "staticimage.h"

#include <QUrl>
#include <QtGlobal>
#include <optional>

namespace KiriView {
class ImageLoadSessionTracker final
{
public:
    ImageLoadPlan start(
        ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext = {});
    void cancel();

    const ImageFirstDisplayDecodeContext &firstDisplayContext() const;
    bool isCurrent(const ImageLoadSession &session) const;
    std::optional<ImageLoadSession> currentForDecodeRequest(
        const ImageDecodeRequest &request) const;
    std::optional<ImageLoadSession> resolveCurrentArchiveImage(
        const ImageLoadSession &session, QUrl imageUrl);
    std::optional<ImageLoadSession> replaceCurrentLocation(
        const ImageLoadSession &session, DisplayedImageLocation location);
    std::optional<ImageLoadSession> takeCurrent(const ImageLoadSession &session);

private:
    quint64 nextSessionId();

    std::optional<ImageLoadSession> m_session;
    ImageFirstDisplayDecodeContext m_firstDisplayContext;
    quint64 m_nextSessionId = 0;
};
}

#endif

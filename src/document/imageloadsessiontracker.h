// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADSESSIONTRACKER_H
#define KIRIVIEW_IMAGELOADSESSIONTRACKER_H

#include "async/imageasyncticket.h"
#include "decoding/imagedecoderequest.h"
#include "imageloadplan.h"
#include "navigation/imagenavigationtypes.h"
#include "rendering/staticimage.h"

#include <QUrl>
#include <QtGlobal>
#include <optional>
#include <vector>

namespace KiriView {
enum class OpenedCollectionCandidateCompletionAction {
    Ignored,
    ReportEmptyArchive,
    StartImageDecode,
    ReportUnsupportedOpenedCollectionVideo,
};

struct OpenedCollectionCandidateCompletion {
    OpenedCollectionCandidateCompletionAction action
        = OpenedCollectionCandidateCompletionAction::Ignored;
    ImageLoadSession session;
};

class ImageLoadSessionTracker final
{
public:
    explicit ImageLoadSessionTracker(quint64 nextSessionId = 0);

    ImageLoadPlan start(
        ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext = {});
    void cancel();

    bool isCurrent(const ImageLoadSession &session) const;
    std::optional<ImageLoadSession> claimCurrentForDecodeRequest(const ImageDecodeRequest &request);
    OpenedCollectionCandidateCompletion completeOpenedCollectionCandidates(
        const ImageLoadSession &session, const std::vector<ImageNavigationCandidate> &candidates);
    std::optional<ImageLoadSession> claimPredecodedImage(
        const ImageLoadSession &session, DisplayedImageLocation location);
    std::optional<ImageLoadSession> claimCurrent(const ImageLoadSession &session);

private:
    quint64 nextSessionId();

    std::optional<ImageLoadSession> m_session;
    ImageAsyncTicket m_sessionIds;
};
}

#endif

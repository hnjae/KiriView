// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADSESSIONTRACKER_H
#define KIRIVIEW_IMAGELOADSESSIONTRACKER_H

#include "imagedecoderequest.h"
#include "imageloadplan.h"
#include "imagenavigationtypes.h"
#include "staticimage.h"

#include <QUrl>
#include <QtGlobal>
#include <optional>
#include <vector>

namespace KiriView {
enum class ImageArchiveCandidateCompletionAction {
    Ignored,
    ReportEmptyArchive,
    StartImageDecode,
};

struct ImageArchiveCandidateCompletion {
    ImageArchiveCandidateCompletionAction action = ImageArchiveCandidateCompletionAction::Ignored;
    ImageLoadSession session;
    QUrl resolvedUrl;
};

class ImageLoadSessionTracker final
{
public:
    ImageLoadPlan start(
        ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext = {});
    void cancel();

    bool isCurrent(const ImageLoadSession &session) const;
    std::optional<ImageLoadSession> claimCurrentForDecodeRequest(const ImageDecodeRequest &request);
    ImageArchiveCandidateCompletion completeArchiveCandidates(
        const ImageLoadSession &session, const std::vector<ImageNavigationCandidate> &candidates);
    std::optional<ImageLoadSession> claimPredecodedImage(
        const ImageLoadSession &session, DisplayedImageLocation location);
    std::optional<ImageLoadSession> claimCurrent(const ImageLoadSession &session);

private:
    quint64 nextSessionId();

    std::optional<ImageLoadSession> m_session;
    quint64 m_nextSessionId = 0;
};
}

#endif

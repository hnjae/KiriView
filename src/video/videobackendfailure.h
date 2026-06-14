// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOBACKENDFAILURE_H
#define KIRIVIEW_VIDEOBACKENDFAILURE_H

#include <QString>
#include <QUrl>

namespace kiriview {
enum class VideoBackendFailureKind {
    Playback,
};

enum class VideoBackendFailureSeverity {
    Error,
};

struct VideoBackendFailure {
    QUrl sourceUrl;
    VideoBackendFailureKind kind = VideoBackendFailureKind::Playback;
    QString userMessage;
    QString diagnosticDetail;
    VideoBackendFailureSeverity severity = VideoBackendFailureSeverity::Error;
    bool retryable = false;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APNGDECODER_H
#define KIRIVIEW_APNGDECODER_H

#include "animationframe.h"

#include <QByteArray>
#include <QString>
#include <vector>

namespace KiriView {
struct ApngAnimation {
    std::vector<AnimationFrame> frames;
    int loopCount = 0;
};

enum class ApngDecodeStatus {
    NotApng,
    Success,
    Error,
};

struct ApngDecodeResult {
    ApngDecodeStatus status = ApngDecodeStatus::NotApng;
    ApngAnimation animation;
    QString errorString;
};

ApngDecodeResult decodeApngAnimation(const QByteArray &data);
}

#endif

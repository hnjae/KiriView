// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAPREDECODEDEPENDENCIES_H
#define KIRIVIEW_MEDIAPREDECODEDEPENDENCIES_H

#include "async/timerscheduler.h"
#include "cache/imagecachepolicy.h"
#include "decoding/imagedecodedependencies.h"
#include "system/powersaverprovider.h"

#include <QtGlobal>

namespace kiriview {
struct MediaPredecodeDependencyOverrides {
    ImageDecodeDependencies imageDecode;
    PowerSaverProvider powerSaver;
    TimerScheduler timerScheduler;
    ImageCacheBudgetRequest cacheBudgetRequest;
};

struct MediaPredecodeDependencies {
    ImageDecodeDependencies imageDecode;
    PowerSaverProvider powerSaver;
    qsizetype cacheByteBudget = 0;
    TimerScheduler timerScheduler;
};

MediaPredecodeDependencies resolveMediaPredecodeDependencies(
    MediaPredecodeDependencyOverrides overrides);
}

#endif

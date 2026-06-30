// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOPLAYBACKSOURCE_H
#define KIRIVIEW_VIDEOPLAYBACKSOURCE_H

#include <QIODevice>
#include <memory>

namespace kiriview {
struct VideoPlaybackSourceDevice
{
    std::shared_ptr<void> owner;
    std::unique_ptr<QIODevice> device;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ANIMATIONFRAME_H
#define KIRIVIEW_ANIMATIONFRAME_H

#include <QImage>

namespace kiriview {
struct AnimationFrame
{
    QImage image;
    int delay = 0;
};
}

#endif

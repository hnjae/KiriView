// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECALLBACK_H
#define KIRIVIEW_IMAGECALLBACK_H

#include <utility>

namespace KiriView {
template <typename Callback, typename... Args> void invokeIfSet(Callback &callback, Args &&...args)
{
    if (callback) {
        callback(std::forward<Args>(args)...);
    }
}
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APNGFRAMECOMPOSER_H
#define KIRIVIEW_APNGFRAMECOMPOSER_H

#include "apngrgbabuffer.h"

#include <QImage>
#include <QSize>
#include <QtGlobal>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
enum class ApngFrameDisposeOp {
    None,
    Background,
    Previous,
};

enum class ApngFrameBlendOp {
    Source,
    Over,
};

struct ApngFrameControl {
    quint32 width = 0;
    quint32 height = 0;
    quint32 xOffset = 0;
    quint32 yOffset = 0;
    ApngFrameDisposeOp disposeOp = ApngFrameDisposeOp::None;
    ApngFrameBlendOp blendOp = ApngFrameBlendOp::Source;
};

class ApngFrameComposer final
{
public:
    bool initialize(QSize canvasSize, std::size_t rowBytes);
    void clear();

    bool canComposeFrame(const ApngFrameControl &control) const;
    unsigned char **frameRows();
    std::optional<QImage> composeFrame(ApngFrameControl control);

private:
    ApngRgbaRegion region(const ApngFrameControl &control) const;
    void premultiplyFrame(const ApngFrameControl &control);
    bool blendFrame(const ApngFrameControl &control);
    bool applyDispose(
        const ApngFrameControl &control, const std::optional<std::vector<unsigned char>> &previous);

    bool m_hasDisplayedFrame = false;
    ApngRgbaBuffer m_canvas;
    ApngRgbaBuffer m_frame;
};
}

#endif

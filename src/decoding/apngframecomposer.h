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

enum class ApngFrameDisposeAction {
    None,
    ClearFrameRegion,
    RestorePreviousRegion,
};

struct ApngFrameControl {
    quint32 width = 0;
    quint32 height = 0;
    quint32 xOffset = 0;
    quint32 yOffset = 0;
    ApngFrameDisposeOp disposeOp = ApngFrameDisposeOp::None;
    ApngFrameBlendOp blendOp = ApngFrameBlendOp::Source;
};

struct ApngFrameCompositionPlan {
    ApngFrameControl displayControl;
    bool capturePreviousRegion = false;
    ApngFrameDisposeAction disposeAction = ApngFrameDisposeAction::None;
};

ApngFrameCompositionPlan apngFrameCompositionPlan(bool hasDisplayedFrame, ApngFrameControl control);

class ApngFrameComposer final
{
public:
    bool initialize(QSize canvasSize, std::size_t rowBytes);
    void clear();

    bool canComposeFrame(const ApngFrameControl &control) const;
    unsigned char **frameRows();
    bool setFrameBytes(const ApngFrameControl &control, const unsigned char *bytes,
        std::size_t byteCount, std::size_t rowBytes);
    std::optional<QImage> composeFrame(ApngFrameControl control);

private:
    ApngRgbaRegion region(const ApngFrameControl &control) const;
    void premultiplyFrame(const ApngFrameControl &control);
    bool blendFrame(const ApngFrameControl &control);
    bool applyDispose(const ApngFrameCompositionPlan &plan,
        const std::optional<std::vector<unsigned char>> &previous);

    bool m_hasDisplayedFrame = false;
    ApngRgbaBuffer m_canvas;
    ApngRgbaBuffer m_frame;
};
}

#endif

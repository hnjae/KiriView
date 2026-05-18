// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APNGFRAMECOMPOSER_H
#define KIRIVIEW_APNGFRAMECOMPOSER_H

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
    std::optional<std::size_t> rowOffset(quint32 x, quint32 y) const;
    void premultiplyFrame(const ApngFrameControl &control);
    std::optional<std::vector<unsigned char>> copyRegion(const ApngFrameControl &control) const;
    bool blendFrame(const ApngFrameControl &control);
    std::optional<QImage> canvasImage() const;
    bool applyDispose(
        const ApngFrameControl &control, const std::optional<std::vector<unsigned char>> &previous);

    QSize m_canvasSize;
    std::size_t m_rowBytes = 0;
    bool m_hasDisplayedFrame = false;
    std::vector<unsigned char> m_canvas;
    std::vector<unsigned char> m_frame;
    std::vector<unsigned char *> m_frameRows;
};
}

#endif

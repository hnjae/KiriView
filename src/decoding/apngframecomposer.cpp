// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "apngframecomposer.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace {
std::optional<std::size_t> frameRowBytes(quint32 width)
{
    const auto pixelCount = static_cast<std::size_t>(width);
    if (pixelCount != 0
        && pixelCount
            > std::numeric_limits<std::size_t>::max() / KiriView::ApngRgbaBuffer::bytesPerPixel) {
        return std::nullopt;
    }
    return pixelCount * KiriView::ApngRgbaBuffer::bytesPerPixel;
}

void premultiplyRgbaRow(unsigned char *row, std::size_t width)
{
    for (std::size_t x = 0; x < width; ++x) {
        unsigned char *pixel = row + x * KiriView::ApngRgbaBuffer::bytesPerPixel;
        const unsigned int alpha = pixel[3];
        pixel[0]
            = static_cast<unsigned char>((static_cast<unsigned int>(pixel[0]) * alpha + 127) / 255);
        pixel[1]
            = static_cast<unsigned char>((static_cast<unsigned int>(pixel[1]) * alpha + 127) / 255);
        pixel[2]
            = static_cast<unsigned char>((static_cast<unsigned int>(pixel[2]) * alpha + 127) / 255);
    }
}

unsigned char overChannel(
    unsigned char source, unsigned char destination, unsigned int inverseAlpha)
{
    const unsigned int value
        = source + (static_cast<unsigned int>(destination) * inverseAlpha + 127) / 255;
    return static_cast<unsigned char>(std::min(value, 255U));
}

void blendPixelOver(unsigned char *destination, const unsigned char *source)
{
    const unsigned int inverseAlpha = 255U - source[3];
    destination[0] = overChannel(source[0], destination[0], inverseAlpha);
    destination[1] = overChannel(source[1], destination[1], inverseAlpha);
    destination[2] = overChannel(source[2], destination[2], inverseAlpha);
    destination[3] = overChannel(source[3], destination[3], inverseAlpha);
}
}

namespace KiriView {
bool ApngFrameComposer::initialize(QSize canvasSize, std::size_t rowBytes)
{
    clear();
    if (!m_canvas.initialize(canvasSize, rowBytes) || !m_frame.initialize(canvasSize, rowBytes)) {
        clear();
        return false;
    }
    return true;
}

void ApngFrameComposer::clear()
{
    m_hasDisplayedFrame = false;
    m_canvas.clear();
    m_frame.clear();
}

bool ApngFrameComposer::canComposeFrame(const ApngFrameControl &control) const
{
    return m_canvas.contains(region(control));
}

unsigned char **ApngFrameComposer::frameRows() { return m_frame.rows(); }

std::optional<QImage> ApngFrameComposer::composeFrame(ApngFrameControl control)
{
    if (!canComposeFrame(control)) {
        return std::nullopt;
    }

    if (!m_hasDisplayedFrame) {
        control.blendOp = ApngFrameBlendOp::Source;
        if (control.disposeOp == ApngFrameDisposeOp::Previous) {
            control.disposeOp = ApngFrameDisposeOp::Background;
        }
    }

    premultiplyFrame(control);
    std::optional<std::vector<unsigned char>> previous
        = control.disposeOp == ApngFrameDisposeOp::Previous ? m_canvas.copyRegion(region(control))
                                                            : std::nullopt;
    if (control.disposeOp == ApngFrameDisposeOp::Previous && !previous.has_value()) {
        return std::nullopt;
    }

    if (!blendFrame(control)) {
        return std::nullopt;
    }

    std::optional<QImage> image = m_canvas.imageCopy();
    if (!image.has_value() || !applyDispose(control, previous)) {
        return std::nullopt;
    }

    m_hasDisplayedFrame = true;
    return image;
}

ApngRgbaRegion ApngFrameComposer::region(const ApngFrameControl &control) const
{
    return ApngRgbaRegion { control.width, control.height, control.xOffset, control.yOffset };
}

void ApngFrameComposer::premultiplyFrame(const ApngFrameControl &control)
{
    const std::size_t width = static_cast<std::size_t>(control.width);
    for (quint32 y = 0; y < control.height; ++y) {
        premultiplyRgbaRow(m_frame.row(y), width);
    }
}

bool ApngFrameComposer::blendFrame(const ApngFrameControl &control)
{
    const std::size_t width = static_cast<std::size_t>(control.width);
    const std::optional<std::size_t> rowLength = frameRowBytes(control.width);
    if (!rowLength.has_value()) {
        return false;
    }

    for (quint32 y = 0; y < control.height; ++y) {
        const std::optional<std::size_t> destinationOffset
            = m_canvas.rowOffset(control.xOffset, control.yOffset + y);
        if (!destinationOffset.has_value()) {
            return false;
        }

        unsigned char *destination = m_canvas.data() + *destinationOffset;
        const unsigned char *source = m_frame.row(y);
        if (control.blendOp == ApngFrameBlendOp::Over) {
            for (std::size_t x = 0; x < width; ++x) {
                blendPixelOver(destination + x * ApngRgbaBuffer::bytesPerPixel,
                    source + x * ApngRgbaBuffer::bytesPerPixel);
            }
        } else {
            std::memcpy(destination, source, *rowLength);
        }
    }
    return true;
}

bool ApngFrameComposer::applyDispose(
    const ApngFrameControl &control, const std::optional<std::vector<unsigned char>> &previous)
{
    switch (control.disposeOp) {
    case ApngFrameDisposeOp::Background:
        return m_canvas.clearRegion(region(control));
    case ApngFrameDisposeOp::Previous:
        if (!previous.has_value()) {
            return false;
        }
        return m_canvas.restoreRegion(region(control), *previous);
    case ApngFrameDisposeOp::None:
    default:
        return true;
    }
}
}

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
    if (pixelCount
        > std::numeric_limits<std::size_t>::max() / kiriview::ApngRgbaBuffer::bytesPerPixel) {
        return std::nullopt;
    }
    return pixelCount * kiriview::ApngRgbaBuffer::bytesPerPixel;
}

void premultiplyRgbaRow(unsigned char* row, std::size_t width)
{
    for (std::size_t x = 0; x < width; ++x) {
        unsigned char* pixel = row + x * kiriview::ApngRgbaBuffer::bytesPerPixel;
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

void blendPixelOver(unsigned char* destination, const unsigned char* source)
{
    const unsigned int inverseAlpha = 255U - source[3];
    destination[0] = overChannel(source[0], destination[0], inverseAlpha);
    destination[1] = overChannel(source[1], destination[1], inverseAlpha);
    destination[2] = overChannel(source[2], destination[2], inverseAlpha);
    destination[3] = overChannel(source[3], destination[3], inverseAlpha);
}
}

namespace kiriview {
ApngFrameCompositionPlan apngFrameCompositionPlan(bool hasDisplayedFrame, ApngFrameControl control)
{
    if (!hasDisplayedFrame) {
        control.blendOp = ApngFrameBlendOp::Source;
        if (control.disposeOp == ApngFrameDisposeOp::Previous) {
            control.disposeOp = ApngFrameDisposeOp::Background;
        }
    }

    ApngFrameDisposeAction disposeAction = ApngFrameDisposeAction::None;
    bool capturePreviousRegion = false;
    switch (control.disposeOp) {
    case ApngFrameDisposeOp::Background:
        disposeAction = ApngFrameDisposeAction::ClearFrameRegion;
        break;
    case ApngFrameDisposeOp::Previous:
        capturePreviousRegion = true;
        disposeAction = ApngFrameDisposeAction::RestorePreviousRegion;
        break;
    case ApngFrameDisposeOp::None:
    default:
        break;
    }

    return ApngFrameCompositionPlan {
        control,
        capturePreviousRegion,
        disposeAction,
    };
}

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

bool ApngFrameComposer::canComposeFrame(const ApngFrameControl& control) const
{
    return m_canvas.contains(region(control));
}

unsigned char** ApngFrameComposer::frameRows() { return m_frame.rows(); }

bool ApngFrameComposer::setFrameBytes(const ApngFrameControl& control, const unsigned char* bytes,
    std::size_t byteCount, std::size_t rowBytes)
{
    if (bytes == nullptr || !canComposeFrame(control)) {
        return false;
    }

    const std::optional<std::size_t> expectedRowBytes = frameRowBytes(control.width);
    if (!expectedRowBytes.has_value() || rowBytes != *expectedRowBytes
        || (control.height != 0 && byteCount / static_cast<std::size_t>(control.height) != rowBytes)
        || byteCount != rowBytes * static_cast<std::size_t>(control.height)) {
        return false;
    }

    for (quint32 y = 0; y < control.height; ++y) {
        std::memcpy(m_frame.row(y), bytes + static_cast<std::size_t>(y) * rowBytes, rowBytes);
    }
    return true;
}

std::optional<QImage> ApngFrameComposer::composeFrame(ApngFrameControl control)
{
    if (!canComposeFrame(control)) {
        return std::nullopt;
    }

    const ApngFrameCompositionPlan plan = apngFrameCompositionPlan(m_hasDisplayedFrame, control);
    const ApngFrameControl& displayControl = plan.displayControl;

    premultiplyFrame(displayControl);
    std::optional<std::vector<unsigned char>> previous
        = plan.capturePreviousRegion ? m_canvas.copyRegion(region(displayControl)) : std::nullopt;
    if (plan.capturePreviousRegion && !previous.has_value()) {
        return std::nullopt;
    }

    if (!blendFrame(displayControl)) {
        return std::nullopt;
    }

    std::optional<QImage> image = m_canvas.imageCopy();
    if (!image.has_value() || !applyDispose(plan, previous)) {
        return std::nullopt;
    }

    m_hasDisplayedFrame = true;
    return image;
}

ApngRgbaRegion ApngFrameComposer::region(const ApngFrameControl& control) const
{
    return ApngRgbaRegion { control.width, control.height, control.xOffset, control.yOffset };
}

void ApngFrameComposer::premultiplyFrame(const ApngFrameControl& control)
{
    const std::size_t width = static_cast<std::size_t>(control.width);
    for (quint32 y = 0; y < control.height; ++y) {
        premultiplyRgbaRow(m_frame.row(y), width);
    }
}

bool ApngFrameComposer::blendFrame(const ApngFrameControl& control)
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

        unsigned char* destination = m_canvas.data() + *destinationOffset;
        const unsigned char* source = m_frame.row(y);
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
    const ApngFrameCompositionPlan& plan, const std::optional<std::vector<unsigned char>>& previous)
{
    switch (plan.disposeAction) {
    case ApngFrameDisposeAction::ClearFrameRegion:
        return m_canvas.clearRegion(region(plan.displayControl));
    case ApngFrameDisposeAction::RestorePreviousRegion:
        if (!previous.has_value()) {
            return false;
        }
        return m_canvas.restoreRegion(region(plan.displayControl), *previous);
    case ApngFrameDisposeAction::None:
    default:
        return true;
    }
}
}

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "apngframecomposer.h"

#include <QColorSpace>
#include <algorithm>
#include <cstring>
#include <limits>

namespace {
constexpr std::size_t rgbaBytesPerPixel = 4;

std::optional<std::size_t> checkedMul(std::size_t lhs, std::size_t rhs)
{
    if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max() / lhs) {
        return std::nullopt;
    }
    return lhs * rhs;
}

std::optional<std::size_t> checkedAdd(std::size_t lhs, std::size_t rhs)
{
    if (rhs > std::numeric_limits<std::size_t>::max() - lhs) {
        return std::nullopt;
    }
    return lhs + rhs;
}

std::optional<std::size_t> frameRowBytes(quint32 width)
{
    return checkedMul(static_cast<std::size_t>(width), rgbaBytesPerPixel);
}

void premultiplyRgbaRow(unsigned char *row, std::size_t width)
{
    for (std::size_t x = 0; x < width; ++x) {
        unsigned char *pixel = row + x * rgbaBytesPerPixel;
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
    if (canvasSize.isEmpty()) {
        return false;
    }

    const std::optional<std::size_t> minimumRowBytes
        = frameRowBytes(static_cast<quint32>(canvasSize.width()));
    if (!minimumRowBytes.has_value() || rowBytes < *minimumRowBytes) {
        return false;
    }

    const std::optional<std::size_t> bufferSize
        = checkedMul(rowBytes, static_cast<std::size_t>(canvasSize.height()));
    if (!bufferSize.has_value()
        || *bufferSize > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        return false;
    }

    m_canvasSize = canvasSize;
    m_rowBytes = rowBytes;
    m_canvas.assign(*bufferSize, 0);
    m_frame.assign(*bufferSize, 0);
    m_frameRows.resize(static_cast<std::size_t>(canvasSize.height()));
    for (std::size_t y = 0; y < m_frameRows.size(); ++y) {
        m_frameRows[y] = m_frame.data() + y * rowBytes;
    }
    return true;
}

void ApngFrameComposer::clear()
{
    m_canvasSize = QSize();
    m_rowBytes = 0;
    m_hasDisplayedFrame = false;
    m_canvas.clear();
    m_frame.clear();
    m_frameRows.clear();
}

bool ApngFrameComposer::canComposeFrame(const ApngFrameControl &control) const
{
    const auto right = static_cast<quint64>(control.xOffset) + control.width;
    const auto bottom = static_cast<quint64>(control.yOffset) + control.height;
    return !m_canvasSize.isEmpty() && control.width > 0 && control.height > 0
        && right <= static_cast<quint64>(m_canvasSize.width())
        && bottom <= static_cast<quint64>(m_canvasSize.height());
}

unsigned char **ApngFrameComposer::frameRows()
{
    return m_frameRows.empty() ? nullptr : m_frameRows.data();
}

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
        = control.disposeOp == ApngFrameDisposeOp::Previous ? copyRegion(control) : std::nullopt;
    if (control.disposeOp == ApngFrameDisposeOp::Previous && !previous.has_value()) {
        return std::nullopt;
    }

    if (!blendFrame(control)) {
        return std::nullopt;
    }

    std::optional<QImage> image = canvasImage();
    if (!image.has_value() || !applyDispose(control, previous)) {
        return std::nullopt;
    }

    m_hasDisplayedFrame = true;
    return image;
}

std::optional<std::size_t> ApngFrameComposer::rowOffset(quint32 x, quint32 y) const
{
    const std::optional<std::size_t> rowStart = checkedMul(static_cast<std::size_t>(y), m_rowBytes);
    const std::optional<std::size_t> xOffset
        = checkedMul(static_cast<std::size_t>(x), rgbaBytesPerPixel);
    if (!rowStart.has_value() || !xOffset.has_value()) {
        return std::nullopt;
    }
    return checkedAdd(*rowStart, *xOffset);
}

void ApngFrameComposer::premultiplyFrame(const ApngFrameControl &control)
{
    const std::size_t width = static_cast<std::size_t>(control.width);
    for (quint32 y = 0; y < control.height; ++y) {
        premultiplyRgbaRow(m_frame.data() + y * m_rowBytes, width);
    }
}

std::optional<std::vector<unsigned char>> ApngFrameComposer::copyRegion(
    const ApngFrameControl &control) const
{
    const std::optional<std::size_t> rowLength = frameRowBytes(control.width);
    const std::optional<std::size_t> byteLength = rowLength.has_value()
        ? checkedMul(*rowLength, static_cast<std::size_t>(control.height))
        : std::nullopt;
    if (!rowLength.has_value() || !byteLength.has_value()) {
        return std::nullopt;
    }

    std::vector<unsigned char> region(*byteLength);
    for (quint32 y = 0; y < control.height; ++y) {
        const std::optional<std::size_t> sourceOffset
            = rowOffset(control.xOffset, control.yOffset + y);
        if (!sourceOffset.has_value()) {
            return std::nullopt;
        }
        std::memcpy(region.data() + static_cast<std::size_t>(y) * *rowLength,
            m_canvas.data() + *sourceOffset, *rowLength);
    }
    return region;
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
            = rowOffset(control.xOffset, control.yOffset + y);
        if (!destinationOffset.has_value()) {
            return false;
        }

        unsigned char *destination = m_canvas.data() + *destinationOffset;
        const unsigned char *source = m_frame.data() + static_cast<std::size_t>(y) * m_rowBytes;
        if (control.blendOp == ApngFrameBlendOp::Over) {
            for (std::size_t x = 0; x < width; ++x) {
                blendPixelOver(destination + x * rgbaBytesPerPixel, source + x * rgbaBytesPerPixel);
            }
        } else {
            std::memcpy(destination, source, *rowLength);
        }
    }
    return true;
}

std::optional<QImage> ApngFrameComposer::canvasImage() const
{
    if (m_canvasSize.width() > std::numeric_limits<int>::max()
        || m_canvasSize.height() > std::numeric_limits<int>::max()
        || m_rowBytes > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        return std::nullopt;
    }

    const QImage borrowedImage(m_canvas.data(), m_canvasSize.width(), m_canvasSize.height(),
        static_cast<qsizetype>(m_rowBytes), QImage::Format_RGBA8888_Premultiplied);
    if (borrowedImage.isNull()) {
        return std::nullopt;
    }

    QImage image = borrowedImage.copy();
    image.setColorSpace(QColorSpace(QColorSpace::SRgb));
    return image;
}

bool ApngFrameComposer::applyDispose(
    const ApngFrameControl &control, const std::optional<std::vector<unsigned char>> &previous)
{
    const std::optional<std::size_t> rowLength = frameRowBytes(control.width);
    if (!rowLength.has_value()) {
        return false;
    }

    switch (control.disposeOp) {
    case ApngFrameDisposeOp::Background:
        for (quint32 y = 0; y < control.height; ++y) {
            const std::optional<std::size_t> destinationOffset
                = rowOffset(control.xOffset, control.yOffset + y);
            if (!destinationOffset.has_value()) {
                return false;
            }
            std::memset(m_canvas.data() + *destinationOffset, 0, *rowLength);
        }
        break;
    case ApngFrameDisposeOp::Previous:
        if (!previous.has_value()) {
            return false;
        }
        for (quint32 y = 0; y < control.height; ++y) {
            const std::optional<std::size_t> destinationOffset
                = rowOffset(control.xOffset, control.yOffset + y);
            if (!destinationOffset.has_value()) {
                return false;
            }
            std::memcpy(m_canvas.data() + *destinationOffset,
                previous->data() + static_cast<std::size_t>(y) * *rowLength, *rowLength);
        }
        break;
    case ApngFrameDisposeOp::None:
    default:
        break;
    }
    return true;
}
}

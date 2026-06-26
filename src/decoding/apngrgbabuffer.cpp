// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "apngrgbabuffer.h"

#include <QColorSpace>
#include <cstring>
#include <limits>

namespace {
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

std::optional<std::size_t> rgbaRowBytes(quint32 width)
{
    return checkedMul(static_cast<std::size_t>(width), kiriview::ApngRgbaBuffer::bytesPerPixel);
}
}

namespace kiriview {
bool ApngRgbaBuffer::initialize(QSize imageSize, std::size_t rowBytes)
{
    clear();
    if (imageSize.isEmpty()) {
        return false;
    }

    const std::optional<std::size_t> minimumRowBytes
        = rgbaRowBytes(static_cast<quint32>(imageSize.width()));
    if (!minimumRowBytes.has_value() || rowBytes < *minimumRowBytes) {
        return false;
    }

    const std::optional<std::size_t> bufferSize
        = checkedMul(rowBytes, static_cast<std::size_t>(imageSize.height()));
    if (!bufferSize.has_value()
        || *bufferSize > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        return false;
    }

    m_imageSize = imageSize;
    m_rowBytes = rowBytes;
    m_bytes.assign(*bufferSize, 0);
    m_rows.resize(static_cast<std::size_t>(imageSize.height()));
    for (std::size_t y = 0; y < m_rows.size(); ++y) {
        m_rows[y] = m_bytes.data() + y * rowBytes;
    }
    return true;
}

void ApngRgbaBuffer::clear()
{
    m_imageSize = QSize();
    m_rowBytes = 0;
    m_bytes.clear();
    m_rows.clear();
}

bool ApngRgbaBuffer::isValid() const { return !m_imageSize.isEmpty() && !m_bytes.empty(); }

QSize ApngRgbaBuffer::imageSize() const { return m_imageSize; }

std::size_t ApngRgbaBuffer::rowBytes() const { return m_rowBytes; }

unsigned char* ApngRgbaBuffer::data() { return m_bytes.empty() ? nullptr : m_bytes.data(); }

const unsigned char* ApngRgbaBuffer::data() const
{
    return m_bytes.empty() ? nullptr : m_bytes.data();
}

unsigned char* ApngRgbaBuffer::row(std::size_t y)
{
    return y >= m_rows.size() ? nullptr : m_rows[y];
}

const unsigned char* ApngRgbaBuffer::row(std::size_t y) const
{
    return y >= m_rows.size() ? nullptr : m_rows[y];
}

unsigned char** ApngRgbaBuffer::rows() { return m_rows.empty() ? nullptr : m_rows.data(); }

bool ApngRgbaBuffer::contains(ApngRgbaRegion region) const
{
    const auto right = static_cast<quint64>(region.xOffset) + region.width;
    const auto bottom = static_cast<quint64>(region.yOffset) + region.height;
    return isValid() && region.width > 0 && region.height > 0
        && right <= static_cast<quint64>(m_imageSize.width())
        && bottom <= static_cast<quint64>(m_imageSize.height());
}

std::optional<std::size_t> ApngRgbaBuffer::rowOffset(quint32 x, quint32 y) const
{
    const std::optional<std::size_t> rowStart = checkedMul(static_cast<std::size_t>(y), m_rowBytes);
    const std::optional<std::size_t> xOffset
        = checkedMul(static_cast<std::size_t>(x), bytesPerPixel);
    if (!rowStart.has_value() || !xOffset.has_value()) {
        return std::nullopt;
    }
    return checkedAdd(*rowStart, *xOffset);
}

std::optional<std::vector<unsigned char>> ApngRgbaBuffer::copyRegion(ApngRgbaRegion region) const
{
    if (!contains(region)) {
        return std::nullopt;
    }

    const std::optional<std::size_t> rowLength = rgbaRowBytes(region.width);
    const std::optional<std::size_t> byteLength = rowLength.has_value()
        ? checkedMul(*rowLength, static_cast<std::size_t>(region.height))
        : std::nullopt;
    if (!rowLength.has_value() || !byteLength.has_value()) {
        return std::nullopt;
    }

    std::vector<unsigned char> bytes(*byteLength);
    for (quint32 y = 0; y < region.height; ++y) {
        const std::optional<std::size_t> sourceOffset
            = rowOffset(region.xOffset, region.yOffset + y);
        if (!sourceOffset.has_value()) {
            return std::nullopt;
        }
        std::memcpy(bytes.data() + static_cast<std::size_t>(y) * *rowLength,
            m_bytes.data() + *sourceOffset, *rowLength);
    }
    return bytes;
}

bool ApngRgbaBuffer::clearRegion(ApngRgbaRegion region)
{
    if (!contains(region)) {
        return false;
    }

    const std::optional<std::size_t> rowLength = rgbaRowBytes(region.width);
    if (!rowLength.has_value()) {
        return false;
    }

    for (quint32 y = 0; y < region.height; ++y) {
        const std::optional<std::size_t> destinationOffset
            = rowOffset(region.xOffset, region.yOffset + y);
        if (!destinationOffset.has_value()) {
            return false;
        }
        std::memset(m_bytes.data() + *destinationOffset, 0, *rowLength);
    }
    return true;
}

bool ApngRgbaBuffer::restoreRegion(ApngRgbaRegion region, const std::vector<unsigned char>& bytes)
{
    if (!contains(region)) {
        return false;
    }

    const std::optional<std::size_t> rowLength = rgbaRowBytes(region.width);
    const std::optional<std::size_t> byteLength = rowLength.has_value()
        ? checkedMul(*rowLength, static_cast<std::size_t>(region.height))
        : std::nullopt;
    if (!rowLength.has_value() || !byteLength.has_value() || bytes.size() != *byteLength) {
        return false;
    }

    for (quint32 y = 0; y < region.height; ++y) {
        const std::optional<std::size_t> destinationOffset
            = rowOffset(region.xOffset, region.yOffset + y);
        if (!destinationOffset.has_value()) {
            return false;
        }
        std::memcpy(m_bytes.data() + *destinationOffset,
            bytes.data() + static_cast<std::size_t>(y) * *rowLength, *rowLength);
    }
    return true;
}

std::optional<QImage> ApngRgbaBuffer::imageCopy() const
{
    if (!isValid() || m_imageSize.width() > std::numeric_limits<int>::max()
        || m_imageSize.height() > std::numeric_limits<int>::max()
        || m_rowBytes > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        return std::nullopt;
    }

    const QImage borrowedImage(m_bytes.data(), m_imageSize.width(), m_imageSize.height(),
        static_cast<qsizetype>(m_rowBytes), QImage::Format_RGBA8888_Premultiplied);
    if (borrowedImage.isNull()) {
        return std::nullopt;
    }

    QImage image = borrowedImage.copy();
    image.setColorSpace(QColorSpace(QColorSpace::SRgb));
    return image;
}
}

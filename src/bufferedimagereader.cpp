// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bufferedimagereader.h"

#include <QIODevice>
#include <QImageReader>
#include <memory>

namespace {
QByteArray bmffImageFormat(const QByteArray &data)
{
    if (data.size() < 12 || data.mid(4, 4) != QByteArrayLiteral("ftyp")) {
        return {};
    }

    const QByteArray majorBrand = data.mid(8, 4);
    if (majorBrand == QByteArrayLiteral("avif") || majorBrand == QByteArrayLiteral("avis")) {
        return QByteArrayLiteral("avif");
    }
    if (majorBrand == QByteArrayLiteral("jp2 ")) {
        return QByteArrayLiteral("jp2");
    }
    if (majorBrand == QByteArrayLiteral("jxl ")) {
        return QByteArrayLiteral("jxl");
    }
    if (majorBrand == QByteArrayLiteral("heic") || majorBrand == QByteArrayLiteral("heix")
        || majorBrand == QByteArrayLiteral("hevc") || majorBrand == QByteArrayLiteral("hevx")
        || majorBrand == QByteArrayLiteral("heim") || majorBrand == QByteArrayLiteral("heis")
        || majorBrand == QByteArrayLiteral("hevm") || majorBrand == QByteArrayLiteral("hevs")
        || majorBrand == QByteArrayLiteral("mif1") || majorBrand == QByteArrayLiteral("msf1")) {
        return QByteArrayLiteral("heif");
    }

    return {};
}

QByteArray inferredImageFormat(const QByteArray &data)
{
    if (data.startsWith(QByteArrayLiteral("\x89PNG\r\n\x1a\n"))) {
        return QByteArrayLiteral("png");
    }
    if (data.startsWith(QByteArrayLiteral("\xff\xd8\xff"))) {
        return QByteArrayLiteral("jpeg");
    }
    if (data.startsWith(QByteArrayLiteral("GIF87a"))
        || data.startsWith(QByteArrayLiteral("GIF89a"))) {
        return QByteArrayLiteral("gif");
    }
    if (data.size() >= 12 && data.startsWith(QByteArrayLiteral("RIFF"))
        && data.mid(8, 4) == QByteArrayLiteral("WEBP")) {
        return QByteArrayLiteral("webp");
    }
    if (data.startsWith(QByteArrayLiteral("BM"))) {
        return QByteArrayLiteral("bmp");
    }
    if (data.startsWith(QByteArrayLiteral("II*\0"))
        || data.startsWith(QByteArrayLiteral("MM\0*"))) {
        return QByteArrayLiteral("tiff");
    }
    if (data.startsWith(QByteArrayLiteral("\xff\x0a"))) {
        return QByteArrayLiteral("jxl");
    }
    if (data.startsWith(QByteArrayLiteral("\0\0\0\x0cjP  \r\n\x87\n"))) {
        return QByteArrayLiteral("jp2");
    }

    return bmffImageFormat(data);
}
}

namespace KiriView {
BufferedImageReader::BufferedImageReader(
    const QByteArray &data, const QByteArray &format, bool autoTransform)
{
    m_buffer.setData(data);
    if (!m_buffer.open(QIODevice::ReadOnly)) {
        return;
    }

    const QByteArray readerFormat = format.isEmpty() ? inferredImageFormat(data) : format;
    if (readerFormat.isEmpty()) {
        return;
    }

    m_reader = std::make_unique<QImageReader>(&m_buffer, readerFormat);
    m_reader->setAutoTransform(autoTransform);
}

BufferedImageReader::~BufferedImageReader() = default;

BufferedImageReader::operator bool() const { return m_reader != nullptr; }

bool BufferedImageReader::canRead() const { return m_reader != nullptr && m_reader->canRead(); }

bool BufferedImageReader::supportsAnimation() const
{
    return m_reader != nullptr && m_reader->supportsAnimation();
}

QSize BufferedImageReader::size() const { return m_reader == nullptr ? QSize() : m_reader->size(); }

QByteArray BufferedImageReader::format() const
{
    return m_reader == nullptr ? QByteArray() : m_reader->format();
}

QImageIOHandler::Transformations BufferedImageReader::transformation() const
{
    return m_reader == nullptr ? QImageIOHandler::TransformationNone : m_reader->transformation();
}

int BufferedImageReader::nextImageDelay() const
{
    return m_reader == nullptr ? -1 : m_reader->nextImageDelay();
}

int BufferedImageReader::loopCount() const
{
    return m_reader == nullptr ? 0 : m_reader->loopCount();
}

QString BufferedImageReader::errorString() const
{
    return m_reader == nullptr ? QString() : m_reader->errorString();
}

void BufferedImageReader::setScaledSize(const QSize &size)
{
    if (m_reader != nullptr) {
        m_reader->setScaledSize(size);
    }
}

void BufferedImageReader::setScaledClipRect(const QRect &rect)
{
    if (m_reader != nullptr) {
        m_reader->setScaledClipRect(rect);
    }
}

void BufferedImageReader::setClipRect(const QRect &rect)
{
    if (m_reader != nullptr) {
        m_reader->setClipRect(rect);
    }
}

QImage BufferedImageReader::read() { return m_reader == nullptr ? QImage() : m_reader->read(); }
}

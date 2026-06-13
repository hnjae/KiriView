// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bufferedimagereader.h"

#include <QIODevice>
#include <QImageReader>
#include <memory>

namespace kiriview {
BufferedImageReader::BufferedImageReader(
    const QByteArray &data, const QByteArray &format, bool autoTransform)
{
    if (format.isEmpty()) {
        return;
    }

    m_buffer.setData(data);
    if (!m_buffer.open(QIODevice::ReadOnly)) {
        return;
    }

    m_reader = std::make_unique<QImageReader>(&m_buffer, format);
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

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_BUFFEREDIMAGEREADER_H
#define KIRIVIEW_BUFFEREDIMAGEREADER_H

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QImageIOHandler>
#include <QRect>
#include <QSize>
#include <QString>
#include <memory>

class QImageReader;

namespace KiriView {
class BufferedImageReader final
{
public:
    explicit BufferedImageReader(
        const QByteArray &data, const QByteArray &format, bool autoTransform = true);
    ~BufferedImageReader();

    explicit operator bool() const;
    bool canRead() const;
    bool supportsAnimation() const;
    QSize size() const;
    QByteArray format() const;
    QImageIOHandler::Transformations transformation() const;
    int nextImageDelay() const;
    int loopCount() const;
    QString errorString() const;

    void setScaledSize(const QSize &size);
    void setScaledClipRect(const QRect &rect);
    void setClipRect(const QRect &rect);
    QImage read();

private:
    QBuffer m_buffer;
    std::unique_ptr<QImageReader> m_reader;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationplaybacksource.h"

#include "decoding/apnganimationreader.h"
#include "decoding/bufferedimagereader.h"
#include "decoding/heifsequencereader.h"
#include "decoding/jxlanimationreader.h"
#include "decoding/webpanimationreader.h"
#include "localization/imageerrortext.h"

#include <QImage>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace {
KiriView::ImageAnimationPlaybackOpenResult playbackOpenResult(
    QImage firstFrame, int firstFrameDelay, int loopCount, bool sourceHasMoreFrames)
{
    return KiriView::ImageAnimationPlaybackOpenResult {
        KiriView::ImageAnimationPlaybackOpenStatus::Success,
        std::move(firstFrame),
        firstFrameDelay,
        loopCount,
        sourceHasMoreFrames,
        {},
    };
}

KiriView::ImageAnimationPlaybackOpenResult playbackOpenError(QString errorString)
{
    KiriView::ImageAnimationPlaybackOpenResult result;
    result.errorString = std::move(errorString);
    return result;
}

KiriView::ImageAnimationPlaybackReadResult playbackReadFrame(
    KiriView::AnimationFrame frame, bool sourceHasMoreFrames)
{
    return KiriView::ImageAnimationPlaybackReadResult {
        KiriView::ImageAnimationPlaybackReadStatus::Frame,
        std::move(frame),
        sourceHasMoreFrames,
        {},
    };
}

KiriView::ImageAnimationPlaybackReadResult playbackReadEnd()
{
    return KiriView::ImageAnimationPlaybackReadResult {
        KiriView::ImageAnimationPlaybackReadStatus::End,
        {},
        false,
        {},
    };
}

KiriView::ImageAnimationPlaybackReadResult playbackReadError(QString errorString)
{
    return KiriView::ImageAnimationPlaybackReadResult {
        KiriView::ImageAnimationPlaybackReadStatus::Error,
        {},
        false,
        std::move(errorString),
    };
}

class ReaderAnimationPlaybackSource final : public KiriView::ImageAnimationPlaybackSource
{
public:
    ReaderAnimationPlaybackSource(QByteArray data, QByteArray format)
        : m_data(std::move(data))
        , m_format(std::move(format))
    {
    }

    KiriView::ImageAnimationPlaybackOpenResult open() override
    {
        m_reader.reset();

        auto reader = std::make_unique<KiriView::BufferedImageReader>(m_data, m_format);
        if (!*reader) {
            return playbackOpenError(
                KiriView::imageErrorText(KiriView::ImageErrorTextId::ReadImageData));
        }

        if (!reader->canRead()) {
            return playbackOpenError(reader->errorString());
        }

        QImage firstFrame = reader->read();
        if (firstFrame.isNull()) {
            return playbackOpenError(reader->errorString());
        }

        const int firstFrameDelay = reader->nextImageDelay();
        const int loopCount = reader->loopCount();
        const bool sourceHasMoreFrames = reader->canRead();
        m_reader = std::move(reader);
        return playbackOpenResult(
            std::move(firstFrame), firstFrameDelay, loopCount, sourceHasMoreFrames);
    }

    KiriView::ImageAnimationPlaybackReadResult readNextFrame() override
    {
        if (m_reader == nullptr || !m_reader->canRead()) {
            return playbackReadEnd();
        }

        QImage frame = m_reader->read();
        if (frame.isNull()) {
            return playbackReadError(m_reader->errorString());
        }

        return playbackReadFrame(
            KiriView::AnimationFrame {
                std::move(frame),
                m_reader->nextImageDelay(),
            },
            m_reader->canRead());
    }

    bool restartable() const override { return true; }

private:
    QByteArray m_data;
    QByteArray m_format;
    std::unique_ptr<KiriView::BufferedImageReader> m_reader;
};

class ApngAnimationPlaybackSource final : public KiriView::ImageAnimationPlaybackSource
{
public:
    explicit ApngAnimationPlaybackSource(QByteArray data)
        : m_data(std::move(data))
    {
    }

    KiriView::ImageAnimationPlaybackOpenResult open() override
    {
        m_reader = std::make_unique<KiriView::ApngAnimationReader>();
        KiriView::ApngOpenResult openResult = m_reader->open(m_data);
        switch (openResult.status) {
        case KiriView::ApngOpenStatus::Success:
            return playbackOpenResult(std::move(openResult.firstFrame), openResult.firstFrameDelay,
                openResult.loopCount, openResult.frameCount > 1);
        case KiriView::ApngOpenStatus::Error:
            m_reader.reset();
            return playbackOpenError(openResult.errorString);
        case KiriView::ApngOpenStatus::NotApng:
            m_reader.reset();
            return playbackOpenError(
                KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeApngAnimation));
        }

        m_reader.reset();
        return playbackOpenError(
            KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeApngAnimation));
    }

    KiriView::ImageAnimationPlaybackReadResult readNextFrame() override
    {
        if (m_reader == nullptr) {
            return playbackReadEnd();
        }

        QString errorString;
        std::optional<KiriView::AnimationFrame> frame = m_reader->readNextFrame(&errorString);
        if (frame.has_value()) {
            return playbackReadFrame(std::move(*frame), m_reader->hasMoreFrames());
        }
        if (!errorString.isEmpty()) {
            return playbackReadError(std::move(errorString));
        }
        return playbackReadEnd();
    }

    bool restartable() const override { return true; }

private:
    QByteArray m_data;
    std::unique_ptr<KiriView::ApngAnimationReader> m_reader;
};

class WebPAnimationPlaybackSource final : public KiriView::ImageAnimationPlaybackSource
{
public:
    explicit WebPAnimationPlaybackSource(QByteArray data)
        : m_data(std::move(data))
    {
    }

    KiriView::ImageAnimationPlaybackOpenResult open() override
    {
        m_reader = std::make_unique<KiriView::WebPAnimationReader>();
        KiriView::WebPAnimationOpenResult openResult = m_reader->open(m_data);
        switch (openResult.status) {
        case KiriView::WebPAnimationOpenStatus::Success:
            return playbackOpenResult(std::move(openResult.firstFrame), openResult.firstFrameDelay,
                openResult.loopCount, openResult.sourceHasMoreFrames);
        case KiriView::WebPAnimationOpenStatus::Error:
            m_reader.reset();
            return playbackOpenError(openResult.errorString);
        case KiriView::WebPAnimationOpenStatus::NotWebP:
        case KiriView::WebPAnimationOpenStatus::NotAnimation:
            m_reader.reset();
            return playbackOpenError(
                KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeImageAnimation));
        }

        m_reader.reset();
        return playbackOpenError(
            KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeImageAnimation));
    }

    KiriView::ImageAnimationPlaybackReadResult readNextFrame() override
    {
        if (m_reader == nullptr) {
            return playbackReadEnd();
        }

        QString errorString;
        std::optional<KiriView::AnimationFrame> frame = m_reader->readNextFrame(&errorString);
        if (frame.has_value()) {
            return playbackReadFrame(std::move(*frame), m_reader->hasMoreFrames());
        }
        if (!errorString.isEmpty()) {
            return playbackReadError(std::move(errorString));
        }
        return playbackReadEnd();
    }

    bool restartable() const override { return true; }

private:
    QByteArray m_data;
    std::unique_ptr<KiriView::WebPAnimationReader> m_reader;
};

class JxlAnimationPlaybackSource final : public KiriView::ImageAnimationPlaybackSource
{
public:
    explicit JxlAnimationPlaybackSource(QByteArray data)
        : m_data(std::move(data))
    {
    }

    KiriView::ImageAnimationPlaybackOpenResult open() override
    {
        m_reader = std::make_unique<KiriView::JxlAnimationReader>();
        KiriView::JxlAnimationOpenResult openResult = m_reader->open(m_data);
        switch (openResult.status) {
        case KiriView::JxlAnimationOpenStatus::Success:
            return playbackOpenResult(std::move(openResult.firstFrame), openResult.firstFrameDelay,
                openResult.loopCount, openResult.sourceHasMoreFrames);
        case KiriView::JxlAnimationOpenStatus::Error:
            m_reader.reset();
            return playbackOpenError(openResult.errorString);
        case KiriView::JxlAnimationOpenStatus::NotJxl:
        case KiriView::JxlAnimationOpenStatus::NotAnimation:
            m_reader.reset();
            return playbackOpenError(
                KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeImageAnimation));
        }

        m_reader.reset();
        return playbackOpenError(
            KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeImageAnimation));
    }

    KiriView::ImageAnimationPlaybackReadResult readNextFrame() override
    {
        if (m_reader == nullptr) {
            return playbackReadEnd();
        }

        QString errorString;
        std::optional<KiriView::AnimationFrame> frame = m_reader->readNextFrame(&errorString);
        if (frame.has_value()) {
            return playbackReadFrame(std::move(*frame), true);
        }
        if (!errorString.isEmpty()) {
            return playbackReadError(std::move(errorString));
        }
        return playbackReadEnd();
    }

    bool restartable() const override { return true; }

private:
    QByteArray m_data;
    std::unique_ptr<KiriView::JxlAnimationReader> m_reader;
};

class HeifSequenceAnimationPlaybackSource final : public KiriView::ImageAnimationPlaybackSource
{
public:
    explicit HeifSequenceAnimationPlaybackSource(QByteArray data)
        : m_data(std::move(data))
    {
    }

    KiriView::ImageAnimationPlaybackOpenResult open() override
    {
        m_reader = std::make_unique<KiriView::HeifSequenceReader>();
        const KiriView::HeifSequenceOpenResult openResult = m_reader->open(m_data);
        if (openResult.status != KiriView::HeifSequenceOpenStatus::Success) {
            m_reader.reset();
            return playbackOpenError(openResult.errorString.isEmpty()
                    ? KiriView::heifSequenceDecodeErrorString()
                    : openResult.errorString);
        }

        QString errorString;
        std::optional<KiriView::AnimationFrame> firstFrame = m_reader->readNextFrame(&errorString);
        if (!firstFrame.has_value()) {
            m_reader.reset();
            return playbackOpenError(
                errorString.isEmpty() ? KiriView::heifSequenceDecodeErrorString() : errorString);
        }

        return playbackOpenResult(std::move(firstFrame->image), firstFrame->delay, 0, true);
    }

    KiriView::ImageAnimationPlaybackReadResult readNextFrame() override
    {
        if (m_reader == nullptr) {
            return playbackReadEnd();
        }

        QString errorString;
        std::optional<KiriView::AnimationFrame> frame = m_reader->readNextFrame(&errorString);
        if (frame.has_value()) {
            return playbackReadFrame(std::move(*frame), true);
        }
        if (!errorString.isEmpty()) {
            return playbackReadError(std::move(errorString));
        }
        return playbackReadEnd();
    }

    bool restartable() const override { return false; }

private:
    QByteArray m_data;
    std::unique_ptr<KiriView::HeifSequenceReader> m_reader;
};

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makeReaderPlaybackSource(
    KiriView::ReaderAnimationPlaybackRequest request)
{
    return std::make_unique<ReaderAnimationPlaybackSource>(
        std::move(request.data), std::move(request.format));
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makeApngPlaybackSource(
    KiriView::ApngAnimationPlaybackRequest request)
{
    return std::make_unique<ApngAnimationPlaybackSource>(std::move(request.data));
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makeWebPPlaybackSource(
    KiriView::WebPAnimationPlaybackRequest request)
{
    return std::make_unique<WebPAnimationPlaybackSource>(std::move(request.data));
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makeJxlPlaybackSource(
    KiriView::JxlAnimationPlaybackRequest request)
{
    return std::make_unique<JxlAnimationPlaybackSource>(std::move(request.data));
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makeHeifSequencePlaybackSource(
    KiriView::HeifSequenceAnimationPlaybackRequest request)
{
    return std::make_unique<HeifSequenceAnimationPlaybackSource>(std::move(request.data));
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makePlaybackSource(std::monostate)
{
    return {};
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makePlaybackSource(
    KiriView::ReaderAnimationPlaybackRequest request)
{
    return makeReaderPlaybackSource(std::move(request));
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makePlaybackSource(
    KiriView::ApngAnimationPlaybackRequest request)
{
    return makeApngPlaybackSource(std::move(request));
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makePlaybackSource(
    KiriView::WebPAnimationPlaybackRequest request)
{
    return makeWebPPlaybackSource(std::move(request));
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makePlaybackSource(
    KiriView::JxlAnimationPlaybackRequest request)
{
    return makeJxlPlaybackSource(std::move(request));
}

std::unique_ptr<KiriView::ImageAnimationPlaybackSource> makePlaybackSource(
    KiriView::HeifSequenceAnimationPlaybackRequest request)
{
    return makeHeifSequencePlaybackSource(std::move(request));
}
}

namespace KiriView {
bool ImageAnimationPlaybackRequest::isValid() const
{
    return !std::holds_alternative<std::monostate>(payload);
}

ImageAnimationPlaybackRequest readerAnimationPlaybackRequest(QByteArray data, QByteArray format)
{
    return ImageAnimationPlaybackRequest {
        ReaderAnimationPlaybackRequest {
            std::move(data),
            std::move(format),
        },
    };
}

ImageAnimationPlaybackRequest apngAnimationPlaybackRequest(QByteArray data)
{
    return ImageAnimationPlaybackRequest {
        ApngAnimationPlaybackRequest {
            std::move(data),
        },
    };
}

ImageAnimationPlaybackRequest webpAnimationPlaybackRequest(QByteArray data)
{
    return ImageAnimationPlaybackRequest {
        WebPAnimationPlaybackRequest {
            std::move(data),
        },
    };
}

ImageAnimationPlaybackRequest jxlAnimationPlaybackRequest(QByteArray data)
{
    return ImageAnimationPlaybackRequest {
        JxlAnimationPlaybackRequest {
            std::move(data),
        },
    };
}

ImageAnimationPlaybackRequest heifSequenceAnimationPlaybackRequest(QByteArray data)
{
    return ImageAnimationPlaybackRequest {
        HeifSequenceAnimationPlaybackRequest {
            std::move(data),
        },
    };
}

std::unique_ptr<ImageAnimationPlaybackSource> makeImageAnimationPlaybackSource(
    ImageAnimationPlaybackRequest request)
{
    return std::visit(
        [](auto &&playbackRequest) { return makePlaybackSource(std::move(playbackRequest)); },
        std::move(request.payload));
}
}

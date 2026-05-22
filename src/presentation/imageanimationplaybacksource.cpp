// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationplaybacksource.h"

#include "decoding/apnganimationreader.h"
#include "decoding/bufferedimagereader.h"
#include "decoding/heifsequencereader.h"
#include "localization/imageerrortext.h"

#include <QImage>
#include <memory>
#include <optional>
#include <utility>

namespace {
KiriView::ImageAnimationPlaybackOpenResult playbackOpenResult(
    QImage firstFrame, int firstFrameDelay, int loopCount, bool sourceHasMoreFrames)
{
    return KiriView::ImageAnimationPlaybackOpenResult {
        std::move(firstFrame),
        firstFrameDelay,
        loopCount,
        sourceHasMoreFrames,
    };
}

void clearError(QString *errorString)
{
    if (errorString != nullptr) {
        errorString->clear();
    }
}

void setError(QString *errorString, const QString &message)
{
    if (errorString != nullptr) {
        *errorString = message;
    }
}

class ReaderAnimationPlaybackSource final : public KiriView::ImageAnimationPlaybackSource
{
public:
    ReaderAnimationPlaybackSource(QByteArray data, QByteArray format)
        : m_data(std::move(data))
        , m_format(std::move(format))
    {
    }

    std::optional<KiriView::ImageAnimationPlaybackOpenResult> open(QString *errorString) override
    {
        clearError(errorString);
        m_reader.reset();

        auto reader = std::make_unique<KiriView::BufferedImageReader>(m_data, m_format);
        if (!*reader) {
            setError(
                errorString, KiriView::imageErrorText(KiriView::ImageErrorTextId::ReadImageData));
            return std::nullopt;
        }

        if (!reader->canRead()) {
            setError(errorString, reader->errorString());
            return std::nullopt;
        }

        QImage firstFrame = reader->read();
        if (firstFrame.isNull()) {
            setError(errorString, reader->errorString());
            return std::nullopt;
        }

        const int firstFrameDelay = reader->nextImageDelay();
        const int loopCount = reader->loopCount();
        const bool sourceHasMoreFrames = reader->canRead();
        m_reader = std::move(reader);
        return playbackOpenResult(
            std::move(firstFrame), firstFrameDelay, loopCount, sourceHasMoreFrames);
    }

    std::optional<KiriView::AnimationFrame> readNextFrame(QString *errorString) override
    {
        clearError(errorString);
        if (m_reader == nullptr || !m_reader->canRead()) {
            return std::nullopt;
        }

        QImage frame = m_reader->read();
        if (frame.isNull()) {
            setError(errorString, m_reader->errorString());
            return std::nullopt;
        }

        return KiriView::AnimationFrame {
            std::move(frame),
            m_reader->nextImageDelay(),
        };
    }

    bool hasMoreFrames() const override { return m_reader != nullptr && m_reader->canRead(); }

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

    std::optional<KiriView::ImageAnimationPlaybackOpenResult> open(QString *errorString) override
    {
        clearError(errorString);
        m_reader = std::make_unique<KiriView::ApngAnimationReader>();
        KiriView::ApngOpenResult openResult = m_reader->open(m_data);
        switch (openResult.status) {
        case KiriView::ApngOpenStatus::Success:
            return playbackOpenResult(std::move(openResult.firstFrame), openResult.firstFrameDelay,
                openResult.loopCount, openResult.frameCount > 1);
        case KiriView::ApngOpenStatus::Error:
            setError(errorString, openResult.errorString);
            m_reader.reset();
            return std::nullopt;
        case KiriView::ApngOpenStatus::NotApng:
            setError(errorString,
                KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeApngAnimation));
            m_reader.reset();
            return std::nullopt;
        }

        setError(
            errorString, KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeApngAnimation));
        m_reader.reset();
        return std::nullopt;
    }

    std::optional<KiriView::AnimationFrame> readNextFrame(QString *errorString) override
    {
        clearError(errorString);
        return m_reader == nullptr ? std::nullopt : m_reader->readNextFrame(errorString);
    }

    bool hasMoreFrames() const override { return m_reader != nullptr && m_reader->hasMoreFrames(); }

    bool restartable() const override { return true; }

private:
    QByteArray m_data;
    std::unique_ptr<KiriView::ApngAnimationReader> m_reader;
};

class HeifSequenceAnimationPlaybackSource final : public KiriView::ImageAnimationPlaybackSource
{
public:
    explicit HeifSequenceAnimationPlaybackSource(QByteArray data)
        : m_data(std::move(data))
    {
    }

    std::optional<KiriView::ImageAnimationPlaybackOpenResult> open(QString *errorString) override
    {
        clearError(errorString);
        m_reader = std::make_unique<KiriView::HeifSequenceReader>();
        const KiriView::HeifSequenceOpenResult openResult = m_reader->open(m_data);
        if (openResult.status != KiriView::HeifSequenceOpenStatus::Success) {
            setError(errorString,
                openResult.errorString.isEmpty() ? KiriView::heifSequenceDecodeErrorString()
                                                 : openResult.errorString);
            m_reader.reset();
            return std::nullopt;
        }

        std::optional<KiriView::AnimationFrame> firstFrame = m_reader->readNextFrame(errorString);
        if (!firstFrame.has_value()) {
            if (errorString != nullptr && errorString->isEmpty()) {
                *errorString = KiriView::heifSequenceDecodeErrorString();
            }
            m_reader.reset();
            return std::nullopt;
        }

        return playbackOpenResult(std::move(firstFrame->image), firstFrame->delay, 0, true);
    }

    std::optional<KiriView::AnimationFrame> readNextFrame(QString *errorString) override
    {
        clearError(errorString);
        return m_reader == nullptr ? std::nullopt : m_reader->readNextFrame(errorString);
    }

    bool hasMoreFrames() const override { return m_reader != nullptr; }

    bool restartable() const override { return false; }

private:
    QByteArray m_data;
    std::unique_ptr<KiriView::HeifSequenceReader> m_reader;
};
}

namespace KiriView {
std::unique_ptr<ImageAnimationPlaybackSource> makeReaderAnimationPlaybackSource(
    QByteArray data, QByteArray format)
{
    return std::make_unique<ReaderAnimationPlaybackSource>(std::move(data), std::move(format));
}

std::unique_ptr<ImageAnimationPlaybackSource> makeApngAnimationPlaybackSource(QByteArray data)
{
    return std::make_unique<ApngAnimationPlaybackSource>(std::move(data));
}

std::unique_ptr<ImageAnimationPlaybackSource> makeHeifSequenceAnimationPlaybackSource(
    QByteArray data)
{
    return std::make_unique<HeifSequenceAnimationPlaybackSource>(std::move(data));
}
}

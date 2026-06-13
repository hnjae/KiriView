// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "apnganimationreader.h"

#include "animationtiming.h"
#include "apngframecomposer.h"
#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/apnganimationreader.cxx.h"
#include "localization/imageerrortext.h"

#include <QSize>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace {
QString apngDecodeErrorString()
{
    return kiriview::imageErrorText(kiriview::ImageErrorTextId::DecodeApngAnimation);
}

kiriview::ApngOpenResult errorResult(QString errorString)
{
    kiriview::ApngOpenResult result;
    result.status = kiriview::ApngOpenStatus::Error;
    result.errorString = std::move(errorString);
    return result;
}

kiriview::ApngFrameDisposeOp disposeOpFromRust(kiriview::RustApngDisposeOp disposeOp)
{
    switch (disposeOp) {
    case kiriview::RustApngDisposeOp::Background:
        return kiriview::ApngFrameDisposeOp::Background;
    case kiriview::RustApngDisposeOp::Previous:
        return kiriview::ApngFrameDisposeOp::Previous;
    case kiriview::RustApngDisposeOp::None:
    default:
        return kiriview::ApngFrameDisposeOp::None;
    }
}

kiriview::ApngFrameBlendOp blendOpFromRust(kiriview::RustApngBlendOp blendOp)
{
    return blendOp == kiriview::RustApngBlendOp::Over ? kiriview::ApngFrameBlendOp::Over
                                                      : kiriview::ApngFrameBlendOp::Source;
}

kiriview::ApngFrameControl frameControlFromRust(const kiriview::RustApngFrameResult &frame)
{
    return kiriview::ApngFrameControl {
        frame.width,
        frame.height,
        frame.x_offset,
        frame.y_offset,
        disposeOpFromRust(frame.dispose_op),
        blendOpFromRust(frame.blend_op),
    };
}

bool canRepresentCanvas(quint32 width, quint32 height)
{
    return width > 0 && height > 0 && width <= static_cast<quint32>(std::numeric_limits<int>::max())
        && height <= static_cast<quint32>(std::numeric_limits<int>::max());
}
}

namespace kiriview {
class ApngAnimationReader::Private
{
public:
    ApngOpenResult open(QByteArray inputData)
    {
        close();

        const RustApngOpenResult openResult
            = rustOpenApngAnimationReader(*reader, Bridge::rustBytes(inputData));
        switch (openResult.status) {
        case RustApngOpenStatus::NotApng:
            return {};
        case RustApngOpenStatus::Error:
            close();
            return errorResult(apngDecodeErrorString());
        case RustApngOpenStatus::Success:
            break;
        }

        if (!canRepresentCanvas(openResult.canvas_width, openResult.canvas_height)
            || !composer.initialize(QSize(static_cast<int>(openResult.canvas_width),
                                        static_cast<int>(openResult.canvas_height)),
                static_cast<std::size_t>(openResult.canvas_width)
                    * ApngRgbaBuffer::bytesPerPixel)) {
            close();
            return errorResult(apngDecodeErrorString());
        }

        QString frameError;
        std::optional<AnimationFrame> firstFrame = readNextFrame(&frameError);
        if (!firstFrame.has_value()) {
            close();
            return errorResult(frameError.isEmpty() ? apngDecodeErrorString() : frameError);
        }

        ApngOpenResult result;
        result.status = ApngOpenStatus::Success;
        result.firstFrame = std::move(firstFrame->image);
        result.firstFrameDelay = firstFrame->delay;
        result.loopCount = openResult.loop_count;
        result.frameCount = openResult.frame_count;
        return result;
    }

    std::optional<AnimationFrame> readNextFrame(QString *errorString)
    {
        clearError(errorString);

        const RustApngFrameResult frame = rustReadApngAnimationFrame(*reader);
        switch (frame.status) {
        case RustApngReadStatus::End:
            return std::nullopt;
        case RustApngReadStatus::Error:
            setError(errorString, apngDecodeErrorString());
            close();
            return std::nullopt;
        case RustApngReadStatus::Frame:
            break;
        }

        const ApngFrameControl control = frameControlFromRust(frame);
        if (!composer.setFrameBytes(
                control, frame.pixels.data(), frame.pixels.size(), frame.row_bytes)) {
            setError(errorString, apngDecodeErrorString());
            close();
            return std::nullopt;
        }

        std::optional<QImage> image = composer.composeFrame(control);
        if (!image.has_value()) {
            setError(errorString, apngDecodeErrorString());
            close();
            return std::nullopt;
        }

        return AnimationFrame {
            std::move(*image),
            apngFrameDelay(frame.delay_num, frame.delay_den),
        };
    }

    bool hasMoreFrames() const { return rustApngAnimationReaderHasMoreFrames(*reader); }

    void close()
    {
        reader = rustNewApngAnimationReader();
        composer.clear();
    }

private:
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

    rust::Box<RustApngAnimationReader> reader = rustNewApngAnimationReader();
    ApngFrameComposer composer;
};

ApngAnimationReader::ApngAnimationReader()
    : d(std::make_unique<Private>())
{
}

ApngAnimationReader::~ApngAnimationReader() = default;

ApngOpenResult ApngAnimationReader::open(QByteArray data) { return d->open(std::move(data)); }

std::optional<AnimationFrame> ApngAnimationReader::readNextFrame(QString *errorString)
{
    return d->readNextFrame(errorString);
}

bool ApngAnimationReader::hasMoreFrames() const { return d->hasMoreFrames(); }
}

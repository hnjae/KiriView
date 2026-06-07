// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "jxlanimationreader.h"

#include "animationtiming.h"
#include "localization/imageerrortext.h"
#include "rendering/imagerendering.h"

#include <jxl/decode.h>
#include <jxl/thread_parallel_runner.h>

#include <QColorSpace>
#include <QSize>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
struct JxlDecoderDeleter {
    void operator()(JxlDecoder *decoder) const
    {
        if (decoder != nullptr) {
            JxlDecoderDestroy(decoder);
        }
    }
};

struct JxlThreadRunnerDeleter {
    void operator()(void *runner) const
    {
        if (runner != nullptr) {
            JxlThreadParallelRunnerDestroy(runner);
        }
    }
};

using JxlDecoderPtr = std::unique_ptr<JxlDecoder, JxlDecoderDeleter>;
using JxlThreadRunnerPtr = std::unique_ptr<void, JxlThreadRunnerDeleter>;

enum class JxlReadStatus {
    Frame,
    End,
    NotAnimation,
    Error,
};

struct JxlReadResult {
    JxlReadStatus status = JxlReadStatus::Error;
    KiriView::AnimationFrame frame;
    QString errorString;
};

QString jxlAnimationDecodeErrorString()
{
    return KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeImageAnimation);
}

KiriView::JxlAnimationOpenResult notJxlResult() { return {}; }

KiriView::JxlAnimationOpenResult notAnimationResult()
{
    KiriView::JxlAnimationOpenResult result;
    result.status = KiriView::JxlAnimationOpenStatus::NotAnimation;
    return result;
}

KiriView::JxlAnimationOpenResult errorOpenResult(QString errorString)
{
    KiriView::JxlAnimationOpenResult result;
    result.status = KiriView::JxlAnimationOpenStatus::Error;
    result.errorString = std::move(errorString);
    return result;
}

JxlReadResult errorReadResult(QString errorString)
{
    return JxlReadResult {
        JxlReadStatus::Error,
        {},
        std::move(errorString),
    };
}

bool isJxlData(const QByteArray &data)
{
    if (data.isEmpty()) {
        return false;
    }
    const JxlSignature signature
        = JxlSignatureCheck(reinterpret_cast<const std::uint8_t *>(data.constData()),
            static_cast<std::size_t>(data.size()));
    return signature == JXL_SIG_CODESTREAM || signature == JXL_SIG_CONTAINER;
}

JxlPixelFormat rgbaPixelFormat()
{
    return JxlPixelFormat {
        4,
        JXL_TYPE_UINT8,
        JXL_NATIVE_ENDIAN,
        0,
    };
}

std::optional<QSize> imageSizeForInfo(const JxlBasicInfo &info)
{
    if (info.xsize == 0 || info.ysize == 0
        || info.xsize > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        || info.ysize > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    return QSize(static_cast<int>(info.xsize), static_cast<int>(info.ysize));
}

std::optional<QImage> imageFromRgbaBuffer(const std::vector<std::uint8_t> &buffer, QSize size)
{
    if (buffer.empty() || size.isEmpty()) {
        return std::nullopt;
    }

    const auto width = static_cast<std::size_t>(size.width());
    const auto height = static_cast<std::size_t>(size.height());
    if (width > std::numeric_limits<std::size_t>::max() / 4U) {
        return std::nullopt;
    }
    const std::size_t rowBytes = width * 4U;
    if (height != 0 && rowBytes > std::numeric_limits<std::size_t>::max() / height) {
        return std::nullopt;
    }
    if (buffer.size() < rowBytes * height
        || rowBytes > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
        return std::nullopt;
    }

    const QImage borrowedImage(buffer.data(), size.width(), size.height(),
        static_cast<qsizetype>(rowBytes), QImage::Format_RGBA8888);
    if (borrowedImage.isNull()) {
        return std::nullopt;
    }

    QImage image = borrowedImage.copy();
    image.setColorSpace(QColorSpace(QColorSpace::SRgb));
    return KiriView::displayReadyImage(image);
}
}

namespace KiriView {
class JxlAnimationReader::Private
{
public:
    JxlAnimationOpenResult open(QByteArray inputData)
    {
        reset();
        if (!isJxlData(inputData)) {
            return notJxlResult();
        }

        data = std::move(inputData);
        if (!initializeDecoder()) {
            reset();
            return errorOpenResult(jxlAnimationDecodeErrorString());
        }

        JxlReadResult firstFrame = readFrame();
        if (firstFrame.status == JxlReadStatus::NotAnimation) {
            reset();
            return notAnimationResult();
        }
        if (firstFrame.status != JxlReadStatus::Frame) {
            reset();
            return errorOpenResult(firstFrame.errorString.isEmpty()
                    ? jxlAnimationDecodeErrorString()
                    : firstFrame.errorString);
        }

        JxlReadResult secondFrame = readFrame();
        if (secondFrame.status == JxlReadStatus::End) {
            reset();
            return notAnimationResult();
        }
        if (secondFrame.status != JxlReadStatus::Frame) {
            reset();
            return errorOpenResult(secondFrame.errorString.isEmpty()
                    ? jxlAnimationDecodeErrorString()
                    : secondFrame.errorString);
        }
        bufferedFrame = std::move(secondFrame.frame);

        JxlAnimationOpenResult result;
        result.status = JxlAnimationOpenStatus::Success;
        result.firstFrame = std::move(firstFrame.frame.image);
        result.firstFrameDelay = firstFrame.frame.delay;
        result.loopCount = loopCount;
        result.sourceHasMoreFrames = true;
        return result;
    }

    std::optional<AnimationFrame> readNextFrame(QString *errorString)
    {
        clearError(errorString);
        if (bufferedFrame.has_value()) {
            AnimationFrame frame = std::move(*bufferedFrame);
            bufferedFrame = std::nullopt;
            return frame;
        }

        JxlReadResult result = readFrame();
        if (result.status == JxlReadStatus::Frame) {
            return std::move(result.frame);
        }
        if (result.status == JxlReadStatus::Error || result.status == JxlReadStatus::NotAnimation) {
            setError(errorString,
                result.errorString.isEmpty() ? jxlAnimationDecodeErrorString()
                                             : result.errorString);
        }
        return std::nullopt;
    }

    void reset()
    {
        decoder.reset();
        runner.reset();
        data.clear();
        basicInfo = {};
        imageSize = {};
        currentFrameDelay = 0;
        loopCount = 0;
        basicInfoAvailable = false;
        frameBuffer.clear();
        bufferedFrame = std::nullopt;
    }

private:
    bool initializeDecoder()
    {
        decoder = JxlDecoderPtr(JxlDecoderCreate(nullptr));
        if (decoder == nullptr) {
            return false;
        }

        runner = JxlThreadRunnerPtr(JxlThreadParallelRunnerCreate(
            nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads()));
        if (runner == nullptr
            || JxlDecoderSetParallelRunner(decoder.get(), JxlThreadParallelRunner, runner.get())
                != JXL_DEC_SUCCESS) {
            return false;
        }

        const int events = JXL_DEC_BASIC_INFO | JXL_DEC_FRAME | JXL_DEC_FULL_IMAGE;
        if (JxlDecoderSubscribeEvents(decoder.get(), events) != JXL_DEC_SUCCESS) {
            return false;
        }

        if (JxlDecoderSetInput(decoder.get(),
                reinterpret_cast<const std::uint8_t *>(data.constData()),
                static_cast<std::size_t>(data.size()))
            != JXL_DEC_SUCCESS) {
            return false;
        }
        JxlDecoderCloseInput(decoder.get());
        return true;
    }

    JxlReadResult readFrame()
    {
        if (decoder == nullptr) {
            return errorReadResult(jxlAnimationDecodeErrorString());
        }

        while (true) {
            const JxlDecoderStatus status = JxlDecoderProcessInput(decoder.get());
            switch (status) {
            case JXL_DEC_BASIC_INFO:
                if (!readBasicInfo()) {
                    return errorReadResult(jxlAnimationDecodeErrorString());
                }
                if (basicInfo.have_animation != JXL_TRUE) {
                    return JxlReadResult { JxlReadStatus::NotAnimation, {}, {} };
                }
                break;
            case JXL_DEC_FRAME:
                if (!readFrameHeader()) {
                    return errorReadResult(jxlAnimationDecodeErrorString());
                }
                break;
            case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
                if (!setImageOutBuffer()) {
                    return errorReadResult(jxlAnimationDecodeErrorString());
                }
                break;
            case JXL_DEC_FULL_IMAGE:
                if (std::optional<QImage> image = imageFromRgbaBuffer(frameBuffer, imageSize)) {
                    frameBuffer.clear();
                    return JxlReadResult {
                        JxlReadStatus::Frame,
                        AnimationFrame { std::move(*image), currentFrameDelay },
                        {},
                    };
                }
                return errorReadResult(jxlAnimationDecodeErrorString());
            case JXL_DEC_SUCCESS:
                return JxlReadResult { JxlReadStatus::End, {}, {} };
            case JXL_DEC_ERROR:
            case JXL_DEC_NEED_MORE_INPUT:
            default:
                return errorReadResult(jxlAnimationDecodeErrorString());
            }
        }
    }

    bool readBasicInfo()
    {
        if (JxlDecoderGetBasicInfo(decoder.get(), &basicInfo) != JXL_DEC_SUCCESS) {
            return false;
        }
        std::optional<QSize> decodedSize = imageSizeForInfo(basicInfo);
        if (!decodedSize.has_value()) {
            return false;
        }

        imageSize = *decodedSize;
        loopCount = animationLoopCountForPlayCount(basicInfo.animation.num_loops);
        basicInfoAvailable = true;
        return true;
    }

    bool readFrameHeader()
    {
        if (!basicInfoAvailable) {
            return false;
        }

        JxlFrameHeader frameHeader;
        if (JxlDecoderGetFrameHeader(decoder.get(), &frameHeader) != JXL_DEC_SUCCESS) {
            return false;
        }
        currentFrameDelay = jxlFrameDelay(frameHeader.duration, basicInfo.animation.tps_numerator,
            basicInfo.animation.tps_denominator);
        return true;
    }

    bool setImageOutBuffer()
    {
        if (!basicInfoAvailable) {
            return false;
        }

        const JxlPixelFormat format = rgbaPixelFormat();
        std::size_t bufferSize = 0;
        if (JxlDecoderImageOutBufferSize(decoder.get(), &format, &bufferSize) != JXL_DEC_SUCCESS) {
            return false;
        }
        if (bufferSize == 0
            || bufferSize > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
            return false;
        }

        frameBuffer.assign(bufferSize, 0);
        return JxlDecoderSetImageOutBuffer(
                   decoder.get(), &format, frameBuffer.data(), frameBuffer.size())
            == JXL_DEC_SUCCESS;
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

    QByteArray data;
    JxlDecoderPtr decoder;
    JxlThreadRunnerPtr runner;
    JxlBasicInfo basicInfo = {};
    QSize imageSize;
    int currentFrameDelay = 0;
    int loopCount = 0;
    bool basicInfoAvailable = false;
    std::vector<std::uint8_t> frameBuffer;
    std::optional<AnimationFrame> bufferedFrame;
};

JxlAnimationReader::JxlAnimationReader()
    : d(std::make_unique<Private>())
{
}

JxlAnimationReader::~JxlAnimationReader() = default;

JxlAnimationReader::JxlAnimationReader(JxlAnimationReader &&) noexcept = default;

JxlAnimationReader &JxlAnimationReader::operator=(JxlAnimationReader &&) noexcept = default;

JxlAnimationOpenResult JxlAnimationReader::open(QByteArray data)
{
    return d->open(std::move(data));
}

std::optional<AnimationFrame> JxlAnimationReader::readNextFrame(QString *errorString)
{
    return d->readNextFrame(errorString);
}

void JxlAnimationReader::close() { d->reset(); }
}

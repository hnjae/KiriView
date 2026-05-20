// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Compatibility shim: KiriView decodes APNG through APNG-patched libpng because
// QImageReader's normal PNG path does not reliably expose authored APNG
// animations yet. The reader keeps libpng's sequential stream state and only
// stores the current composed canvas instead of all frames.

#include "apnganimationreader.h"

#include "apngframecomposer.h"
#include "imageerrortext.h"
#include "presentation/imageanimationpolicy.h"

#include <png.h>

#include <QtGlobal>
#include <algorithm>
#include <array>
#include <csetjmp>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#ifndef PNG_APNG_SUPPORTED
#error "KiriView APNG playback requires libpng built with PNG_APNG_SUPPORTED"
#endif

#ifndef PNG_READ_APNG_SUPPORTED
#error "KiriView APNG playback requires libpng built with PNG_READ_APNG_SUPPORTED"
#endif

namespace {
constexpr std::array<unsigned char, 8> pngSignature { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a };
constexpr png_uint_16 apngDefaultDelayDenominator = 100;

struct PngErrorContext {
    QString errorString;
};

struct ReadCursor {
    const QByteArray *data = nullptr;
    qsizetype offset = 0;
};

struct FrameControl {
    KiriView::ApngFrameControl composition;
    png_uint_16 delayNum = 1;
    png_uint_16 delayDen = 10;
};

bool isPngData(const QByteArray &data)
{
    return data.size() >= static_cast<qsizetype>(pngSignature.size())
        && std::memcmp(data.constData(), pngSignature.data(), pngSignature.size()) == 0;
}

QString pngDecodeErrorString()
{
    return KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodePngImage);
}

QString apngDecodeErrorString()
{
    return KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeApngAnimation);
}

KiriView::ApngOpenResult notApngResult() { return {}; }

KiriView::ApngOpenResult errorResult(QString errorString)
{
    KiriView::ApngOpenResult result;
    result.status = KiriView::ApngOpenStatus::Error;
    result.errorString = std::move(errorString);
    return result;
}

[[noreturn]] void handlePngError(png_structp png, png_const_charp message)
{
    if (auto *context = static_cast<PngErrorContext *>(png_get_error_ptr(png))) {
        context->errorString = QString::fromLatin1(message != nullptr ? message : "");
    }
    longjmp(png_jmpbuf(png), 1);
}

void handlePngWarning(png_structp, png_const_charp) { }

void readPngBytes(png_structp png, png_bytep output, png_size_t byteCount)
{
    auto *cursor = static_cast<ReadCursor *>(png_get_io_ptr(png));
    if (cursor == nullptr || cursor->data == nullptr || output == nullptr
        || byteCount > static_cast<png_size_t>(std::numeric_limits<qsizetype>::max())) {
        png_error(png, "invalid PNG read request");
    }

    const qsizetype requested = static_cast<qsizetype>(byteCount);
    const qsizetype available = cursor->data->size() - cursor->offset;
    if (cursor->offset < 0 || requested < 0 || available < requested) {
        png_error(png, "unexpected end of PNG data");
    }

    std::memcpy(output, cursor->data->constData() + cursor->offset, byteCount);
    cursor->offset += requested;
}

int loopCountForPlayCount(png_uint_32 playCount)
{
    if (playCount == 0) {
        return -1;
    }
    return static_cast<int>(std::min<png_uint_32>(
        playCount - 1, static_cast<png_uint_32>(std::numeric_limits<int>::max())));
}

int frameDelay(const FrameControl &control)
{
    const png_uint_16 denominator
        = control.delayDen == 0 ? apngDefaultDelayDenominator : control.delayDen;
    const std::uint64_t delay
        = (static_cast<std::uint64_t>(control.delayNum) * 1000U) / denominator;
    return static_cast<int>(
        std::min(delay, static_cast<std::uint64_t>(std::numeric_limits<int>::max())));
}

KiriView::ApngFrameDisposeOp disposeOpFromPng(png_byte disposeOp)
{
    switch (disposeOp) {
    case PNG_DISPOSE_OP_BACKGROUND:
        return KiriView::ApngFrameDisposeOp::Background;
    case PNG_DISPOSE_OP_PREVIOUS:
        return KiriView::ApngFrameDisposeOp::Previous;
    case PNG_DISPOSE_OP_NONE:
    default:
        return KiriView::ApngFrameDisposeOp::None;
    }
}

KiriView::ApngFrameBlendOp blendOpFromPng(png_byte blendOp)
{
    return blendOp == PNG_BLEND_OP_OVER ? KiriView::ApngFrameBlendOp::Over
                                        : KiriView::ApngFrameBlendOp::Source;
}
}

namespace KiriView {
class ApngAnimationReader::Private
{
public:
    ~Private() { close(); }

    ApngOpenResult open(QByteArray inputData)
    {
        close();
        if (!isPngData(inputData)) {
            return notApngResult();
        }

        data = std::move(inputData);
        errorContext.errorString.clear();
        png = png_create_read_struct(
            PNG_LIBPNG_VER_STRING, &errorContext, handlePngError, handlePngWarning);
        if (png == nullptr) {
            close();
            return errorResult(pngDecodeErrorString());
        }

        info = png_create_info_struct(png);
        if (info == nullptr) {
            close();
            return errorResult(pngDecodeErrorString());
        }

        if (setjmp(png_jmpbuf(png))) {
            const QString errorString
                = readingApng ? apngDecodeErrorString() : pngDecodeErrorString();
            close();
            return errorResult(errorString);
        }

        cursor = ReadCursor { &data, 0 };
        png_set_read_fn(png, &cursor, readPngBytes);
        png_read_info(png, info);
        if (!png_get_valid(png, info, PNG_INFO_acTL)) {
            close();
            return notApngResult();
        }

        readingApng = true;
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
        png_set_progressive_frame_fn(png, nullptr, nullptr);
#endif

        png_uint_32 playCount = 0;
        if (png_get_acTL(png, info, &totalFrameCount, &playCount) == 0 || totalFrameCount == 0) {
            close();
            return errorResult(apngDecodeErrorString());
        }

        firstFrameHidden = png_get_first_frame_is_hidden(png, info) != 0;
        if (displayFrameCount() <= 0) {
            close();
            return errorResult(apngDecodeErrorString());
        }

        png_set_expand(png);
        png_set_strip_16(png);
        png_set_gray_to_rgb(png);
        png_set_add_alpha(png, 0xff, PNG_FILLER_AFTER);
        png_read_update_info(png, info);

        if (!initializeImageBuffers()) {
            close();
            return errorResult(apngDecodeErrorString());
        }

        QString frameError;
        if (firstFrameHidden) {
            readFrame(false, &frameError);
            if (!frameError.isEmpty()) {
                close();
                return errorResult(frameError);
            }
        }

        std::optional<AnimationFrame> firstFrame = readFrame(true, &frameError);
        if (!firstFrame.has_value()) {
            close();
            return errorResult(frameError.isEmpty() ? apngDecodeErrorString() : frameError);
        }

        ApngOpenResult result;
        result.status = ApngOpenStatus::Success;
        result.firstFrame = std::move(firstFrame->image);
        result.firstFrameDelay = firstFrame->delay;
        result.loopCount = loopCountForPlayCount(playCount);
        result.frameCount = displayFrameCount();
        return result;
    }

    std::optional<AnimationFrame> readNextFrame(QString *errorString)
    {
        clearError(errorString);
        if (png == nullptr || info == nullptr || rawFramesRead >= totalFrameCount) {
            return std::nullopt;
        }

        if (setjmp(png_jmpbuf(png))) {
            setError(errorString, apngDecodeErrorString());
            close();
            return std::nullopt;
        }

        return readFrame(true, errorString);
    }

    bool hasMoreFrames() const { return png != nullptr && rawFramesRead < totalFrameCount; }

    void close()
    {
        if (png != nullptr || info != nullptr) {
            png_destroy_read_struct(&png, &info, nullptr);
        }

        data.clear();
        cursor = {};
        errorContext.errorString.clear();
        readingApng = false;
        firstFrameHidden = false;
        canvasWidth = 0;
        canvasHeight = 0;
        totalFrameCount = 0;
        rawFramesRead = 0;
        composer.clear();
    }

private:
    int displayFrameCount() const
    {
        if (firstFrameHidden && totalFrameCount > 0) {
            return static_cast<int>(std::min<png_uint_32>(
                totalFrameCount - 1, static_cast<png_uint_32>(std::numeric_limits<int>::max())));
        }
        return static_cast<int>(std::min<png_uint_32>(
            totalFrameCount, static_cast<png_uint_32>(std::numeric_limits<int>::max())));
    }

    bool initializeImageBuffers()
    {
        canvasWidth = png_get_image_width(png, info);
        canvasHeight = png_get_image_height(png, info);
        if (canvasWidth == 0 || canvasHeight == 0
            || canvasWidth > static_cast<png_uint_32>(std::numeric_limits<int>::max())
            || canvasHeight > static_cast<png_uint_32>(std::numeric_limits<int>::max())) {
            return false;
        }

        if (png_get_bit_depth(png, info) != 8 || png_get_channels(png, info) != 4) {
            return false;
        }

        return composer.initialize(
            QSize(static_cast<int>(canvasWidth), static_cast<int>(canvasHeight)),
            png_get_rowbytes(png, info));
    }

    std::optional<AnimationFrame> readFrame(bool displayFrame, QString *errorString)
    {
        if (rawFramesRead >= totalFrameCount) {
            return std::nullopt;
        }

        std::optional<FrameControl> control = nextFrameControl(displayFrame);
        if (!control.has_value()) {
            setError(errorString, apngDecodeErrorString());
            return std::nullopt;
        }

        png_read_image(png, composer.frameRows());
        ++rawFramesRead;

        if (!displayFrame) {
            return std::nullopt;
        }

        std::optional<QImage> composedFrame = composer.composeFrame(control->composition);
        if (!composedFrame.has_value()) {
            setError(errorString, apngDecodeErrorString());
            return std::nullopt;
        }

        return AnimationFrame { std::move(*composedFrame), frameDelay(*control) };
    }

    std::optional<FrameControl> nextFrameControl(bool displayFrame)
    {
        png_read_frame_head(png, info);

        FrameControl control;
        control.composition.width = canvasWidth;
        control.composition.height = canvasHeight;

        if (png_get_valid(png, info, PNG_INFO_fcTL)) {
            png_byte disposeOp = PNG_DISPOSE_OP_NONE;
            png_byte blendOp = PNG_BLEND_OP_SOURCE;
            if (png_get_next_frame_fcTL(png, info, &control.composition.width,
                    &control.composition.height, &control.composition.xOffset,
                    &control.composition.yOffset, &control.delayNum, &control.delayDen, &disposeOp,
                    &blendOp)
                == 0) {
                return std::nullopt;
            }
            control.composition.disposeOp = disposeOpFromPng(disposeOp);
            control.composition.blendOp = blendOpFromPng(blendOp);
        } else if (displayFrame) {
            return std::nullopt;
        }

        if (!composer.canComposeFrame(control.composition)) {
            return std::nullopt;
        }
        return control;
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
    ReadCursor cursor;
    PngErrorContext errorContext;
    png_structp png = nullptr;
    png_infop info = nullptr;
    bool readingApng = false;
    bool firstFrameHidden = false;
    png_uint_32 canvasWidth = 0;
    png_uint_32 canvasHeight = 0;
    png_uint_32 totalFrameCount = 0;
    png_uint_32 rawFramesRead = 0;
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

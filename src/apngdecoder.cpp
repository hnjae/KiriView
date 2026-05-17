// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

// Compatibility shim: KiriView decodes APNG through APNG-patched libpng because
// QImageReader's normal PNG path does not reliably expose authored APNG
// animations yet. The reader keeps libpng's sequential stream state and only
// stores the current composed canvas instead of all frames.

#include "apngdecoder.h"

#include "imageanimationpolicy.h"
#include "imageviewtext.h"

#include <png.h>

#include <QColorSpace>
#include <QtGlobal>
#include <algorithm>
#include <array>
#include <csetjmp>
#include <cstddef>
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
constexpr std::size_t rgbaBytesPerPixel = 4;
constexpr png_uint_16 apngDefaultDelayDenominator = 100;

struct PngErrorContext {
    QString errorString;
};

struct ReadCursor {
    const QByteArray *data = nullptr;
    qsizetype offset = 0;
};

struct FrameControl {
    png_uint_32 width = 0;
    png_uint_32 height = 0;
    png_uint_32 xOffset = 0;
    png_uint_32 yOffset = 0;
    png_uint_16 delayNum = 1;
    png_uint_16 delayDen = 10;
    png_byte disposeOp = PNG_DISPOSE_OP_NONE;
    png_byte blendOp = PNG_BLEND_OP_SOURCE;
};

bool isPngData(const QByteArray &data)
{
    return data.size() >= static_cast<qsizetype>(pngSignature.size())
        && std::memcmp(data.constData(), pngSignature.data(), pngSignature.size()) == 0;
}

QString pngDecodeErrorString()
{
    return KiriView::imageViewText("Could not decode the selected PNG image.");
}

QString apngDecodeErrorString()
{
    return KiriView::imageViewText("Could not decode the selected APNG animation.");
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

std::optional<std::size_t> frameRowBytes(png_uint_32 width)
{
    return checkedMul(static_cast<std::size_t>(width), rgbaBytesPerPixel);
}

bool validateFrameBounds(
    const FrameControl &control, png_uint_32 canvasWidth, png_uint_32 canvasHeight)
{
    const auto right = static_cast<std::uint64_t>(control.xOffset) + control.width;
    const auto bottom = static_cast<std::uint64_t>(control.yOffset) + control.height;
    return control.width > 0 && control.height > 0 && right <= canvasWidth
        && bottom <= canvasHeight;
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
class ApngAnimationReader::Private
{
public:
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

    int frameCount() const { return displayFrameCount(); }

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
        rowBytes = 0;
        totalFrameCount = 0;
        rawFramesRead = 0;
        displayedFramesRead = 0;
        canvas.clear();
        frame.clear();
        frameRows.clear();
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

        rowBytes = png_get_rowbytes(png, info);
        const std::optional<std::size_t> minimumRowBytes = frameRowBytes(canvasWidth);
        if (!minimumRowBytes.has_value() || rowBytes < *minimumRowBytes) {
            return false;
        }

        const std::optional<std::size_t> bufferSize
            = checkedMul(rowBytes, static_cast<std::size_t>(canvasHeight));
        if (!bufferSize.has_value()
            || *bufferSize > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
            return false;
        }

        canvas.assign(*bufferSize, 0);
        frame.assign(*bufferSize, 0);
        frameRows.resize(static_cast<std::size_t>(canvasHeight));
        for (std::size_t y = 0; y < frameRows.size(); ++y) {
            frameRows[y] = frame.data() + y * rowBytes;
        }
        return true;
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

        png_read_image(png, frameRows.data());
        ++rawFramesRead;

        if (!displayFrame) {
            return std::nullopt;
        }

        const bool firstDisplayedFrame = displayedFramesRead == 0;
        if (firstDisplayedFrame) {
            control->blendOp = PNG_BLEND_OP_SOURCE;
            if (control->disposeOp == PNG_DISPOSE_OP_PREVIOUS) {
                control->disposeOp = PNG_DISPOSE_OP_BACKGROUND;
            }
        }

        premultiplyFrame(*control);
        std::optional<std::vector<unsigned char>> previous
            = control->disposeOp == PNG_DISPOSE_OP_PREVIOUS ? copyRegion(*control) : std::nullopt;
        if (control->disposeOp == PNG_DISPOSE_OP_PREVIOUS && !previous.has_value()) {
            setError(errorString, apngDecodeErrorString());
            return std::nullopt;
        }

        blendFrame(*control);
        std::optional<QImage> composedFrame = canvasImage();
        if (!composedFrame.has_value()) {
            setError(errorString, apngDecodeErrorString());
            return std::nullopt;
        }

        applyDispose(*control, previous);
        ++displayedFramesRead;
        return AnimationFrame { std::move(*composedFrame), frameDelay(*control) };
    }

    std::optional<FrameControl> nextFrameControl(bool displayFrame)
    {
        png_read_frame_head(png, info);

        FrameControl control {
            canvasWidth,
            canvasHeight,
            0,
            0,
            1,
            10,
            PNG_DISPOSE_OP_NONE,
            PNG_BLEND_OP_SOURCE,
        };

        if (png_get_valid(png, info, PNG_INFO_fcTL)) {
            if (png_get_next_frame_fcTL(png, info, &control.width, &control.height,
                    &control.xOffset, &control.yOffset, &control.delayNum, &control.delayDen,
                    &control.disposeOp, &control.blendOp)
                == 0) {
                return std::nullopt;
            }
        } else if (displayFrame) {
            return std::nullopt;
        }

        if (!validateFrameBounds(control, canvasWidth, canvasHeight)) {
            return std::nullopt;
        }
        return control;
    }

    std::optional<std::size_t> rowOffset(png_uint_32 x, png_uint_32 y) const
    {
        const std::optional<std::size_t> rowStart
            = checkedMul(static_cast<std::size_t>(y), rowBytes);
        const std::optional<std::size_t> xOffset
            = checkedMul(static_cast<std::size_t>(x), rgbaBytesPerPixel);
        if (!rowStart.has_value() || !xOffset.has_value()) {
            return std::nullopt;
        }
        return checkedAdd(*rowStart, *xOffset);
    }

    void premultiplyFrame(const FrameControl &control)
    {
        const std::size_t width = static_cast<std::size_t>(control.width);
        for (png_uint_32 y = 0; y < control.height; ++y) {
            premultiplyRgbaRow(frame.data() + y * rowBytes, width);
        }
    }

    std::optional<std::vector<unsigned char>> copyRegion(const FrameControl &control) const
    {
        const std::optional<std::size_t> rowLength = frameRowBytes(control.width);
        const std::optional<std::size_t> byteLength = rowLength.has_value()
            ? checkedMul(*rowLength, static_cast<std::size_t>(control.height))
            : std::nullopt;
        if (!rowLength.has_value() || !byteLength.has_value()) {
            return std::nullopt;
        }

        std::vector<unsigned char> region(*byteLength);
        for (png_uint_32 y = 0; y < control.height; ++y) {
            const std::optional<std::size_t> sourceOffset
                = rowOffset(control.xOffset, control.yOffset + y);
            if (!sourceOffset.has_value()) {
                return std::nullopt;
            }
            std::memcpy(region.data() + static_cast<std::size_t>(y) * *rowLength,
                canvas.data() + *sourceOffset, *rowLength);
        }
        return region;
    }

    void blendFrame(const FrameControl &control)
    {
        const std::size_t width = static_cast<std::size_t>(control.width);
        const std::size_t rowLength = width * rgbaBytesPerPixel;
        for (png_uint_32 y = 0; y < control.height; ++y) {
            unsigned char *destination
                = canvas.data() + *rowOffset(control.xOffset, control.yOffset + y);
            const unsigned char *source = frame.data() + static_cast<std::size_t>(y) * rowBytes;
            if (control.blendOp == PNG_BLEND_OP_OVER) {
                for (std::size_t x = 0; x < width; ++x) {
                    blendPixelOver(
                        destination + x * rgbaBytesPerPixel, source + x * rgbaBytesPerPixel);
                }
            } else {
                std::memcpy(destination, source, rowLength);
            }
        }
    }

    std::optional<QImage> canvasImage() const
    {
        if (canvasWidth > static_cast<png_uint_32>(std::numeric_limits<int>::max())
            || canvasHeight > static_cast<png_uint_32>(std::numeric_limits<int>::max())
            || rowBytes > static_cast<std::size_t>(std::numeric_limits<qsizetype>::max())) {
            return std::nullopt;
        }

        const QImage borrowedImage(canvas.data(), static_cast<int>(canvasWidth),
            static_cast<int>(canvasHeight), static_cast<qsizetype>(rowBytes),
            QImage::Format_RGBA8888_Premultiplied);
        if (borrowedImage.isNull()) {
            return std::nullopt;
        }

        QImage image = borrowedImage.copy();
        image.setColorSpace(QColorSpace(QColorSpace::SRgb));
        return image;
    }

    void applyDispose(
        const FrameControl &control, const std::optional<std::vector<unsigned char>> &previous)
    {
        const std::size_t rowLength = *frameRowBytes(control.width);
        switch (control.disposeOp) {
        case PNG_DISPOSE_OP_BACKGROUND:
            for (png_uint_32 y = 0; y < control.height; ++y) {
                std::memset(
                    canvas.data() + *rowOffset(control.xOffset, control.yOffset + y), 0, rowLength);
            }
            break;
        case PNG_DISPOSE_OP_PREVIOUS:
            if (previous.has_value()) {
                for (png_uint_32 y = 0; y < control.height; ++y) {
                    std::memcpy(canvas.data() + *rowOffset(control.xOffset, control.yOffset + y),
                        previous->data() + static_cast<std::size_t>(y) * rowLength, rowLength);
                }
            }
            break;
        case PNG_DISPOSE_OP_NONE:
        default:
            break;
        }
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
    std::size_t rowBytes = 0;
    png_uint_32 totalFrameCount = 0;
    png_uint_32 rawFramesRead = 0;
    png_uint_32 displayedFramesRead = 0;
    std::vector<unsigned char> canvas;
    std::vector<unsigned char> frame;
    std::vector<png_bytep> frameRows;
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

int ApngAnimationReader::frameCount() const { return d->frameCount(); }

void ApngAnimationReader::close() { d->close(); }
}

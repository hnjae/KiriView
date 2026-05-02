// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DISPLAYEDIMAGESTATE_H
#define KIRIVIEW_DISPLAYEDIMAGESTATE_H

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <functional>
#include <memory>
#include <vector>

class QObject;

namespace KiriView {
struct AnimationFrame;
class ImageAnimationPlayer;

class DisplayedImageState
{
public:
    using ImageChangedCallback = std::function<void(const QSize &)>;
    using AnimationErrorCallback = std::function<void(const QString &)>;

    DisplayedImageState(
        QObject *context, ImageChangedCallback imageChanged, AnimationErrorCallback animationError);
    ~DisplayedImageState();

    bool hasImage() const;
    const QImage &image() const;
    QSize imageSize() const;
    quint64 revision() const;
    bool isPredecodeCacheable() const;

    void setPredecodeCacheable(bool cacheable);
    void setImage(const QImage &image);
    void setSvgImage(
        QByteArray data, const QSize &intrinsicSize, const QImage &image, const QSize &rasterSize);
    bool updateSvgRaster(const QSizeF &displaySize, qreal devicePixelRatio, int maximumTextureSize);
    void clear();

    void startAnimation(
        const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay);
    void startDecodedAnimation(std::vector<AnimationFrame> frames, int loopCount);
    void stopAnimation();

private:
    void clearSvgImage();
    void notifyImageChanged();

    ImageChangedCallback m_imageChanged;
    AnimationErrorCallback m_animationError;
    QImage m_image;
    bool m_imageIsSvg = false;
    bool m_imageIsPredecodeCacheable = false;
    QByteArray m_svgData;
    QSize m_svgIntrinsicSize;
    QSize m_svgRasterSize;
    quint64 m_imageRevision = 0;
    std::unique_ptr<ImageAnimationPlayer> m_animationPlayer;
};
}

#endif

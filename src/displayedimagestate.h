// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DISPLAYEDIMAGESTATE_H
#define KIRIVIEW_DISPLAYEDIMAGESTATE_H

#include "animationframe.h"
#include "imagesurface.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QObject;

namespace KiriView {
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
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    const QImage &image() const;
    QSize imageSize() const;
    quint64 revision() const;
    bool isPredecodeCacheable() const;
    std::optional<StaticImagePayload> staticImage() const;

    void setPredecodeCacheable(bool cacheable);
    void setImage(const QImage &image);
    void setStaticImage(StaticImagePayload staticImage, bool useFullImageSurface);
    bool insertTile(DecodedTile tile);
    void clear();

    void startAnimation(
        const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay);
    void startApngAnimation(const QByteArray &data, int loopCount, int firstFrameDelay);
    void startDecodedAnimation(std::vector<AnimationFrame> frames, int loopCount);
    void startHeifSequenceAnimation(const QByteArray &data);
    void stopAnimation();

private:
    void replaceDisplayedImage(QImage image, std::shared_ptr<DisplayedImageSurface> surface,
        std::optional<StaticImagePayload> staticImage);
    void finishImageChange();
    void notifyImageChanged();

    ImageChangedCallback m_imageChanged;
    AnimationErrorCallback m_animationError;
    std::shared_ptr<DisplayedImageSurface> m_surface;
    QImage m_image;
    std::optional<StaticImagePayload> m_staticImage;
    bool m_imageIsPredecodeCacheable = false;
    quint64 m_imageRevision = 0;
    std::unique_ptr<ImageAnimationPlayer> m_animationPlayer;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DISPLAYEDIMAGESTATE_H
#define KIRIVIEW_DISPLAYEDIMAGESTATE_H

#include "imagesurface.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <functional>
#include <memory>
#include <optional>

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
    QSize imageSize() const;
    quint64 revision() const;
    bool isPredecodeCacheable() const;
    std::optional<StaticImagePayload> staticImage() const;

    void setImage(const QImage &image, bool predecodeCacheable);
    void setStaticImage(
        StaticImagePayload staticImage, bool useFullImageSurface, bool predecodeCacheable);
    bool insertTile(DecodedTile tile);
    void clear();

    void startAnimation(const QByteArray &data, const QByteArray &format);
    void startApngAnimation(const QByteArray &data);
    void startHeifSequenceAnimation(const QByteArray &data);
    void stopAnimation();

private:
    void replaceDisplayedImage(std::shared_ptr<DisplayedImageSurface> surface,
        std::optional<StaticImagePayload> staticImage, bool predecodeCacheable);
    void finishImageChange();
    void notifyImageChanged();

    ImageChangedCallback m_imageChanged;
    AnimationErrorCallback m_animationError;
    std::shared_ptr<DisplayedImageSurface> m_surface;
    std::optional<StaticImagePayload> m_staticImage;
    bool m_imageIsPredecodeCacheable = false;
    quint64 m_imageRevision = 0;
    std::unique_ptr<ImageAnimationPlayer> m_animationPlayer;
};
}

#endif

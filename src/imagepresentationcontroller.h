// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONCONTROLLER_H
#define KIRIVIEW_IMAGEPRESENTATIONCONTROLLER_H

#include "animationframe.h"
#include "imageasyncticket.h"
#include "imagedocumenttypes.h"
#include "imagesurface.h"
#include "imagezoomstate.h"

#include <QByteArray>
#include <QImage>
#include <QRectF>
#include <QSet>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QObject;

namespace KiriView {
class DisplayedImageState;

class ImagePresentationController final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using AnimationErrorCallback = std::function<void(const QString &)>;

    struct Callbacks {
        ChangeCallback change;
        AnimationErrorCallback animationError;
    };

    ImagePresentationController(
        QObject *context, RenderContextProvider renderContextProvider, Callbacks callbacks);
    ~ImagePresentationController();

    QSize imageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
    QSizeF displaySize() const;
    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    const QImage &image() const;
    quint64 imageRevision() const;
    bool hasImage() const;
    bool isPredecodeCacheable() const;
    std::optional<StaticImagePayload> staticImage() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;

    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    void updateRenderContext();
    void prepareImageContainer(const QUrl &containerUrl);
    void prepareFailedContainer(const QUrl &containerUrl);
    void setPredecodeCacheable(bool cacheable);
    void setImage(const QImage &image);
    void setStaticImage(StaticImagePayload staticImage);
    void clearImage();

    void startAnimation(
        const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay);
    void startDecodedAnimation(std::vector<AnimationFrame> frames, int loopCount);
    void startHeifSequenceAnimation(const QByteArray &data, int firstFrameDelay);
    void stopAnimation();

private:
    struct TileDecodeLifetime;

    using ZoomStateMutation = std::function<bool(ImageZoomState &, qreal devicePixelRatio)>;

    void setImageSize(const QSize &imageSize);
    void invalidateTiles();
    void scheduleVisibleTileDecode(const ImageDocumentRenderContext &context);
    bool staticTileSurfaceFirstDisplayIsSufficient(
        const StaticTileSurface &surface, const ImageDocumentRenderContext &context) const;
    bool tileRequestIsCurrent(quint64 generation, const TileKey &key) const;
    void finishTileDecode(quint64 generation, TileKey key, std::optional<DecodedTile> tile);
    bool mutateZoomState(const ZoomStateMutation &mutation);
    void applyZoomStateChanges(
        const ImageZoomSnapshot &previous, const ImageDocumentRenderContext &context);
    ImageDocumentRenderContext renderContext() const;
    void notify(ImageDocumentChange change);

    RenderContextProvider m_renderContextProvider;
    Callbacks m_callbacks;
    ImageZoomState m_zoomState;
    std::unique_ptr<DisplayedImageState> m_displayedImageState;
    std::shared_ptr<TileDecodeLifetime> m_tileDecodeLifetime;
    QObject *m_context = nullptr;
    QRectF m_visibleItemRect;
    ImageAsyncTicket m_tileGeneration;
    QSet<TileKey> m_pendingTileKeys;
    QSet<TileKey> m_failedTileKeys;
};
}

#endif

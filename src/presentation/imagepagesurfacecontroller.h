// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPAGESURFACECONTROLLER_H
#define KIRIVIEW_IMAGEPAGESURFACECONTROLLER_H

#include "cache/imagecachepolicy.h"
#include "document/imagedocumenttypes.h"
#include "presentation/imageanimationplaybacksource.h"
#include "presentation/imagepresentationruntime.h"
#include "rendering/displayimagestore.h"
#include "rendering/imagerendercontext.h"
#include "rendering/imagesurface.h"

#include <QImage>
#include <QSize>
#include <QString>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class DisplayedImageSurfaceState;
struct DisplayedImageSurfaceStateChange;
class ImageAnimationPlayer;
class ImageTileDecodeScheduler;

class ImagePageSurfaceController final
{
public:
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using AnimationErrorCallback = std::function<void(const QString &)>;

    struct Callbacks {
        ChangeCallback change;
        AnimationErrorCallback animationError;
    };

    ImagePageSurfaceController(QObject *context, Callbacks callbacks,
        ImageCacheBudgets cacheBudgets, std::shared_ptr<DisplayImageStore> displayImageStore = {},
        DisplayedPageRole pageRole = DisplayedPageRole::Primary);
    ~ImagePageSurfaceController();

    QSize imageSize() const;
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    quint64 imageRevision() const;
    bool hasImage() const;
    bool isPredecodeCacheable() const;
    qsizetype predecodeCacheByteBudget() const;
    std::optional<StaticImagePayload> staticImage() const;
    std::optional<StaticDisplayImagePayload> displayImage() const;
    ImagePresentationPageSlotSnapshot snapshot() const;

    void setImage(const QImage &image, bool predecodeCacheable);
    void setStaticDisplayImage(StaticDisplayImagePayload displayImage, bool predecodeCacheable,
        const ImageDocumentRenderContext &renderContext);
    void setStaticImage(StaticImagePayload staticImage, bool predecodeCacheable,
        const ImageDocumentRenderContext &renderContext);
    QString publishShadowDisplayImage(StaticDisplayImagePayload displayImage);
    void clearShadowDisplayImage();
    void discardDecodedTiles();
    void scheduleVisibleTileDecode(const ImagePresentationRenderProjection &projection);
    void clearImage();

    void startAnimation(ImageAnimationPlaybackRequest request);
    void stopAnimation();

private:
    void applyDisplayedImageChange(const DisplayedImageSurfaceStateChange &change);
    void applyDisplayedImageTileChange(const DisplayedImageSurfaceStateChange &change);
    void publishDisplaySource(const StaticDisplayImagePayload &displayImage);
    void clearDisplaySource();
    void releaseCurrentDisplayEntry();
    void releaseShadowDisplayEntry();
    void updateDisplaySourceVisibility(bool visible);
    void notify(ImageDocumentChange change);

    Callbacks m_callbacks;
    qsizetype m_predecodeCacheByteBudget = 0;
    qsizetype m_staticTileCacheByteBudget = 0;
    std::shared_ptr<DisplayImageStore> m_displayImageStore;
    DisplayedPageRole m_pageRole = DisplayedPageRole::Primary;
    QString m_displayEntryId;
    QString m_shadowDisplayEntryId;
    bool m_displayEntryVisiblePinned = false;
    ImageDisplaySourceSlot m_displaySource;
    quint64 m_displaySourceRevision = 0;
    std::unique_ptr<DisplayedImageSurfaceState> m_displayedSurfaceState;
    std::unique_ptr<ImageTileDecodeScheduler> m_tileDecodeScheduler;
    std::unique_ptr<ImageAnimationPlayer> m_animationPlayer;
};
}

#endif

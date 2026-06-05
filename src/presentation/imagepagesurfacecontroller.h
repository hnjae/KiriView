// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPAGESURFACECONTROLLER_H
#define KIRIVIEW_IMAGEPAGESURFACECONTROLLER_H

#include "async/imageasyncticket.h"
#include "cache/imagecachepolicy.h"
#include "document/imagedocumenttypes.h"
#include "presentation/imageanimationplaybacksource.h"
#include "presentation/imagepresentationruntime.h"
#include "rendering/displayimagestore.h"
#include "rendering/imagerendercontext.h"
#include "rendering/imagesurface.h"
#include "rendering/rasterdisplaybucketpolicy.h"

#include <QImage>
#include <QSize>
#include <QString>
#include <QUrl>
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
    void setAnimationFrame(const QImage &image, const QString &sourceIdentity);
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
    void acknowledgeStillImageDisplayLoad(const QUrl &providerUrl, quint64 revision,
        const QString &sourceIdentity, ImageDisplayLoadOutcome outcome);
    void acknowledgeAnimationFrameDisplayLoad(const QUrl &providerUrl, quint64 revision,
        const QString &sourceIdentity, ImageDisplayLoadOutcome outcome);

private:
    void applyDisplayedImageChange(const DisplayedImageSurfaceStateChange &change);
    void applyDisplayedImageTileChange(const DisplayedImageSurfaceStateChange &change);
    void publishDisplaySource(const StaticDisplayImagePayload &displayImage);
    void publishAnimationFrameDisplaySource(const QImage &image, const QString &sourceIdentity);
    void clearDisplaySource();
    void releaseCurrentDisplayEntry();
    void releaseShadowDisplayEntry();
    void retainCurrentAnimationFrameEntryForLoad();
    void releaseRetainedAnimationFrameEntry();
    void clearStillImageLoadContract();
    void clearAnimationFrameLoadContract();
    void updateDisplaySourceVisibility(bool visible);
    void cancelRasterDisplayRefinement();
    void scheduleRasterDisplayRefinement(const ImagePresentationRenderProjection &projection);
    void notify(ImageDocumentChange change);

    Callbacks m_callbacks;
    QObject *m_context = nullptr;
    qsizetype m_predecodeCacheByteBudget = 0;
    qsizetype m_staticTileCacheByteBudget = 0;
    std::shared_ptr<DisplayImageStore> m_displayImageStore;
    DisplayedPageRole m_pageRole = DisplayedPageRole::Primary;
    QString m_displayEntryId;
    QString m_shadowDisplayEntryId;
    QString m_pendingStillImageEntryId;
    QString m_retainedAnimationFrameEntryId;
    QString m_animationFrameSourceIdentity;
    QUrl m_pendingStillImageProviderUrl;
    quint64 m_pendingStillImageRevision = 0;
    QString m_pendingStillImageSourceIdentity;
    QUrl m_pendingAnimationFrameProviderUrl;
    quint64 m_pendingAnimationFrameRevision = 0;
    QString m_pendingAnimationFrameSourceIdentity;
    bool m_displayEntryVisiblePinned = false;
    bool m_currentDisplayEntryIsAnimationFrame = false;
    bool m_stillImageDisplayLoadPending = false;
    bool m_animationFrameDisplayLoadPending = false;
    ImageDisplaySourceSlot m_displaySource;
    quint64 m_displaySourceRevision = 0;
    std::optional<RasterDisplayRefinementDemandKey> m_rasterDisplayRefinementDemand;
    ImageAsyncTicket m_rasterDisplayRefinementTicket;
    std::unique_ptr<DisplayedImageSurfaceState> m_displayedSurfaceState;
    std::unique_ptr<ImageTileDecodeScheduler> m_tileDecodeScheduler;
    std::unique_ptr<ImageAnimationPlayer> m_animationPlayer;
};
}

#endif

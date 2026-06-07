// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPAGESURFACECONTROLLER_H
#define KIRIVIEW_IMAGEPAGESURFACECONTROLLER_H

#include "async/imageasyncticket.h"
#include "async/imageworkerscheduler.h"
#include "cache/imagecachepolicy.h"
#include "document/imagedocumenttypes.h"
#include "presentation/imageanimationplaybacksource.h"
#include "presentation/imagepresentationruntime.h"
#include "rendering/displayimagestore.h"
#include "rendering/imagerendercontext.h"
#include "rendering/rasterdisplaybucketpolicy.h"
#include "rendering/staticimage.h"

#include <QImage>
#include <QSize>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class ImageAnimationPlayer;

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
        DisplayedPageRole pageRole = DisplayedPageRole::Primary,
        ImageWorkerScheduler workerScheduler = {});
    ~ImagePageSurfaceController();

    QSize imageSize() const;
    quint64 imageRevision() const;
    bool hasImage() const;
    bool isPredecodeCacheable() const;
    qsizetype predecodeCacheByteBudget() const;
    std::optional<StaticDisplayImagePayload> displayImage() const;
    ImagePresentationPageSlotSnapshot snapshot() const;

    void setImage(const QImage &image, bool predecodeCacheable);
    void setAnimationFrame(const QImage &image, const QString &sourceIdentity);
    void setStaticDisplayImage(StaticDisplayImagePayload displayImage, bool predecodeCacheable,
        const ImageDocumentRenderContext &renderContext);
    QString publishShadowDisplayImage(StaticDisplayImagePayload displayImage);
    void clearShadowDisplayImage();
    void updateDisplayProjection(const ImagePresentationRenderProjection &projection);
    void clearImage();

    void startAnimation(ImageAnimationPlaybackRequest request);
    void stopAnimation();
    bool acknowledgeDisplayImageLoad(const QUrl &providerUrl, quint64 revision,
        const QString &sourceIdentity, ImageDisplayLoadOutcome outcome);
    bool acknowledgeStillImageDisplayLoad(const QUrl &providerUrl, quint64 revision,
        const QString &sourceIdentity, ImageDisplayLoadOutcome outcome);
    bool acknowledgeAnimationFrameDisplayLoad(const QUrl &providerUrl, quint64 revision,
        const QString &sourceIdentity, ImageDisplayLoadOutcome outcome);

private:
    void acceptImageState(QSize imageSize, bool predecodeCacheable,
        std::optional<StaticDisplayImagePayload> displayImage);
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
    qsizetype m_displayImageByteBudget = 0;
    std::shared_ptr<DisplayImageStore> m_displayImageStore;
    DisplayedPageRole m_pageRole = DisplayedPageRole::Primary;
    ImageWorkerScheduler m_workerScheduler;
    QSize m_imageSize;
    quint64 m_imageRevision = 0;
    bool m_hasImage = false;
    bool m_predecodeCacheable = false;
    std::optional<StaticDisplayImagePayload> m_displayImage;
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
    std::unique_ptr<ImageAnimationPlayer> m_animationPlayer;
};
}

#endif

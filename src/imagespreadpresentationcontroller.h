// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADPRESENTATIONCONTROLLER_H
#define KIRIVIEW_IMAGESPREADPRESENTATIONCONTROLLER_H

#include "imageasyncdependencies.h"
#include "imagedocumentstate.h"
#include "imagenavigationtypes.h"
#include "imagesurface.h"
#include "imagezoomstate.h"
#include "predecodedimage.h"

#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QObject;

namespace KiriView {
class ImagePresentationController;
class ImageSecondaryPageController;
class ImageSpreadModeController;
class ImageSpreadZoomController;
enum class ImageSecondaryPageLoadResult;

struct ImageSpreadPageNavigationTarget {
    bool handledBySpread = false;
    int pageNumber = 0;
};

class ImageSpreadPresentationController final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using TakePredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl &)>;
    using CurrentPageNumberProvider = std::function<int()>;
    using ImageCountProvider = std::function<int()>;
    using PageUrlProvider = std::function<std::optional<QUrl>(int)>;

    struct Callbacks {
        ChangeCallback change;
        TakePredecodedImageCallback takePredecodedImage;
        CurrentPageNumberProvider currentPageNumber;
        ImageCountProvider imageCount;
        PageUrlProvider urlAtPage;
    };

    ImageSpreadPresentationController(QObject *parent, RenderContextProvider renderContextProvider,
        ImageDocumentState &state, ImagePresentationController &primaryPresentation,
        Callbacks callbacks, ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies);
    ~ImageSpreadPresentationController();

    bool transitionInProgress() const;
    ImageDocumentStatus status(ImageDocumentStatus documentStatus) const;
    bool loading(bool documentLoading) const;

    QSize imageSize() const;
    QSize secondaryImageSize() const;
    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;
    int currentLastPageNumber() const;
    ImageSpreadPageNavigationTarget imageNavigationTarget(NavigationDirection direction) const;
    int relativePageNavigationTarget(int offset) const;

    bool twoPageModeEnabled() const;
    void setTwoPageModeEnabled(bool enabled);
    bool twoPageModeAvailable() const;
    bool twoPageModeActive() const;
    bool rightToLeftReadingEnabled() const;
    void setRightToLeftReadingEnabled(bool enabled);
    bool rightToLeftReadingAvailable() const;
    bool rightToLeftReadingActive() const;
    bool secondaryPageVisible() const;
    std::shared_ptr<DisplayedImageSurface> secondaryImageSurface() const;
    quint64 secondaryImageRevision() const;

    void setViewportSize(const QSizeF &viewportSize);
    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    void updateRenderContext();
    void refreshSecondaryPage();
    bool shouldBeginTransition(int targetPageNumber) const;
    void beginTransition();
    void finishTransition();
    void clearSecondaryPage();
    void shutdown();

    bool shouldResetRightToLeftReadingForLoad(
        const ArchiveDocumentLocation &displayedArchiveDocument, const QUrl &sourceUrl,
        const QUrl &containerNavigationUrl) const;
    void resetRightToLeftReading();
    void notifyRightToLeftReadingChanged();

private:
    void startSecondaryPageLoad(const QUrl &url);
    void handleSecondaryPageLoadFinished(ImageSecondaryPageLoadResult result,
        const DisplayedImageLocation &location, const QSize &imageSize);
    void finishSecondaryPageAsPrimaryOnly();
    void finishSecondaryPageVisible();
    void notifyTransitionChanged();
    void updateZoomState();
    QSize spreadImageSize() const;
    bool primaryPageIsWide() const;
    std::optional<bool> cachedPageIsWide(const QUrl &url) const;
    int currentPageNumber() const;
    int imageCount() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;
    void notifyTwoPageModeChanged();
    void notifySpreadZoomChanged();
    void notifyChanges(const std::vector<ImageDocumentChange> &changes);
    void notify(ImageDocumentChange change);

    ImageDocumentState &m_state;
    ImagePresentationController &m_primaryPresentation;
    Callbacks m_callbacks;
    std::unique_ptr<ImageSecondaryPageController> m_secondaryPageController;
    std::unique_ptr<ImageSpreadModeController> m_modeController;
    std::unique_ptr<ImageSpreadZoomController> m_zoomController;
};
}

#endif

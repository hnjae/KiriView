// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEDOCUMENT_H
#define KIRIVIEW_KIRIIMAGEDOCUMENT_H

#include "document/imagedocumentruntimedependencies.h"
#include "document/imagedocumenttypes.h"
#include "facade/kiriimagedisplaysource.h"
#include "metadata/embeddedmetadata.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "predecode/predecodedimage.h"
#include "presentation/imagepresentationstate.h"
#include "presentation/imageviewportcommandstate.h"
#include "rendering/imagerendercontext.h"

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace kiriview {
class OpenedCollectionScopeLocation;
class ImageDocumentRuntime;
}

class KiriDocumentSession;

class KiriImageDocument : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(KiriImageDocument)
    QML_UNCREATABLE("KiriImageDocument is owned by KiriDocumentSession")

    Q_PROPERTY(QUrl sourceUrl READ sourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(
        QString windowTitleFileName READ windowTitleFileName NOTIFY windowTitleFileNameChanged)
    Q_PROPERTY(QUrl displayedUrl READ displayedUrl NOTIFY displayedUrlChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(QSize primaryImageSize READ primaryImageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(QSize secondaryImageSize READ secondaryImageSize NOTIFY twoPageModeChanged)
    Q_PROPERTY(
        QSizeF viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
    Q_PROPERTY(
        QPointF viewportContentPosition READ viewportContentPosition NOTIFY viewportFrameChanged)
    Q_PROPERTY(QString viewportCommandRevisionToken READ viewportCommandRevisionToken NOTIFY
            viewportFrameChanged)
    Q_PROPERTY(QString viewportObservationRevisionToken READ viewportObservationRevisionToken NOTIFY
            viewportFrameChanged)
    Q_PROPERTY(int viewportCommandStatus READ viewportCommandStatus NOTIFY viewportFrameChanged)
    Q_PROPERTY(ViewportObservationOrigin viewportObservationOrigin READ viewportObservationOrigin
            NOTIFY viewportFrameChanged)
    Q_PROPERTY(QSizeF viewportContentSize READ viewportContentSize NOTIFY viewportFrameChanged)
    Q_PROPERTY(QRectF viewportImageRect READ viewportImageRect NOTIFY viewportFrameChanged)
    Q_PROPERTY(bool viewportHorizontallyPannable READ viewportHorizontallyPannable NOTIFY
            viewportFrameChanged)
    Q_PROPERTY(
        bool viewportVerticallyPannable READ viewportVerticallyPannable NOTIFY viewportFrameChanged)
    Q_PROPERTY(bool viewportPannable READ viewportPannable NOTIFY viewportFrameChanged)
    Q_PROPERTY(QRectF visibleItemRect READ visibleItemRect NOTIFY visibleItemRectChanged)
    Q_PROPERTY(QSizeF displaySize READ displaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(double displayWidth READ displayWidth NOTIFY displaySizeChanged)
    Q_PROPERTY(double displayHeight READ displayHeight NOTIFY displaySizeChanged)
    Q_PROPERTY(QSizeF primaryDisplaySize READ primaryDisplaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(double primaryDisplayWidth READ primaryDisplayWidth NOTIFY displaySizeChanged)
    Q_PROPERTY(double primaryDisplayHeight READ primaryDisplayHeight NOTIFY displaySizeChanged)
    Q_PROPERTY(QSizeF secondaryDisplaySize READ secondaryDisplaySize NOTIFY twoPageModeChanged)
    Q_PROPERTY(double secondaryDisplayWidth READ secondaryDisplayWidth NOTIFY twoPageModeChanged)
    Q_PROPERTY(double secondaryDisplayHeight READ secondaryDisplayHeight NOTIFY twoPageModeChanged)
    Q_PROPERTY(bool zoomPercentKnown READ zoomPercentKnown NOTIFY zoomPercentKnownChanged)
    Q_PROPERTY(double zoomPercent READ zoomPercent NOTIFY zoomPercentChanged)
    Q_PROPERTY(ZoomMode zoomMode READ zoomMode NOTIFY zoomModeChanged)
    Q_PROPERTY(ZoomMode fitModeSelection READ fitModeSelection NOTIFY zoomModeChanged)
    Q_PROPERTY(int rotationDegrees READ rotationDegrees NOTIFY rotationDegreesChanged)
    Q_PROPERTY(int minimumManualZoomPercent READ minimumManualZoomPercent CONSTANT)
    Q_PROPERTY(int maximumManualZoomPercent READ maximumManualZoomPercent NOTIFY
            maximumManualZoomPercentChanged)
    Q_PROPERTY(double zoomStepFactor READ zoomStepFactor CONSTANT)
    Q_PROPERTY(QStringList openDialogNameFilters READ openDialogNameFilters CONSTANT)
    Q_PROPERTY(int currentPageNumber READ currentPageNumber NOTIFY pageNavigationChanged)
    Q_PROPERTY(int currentLastPageNumber READ currentLastPageNumber NOTIFY pageNavigationChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageNavigationChanged)
    Q_PROPERTY(bool containerNavigationAvailable READ containerNavigationAvailable NOTIFY
            containerNavigationChanged)
    Q_PROPERTY(bool ordinaryDirectMediaScopeActive READ ordinaryDirectMediaScopeActive NOTIFY
            imageDocumentSourceScopeChanged)
    Q_PROPERTY(bool openedCollectionScopeActive READ openedCollectionScopeActive NOTIFY
            imageDocumentSourceScopeChanged)
    Q_PROPERTY(bool fileDeletionInProgress READ fileDeletionInProgress NOTIFY
            fileDeletionInProgressChanged)
    Q_PROPERTY(bool twoPageModeEnabled READ twoPageModeEnabled NOTIFY twoPageModeChanged)
    Q_PROPERTY(bool twoPageModeAvailable READ twoPageModeAvailable NOTIFY twoPageModeChanged)
    Q_PROPERTY(bool rightToLeftReadingEnabled READ rightToLeftReadingEnabled NOTIFY
            rightToLeftReadingChanged)
    Q_PROPERTY(bool rightToLeftReadingAvailable READ rightToLeftReadingAvailable NOTIFY
            rightToLeftReadingChanged)
    Q_PROPERTY(bool secondaryPageVisible READ secondaryPageVisible NOTIFY twoPageModeChanged)
    Q_PROPERTY(PresentationTransitionState presentationTransitionState READ
            presentationTransitionState NOTIFY presentationTransitionStateChanged)
    Q_PROPERTY(bool unsupportedOpenedCollectionVideo READ unsupportedOpenedCollectionVideo NOTIFY
            unsupportedOpenedCollectionVideoChanged)
    Q_PROPERTY(KiriImageDisplaySource* primaryDisplaySource READ primaryDisplaySource CONSTANT)
    Q_PROPERTY(KiriImageDisplaySource* secondaryDisplaySource READ secondaryDisplaySource CONSTANT)

public:
    using RenderContextProvider = std::function<kiriview::ImageDocumentRenderContext()>;

    enum class Status {
        Null,
        Loading,
        Ready,
        Error,
    };
    Q_ENUM(Status)

    enum class ZoomMode {
        Fit,
        FitHeight,
        FitWidth,
        Manual,
    };
    Q_ENUM(ZoomMode)

    enum class DeletionMode {
        MoveToTrash,
        DeletePermanently,
    };
    Q_ENUM(DeletionMode)

    enum class ViewportObservationOrigin {
        Command,
        User,
        Inertia,
        Overshoot,
        Resize,
        Rotation,
        DevicePixelRatio,
        System,
    };
    Q_ENUM(ViewportObservationOrigin)

    enum class ViewportCommandStatus {
        Pending,
        Applying,
        Applied,
        Acknowledged,
        Settled,
        Superseded,
        Rejected,
    };
    Q_ENUM(ViewportCommandStatus)

    enum class PresentationTransitionState {
        PreviousActive,
        TransitioningPlaceholder,
        CommittedActive,
    };
    Q_ENUM(PresentationTransitionState)

    explicit KiriImageDocument(QObject* parent = nullptr);
    explicit KiriImageDocument(
        kiriview::ImageDocumentRuntimeDependencyOverrides dependencies, QObject* parent = nullptr);
    ~KiriImageDocument() override;

    QUrl sourceUrl() const;

    Status status() const;
    bool loading() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    QUrl displayedUrl() const;
    kiriview::OpenedCollectionScopeLocation displayedOpenedCollectionScope() const;
    QSize imageSize() const;
    QSize primaryImageSize() const;
    QSize secondaryImageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF& viewportSize);
    QPointF viewportContentPosition() const;
    quint64 viewportCommandRevision() const;
    QString viewportCommandRevisionToken() const;
    quint64 viewportAppliedCommandRevision() const;
    quint64 viewportObservationRevision() const;
    QString viewportObservationRevisionToken() const;
    int viewportCommandStatus() const;
    ViewportObservationOrigin viewportObservationOrigin() const;
    QSizeF viewportContentSize() const;
    QRectF viewportImageRect() const;
    bool viewportHorizontallyPannable() const;
    bool viewportVerticallyPannable() const;
    bool viewportPannable() const;
    QRectF visibleItemRect() const;
    QSizeF displaySize() const;
    double displayWidth() const;
    double displayHeight() const;
    QSizeF primaryDisplaySize() const;
    double primaryDisplayWidth() const;
    double primaryDisplayHeight() const;
    QSizeF secondaryDisplaySize() const;
    double secondaryDisplayWidth() const;
    double secondaryDisplayHeight() const;
    bool zoomPercentKnown() const;
    double zoomPercent() const;
    ZoomMode zoomMode() const;
    ZoomMode fitModeSelection() const;
    int rotationDegrees() const;
    int minimumManualZoomPercent() const;
    int maximumManualZoomPercent() const;
    double zoomStepFactor() const;
    QStringList openDialogNameFilters() const;
    int currentPageNumber() const;
    int currentLastPageNumber() const;
    int pageCount() const;
    kiriview::ImageDocumentPageNavigationSnapshot pageNavigationSnapshot() const;
    kiriview::ImageDocumentPageActiveNavigationSnapshot activeNavigationSnapshot() const;
    bool containerNavigationAvailable() const;
    bool ordinaryDirectMediaScopeActive() const;
    bool openedCollectionScopeActive() const;
    bool fileDeletionInProgress() const;
    bool twoPageModeEnabled() const;
    bool twoPageModeAvailable() const;
    bool rightToLeftReadingEnabled() const;
    bool rightToLeftReadingAvailable() const;
    bool secondaryPageVisible() const;
    PresentationTransitionState presentationTransitionState() const;
    bool unsupportedOpenedCollectionVideo() const;
    KiriImageDisplaySource* primaryDisplaySource() const;
    KiriImageDisplaySource* secondaryDisplaySource() const;
    std::optional<kiriview::DisplayedPredecodeImage> primaryDisplayedPredecodeImage() const;
    kiriview::ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;
    const kiriview::EmbeddedMetadata& embeddedMetadata() const;

    void setRenderContextProvider(RenderContextProvider provider);

    Q_INVOKABLE void openPreviousPage();
    Q_INVOKABLE void openNextPage();
    Q_INVOKABLE void openPreviousSinglePage();
    Q_INVOKABLE void openNextSinglePage();
    Q_INVOKABLE void openImageAtPage(int pageNumber);
    Q_INVOKABLE void openPreviousContainer();
    Q_INVOKABLE void openNextContainer();
    Q_INVOKABLE void deleteDisplayedFile(KiriImageDocument::DeletionMode mode);
    Q_INVOKABLE void resetZoom();
    Q_INVOKABLE void setFitMode(KiriImageDocument::ZoomMode zoomMode);
    Q_INVOKABLE void rotateClockwise();
    Q_INVOKABLE void rotateCounterclockwise();
    Q_INVOKABLE double clampedManualZoomPercent(double zoomPercent) const;
    Q_INVOKABLE double steppedManualZoomPercent(double stepCount) const;
    Q_INVOKABLE bool requestManualZoomPercent(double zoomPercent);
    Q_INVOKABLE bool requestZoomByStep(double stepCount, const QPointF& viewportAnchorPoint);
    Q_INVOKABLE bool requestZoomByStepAtCenter(double stepCount);
    Q_INVOKABLE bool requestActualSizeAtCenter();
    Q_INVOKABLE bool requestFitMode(KiriImageDocument::ZoomMode zoomMode);
    Q_INVOKABLE bool requestToggleFitOrActualSize(const QPointF& viewportPoint);
    Q_INVOKABLE bool requestViewportPanBy(double deltaX, double deltaY);
    Q_INVOKABLE bool requestViewportPanToInitialScanPosition();
    Q_INVOKABLE bool requestViewportPanToFinalScanPosition();
    Q_INVOKABLE bool requestViewportScanForward();
    Q_INVOKABLE bool requestViewportScanBackward();
    Q_INVOKABLE void requestNextDisplayedImageStartToFinalScanPosition();
    Q_INVOKABLE bool requestDisplayedImageInitialContentPosition();
    Q_INVOKABLE bool viewportPointInsideImage(const QPointF& viewportPoint) const;
    Q_INVOKABLE QPointF nearestImageViewportPoint(const QPointF& viewportPoint) const;
    Q_INVOKABLE void requestToggleTwoPageMode();
    Q_INVOKABLE void requestToggleRightToLeftReading();
    Q_INVOKABLE void updateRenderContext();
    Q_INVOKABLE bool requestViewportContentPosition(const QPointF& viewportContentPosition);
    Q_INVOKABLE bool viewportCommandRevisionNewerThan(const QString& revisionToken) const;
    Q_INVOKABLE bool viewportProjectionNewerThan(
        const QString& commandRevisionToken, const QString& observationRevisionToken) const;
    bool beginViewportCommandApplication(quint64 commandRevision);
    Q_INVOKABLE bool beginViewportCommandApplication(const QString& commandRevisionToken);
    bool completeViewportCommandApplication(
        quint64 commandRevision, const QPointF& actualContentPosition);
    Q_INVOKABLE bool completeViewportCommandApplication(
        const QString& commandRevisionToken, const QPointF& actualContentPosition);
    bool acknowledgeViewportCommand(quint64 commandRevision, const QPointF& actualContentPosition);
    Q_INVOKABLE bool acknowledgeViewportCommand(
        const QString& commandRevisionToken, const QPointF& actualContentPosition);
    Q_INVOKABLE bool observeViewportContentPosition(
        const QPointF& contentPosition, KiriImageDocument::ViewportObservationOrigin origin);
    Q_INVOKABLE bool acknowledgeDisplayImageLoad(int pageRole, const QUrl& providerUrl,
        const QString& revisionToken, const QString& sourceIdentity, int outcome);
    Q_INVOKABLE bool acknowledgeStillImageDisplayLoad(int pageRole, const QUrl& providerUrl,
        const QString& revisionToken, const QString& sourceIdentity, int outcome);

Q_SIGNALS:
    void sourceUrlChanged();
    void statusChanged();
    void loadingChanged();
    void errorStringChanged();
    void windowTitleFileNameChanged();
    void displayedUrlChanged();
    void imageSizeChanged();
    void viewportSizeChanged();
    void viewportFrameChanged();
    void visibleItemRectChanged();
    void displaySizeChanged();
    void zoomPercentKnownChanged();
    void zoomPercentChanged();
    void zoomModeChanged();
    void rotationDegreesChanged();
    void maximumManualZoomPercentChanged();
    void pageNavigationChanged();
    void containerNavigationChanged();
    void imageDocumentSourceScopeChanged();
    void fileDeletionInProgressChanged();
    void twoPageModeChanged();
    void rightToLeftReadingChanged();
    void presentationTransitionStateChanged();
    void fileDeletionFailed(const QString& errorString);
    void unsupportedOpenedCollectionVideoChanged();
    void embeddedMetadataChanged();
    void displaySourceChanged();
    void unsupportedOpenedCollectionVideoEntered(const QString& message);
    void containerNavigationBoundaryReached(const QString& message);

private:
    friend class KiriDocumentSession;

    void setSourceUrl(const QUrl& sourceUrl);
    void setTwoPageModeEnabled(bool enabled);
    void setRightToLeftReadingEnabled(bool enabled);
    void handleDocumentChanges(const std::vector<kiriview::ImageDocumentChange>& changes);
    void refreshDisplaySources();

    std::unique_ptr<kiriview::ImageDocumentRuntime> m_runtime;
    std::unique_ptr<KiriImageDisplaySource> m_primaryDisplaySource;
    std::unique_ptr<KiriImageDisplaySource> m_secondaryDisplaySource;
};

#endif

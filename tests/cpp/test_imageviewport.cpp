// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportframe.h"
#include "presentation/imageviewportgeometry.h"

#include "qml_component_test_support.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QPoint>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QVariant>
#include <QWheelEvent>
#include <QtQml/qqml.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace {
double binaryOctaveZoomStepFactor() { return std::pow(2.0, 1.0 / 8.0); }
}

class TestImageViewport : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ctrlWheelZoomsWhileImageIsPannable();
    void ctrlWheelZoomsWhileImageIsPanned();
    void ctrlWheelZoomsFromViewportMarginAroundNearestImagePoint();
    void rightButtonWheelZoomsWhileImageIsPannable();
    void rightButtonWheelZoomsFromViewportMarginAroundNearestImagePoint();
    void plainWheelStillPansWhileImageIsPannable();
    void fittedFractionalDisplayDoesNotEnableHorizontalPanning();
    void doubleClickTogglesFitToActualSize();
    void doubleClickTogglesManualZoomToFit();
    void doubleClickTogglesFitHeightToFit();
    void doubleClickFromViewportMarginZoomsAroundNearestImagePoint();
    void viewportUsesDisplayImagePagesWithContextBridge();
    void displayImagePageLoadAcknowledgmentReachesDocument();
    void viewportHitTestingUsesDocumentFacade();
    void singleClickStillEmitsViewerClicked();
};

namespace {
class FakeKiriImageDisplaySource : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY changed)
    Q_PROPERTY(int pageRole READ pageRole CONSTANT)
    Q_PROPERTY(QUrl providerUrl READ providerUrl WRITE setProviderUrl NOTIFY changed)
    Q_PROPERTY(QString revisionToken READ revisionToken WRITE setRevisionToken NOTIFY changed)
    Q_PROPERTY(QString sourceIdentity READ sourceIdentity WRITE setSourceIdentity NOTIFY changed)
    Q_PROPERTY(QSize sourceSizeHint READ sourceSizeHint WRITE setSourceSizeHint NOTIFY changed)
    Q_PROPERTY(bool cacheEnabled READ cacheEnabled WRITE setCacheEnabled NOTIFY changed)
    Q_PROPERTY(bool loadAcknowledgmentRequired READ loadAcknowledgmentRequired WRITE
            setLoadAcknowledgmentRequired NOTIFY changed)
    Q_PROPERTY(int rotationDegrees READ rotationDegrees WRITE setRotationDegrees NOTIFY changed)
    Q_PROPERTY(bool retainWhileLoadingEligible READ retainWhileLoadingEligible WRITE
            setRetainWhileLoadingEligible NOTIFY changed)

public:
    explicit FakeKiriImageDisplaySource(int pageRole, QObject* parent = nullptr)
        : QObject(parent)
        , m_pageRole(pageRole)
    {
    }

    bool visible() const { return m_visible; }
    int pageRole() const { return m_pageRole; }
    QUrl providerUrl() const { return m_providerUrl; }
    QString revisionToken() const { return m_revisionToken; }
    QString sourceIdentity() const { return m_sourceIdentity; }
    QSize sourceSizeHint() const { return m_sourceSizeHint; }
    bool cacheEnabled() const { return m_cacheEnabled; }
    bool loadAcknowledgmentRequired() const { return m_loadAcknowledgmentRequired; }
    int rotationDegrees() const { return m_rotationDegrees; }
    bool retainWhileLoadingEligible() const { return m_retainWhileLoadingEligible; }

    void setVisible(bool visible)
    {
        if (m_visible == visible) {
            return;
        }

        m_visible = visible;
        Q_EMIT changed();
    }

    void setProviderUrl(const QUrl& providerUrl)
    {
        if (m_providerUrl == providerUrl) {
            return;
        }

        m_providerUrl = providerUrl;
        Q_EMIT changed();
    }

    void setRevisionToken(const QString& revisionToken)
    {
        if (m_revisionToken == revisionToken) {
            return;
        }

        m_revisionToken = revisionToken;
        Q_EMIT changed();
    }

    void setSourceIdentity(const QString& sourceIdentity)
    {
        if (m_sourceIdentity == sourceIdentity) {
            return;
        }

        m_sourceIdentity = sourceIdentity;
        Q_EMIT changed();
    }

    void setSourceSizeHint(const QSize& sourceSizeHint)
    {
        if (m_sourceSizeHint == sourceSizeHint) {
            return;
        }

        m_sourceSizeHint = sourceSizeHint;
        Q_EMIT changed();
    }

    void setCacheEnabled(bool cacheEnabled)
    {
        if (m_cacheEnabled == cacheEnabled) {
            return;
        }

        m_cacheEnabled = cacheEnabled;
        Q_EMIT changed();
    }

    void setLoadAcknowledgmentRequired(bool loadAcknowledgmentRequired)
    {
        if (m_loadAcknowledgmentRequired == loadAcknowledgmentRequired) {
            return;
        }

        m_loadAcknowledgmentRequired = loadAcknowledgmentRequired;
        Q_EMIT changed();
    }

    void setRotationDegrees(int rotationDegrees)
    {
        if (m_rotationDegrees == rotationDegrees) {
            return;
        }

        m_rotationDegrees = rotationDegrees;
        Q_EMIT changed();
    }

    void setRetainWhileLoadingEligible(bool retainWhileLoadingEligible)
    {
        if (m_retainWhileLoadingEligible == retainWhileLoadingEligible) {
            return;
        }

        m_retainWhileLoadingEligible = retainWhileLoadingEligible;
        Q_EMIT changed();
    }

Q_SIGNALS:
    void changed();

private:
    int m_pageRole = 0;
    bool m_visible = true;
    QUrl m_providerUrl;
    QString m_revisionToken = QStringLiteral("1");
    QString m_sourceIdentity = QStringLiteral("source-primary");
    QSize m_sourceSizeHint;
    bool m_cacheEnabled = false;
    bool m_loadAcknowledgmentRequired = false;
    int m_rotationDegrees = 0;
    bool m_retainWhileLoadingEligible = false;
};

class FakeKiriImageDocument : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(
        QSizeF viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
    Q_PROPERTY(QPointF viewportContentPosition READ viewportContentPosition WRITE
            setViewportContentPosition NOTIFY viewportFrameChanged)
    Q_PROPERTY(QString viewportCommandRevisionToken READ viewportCommandRevisionToken NOTIFY
            viewportFrameChanged)
    Q_PROPERTY(QString viewportObservationRevisionToken READ viewportObservationRevisionToken NOTIFY
            viewportFrameChanged)
    Q_PROPERTY(int viewportCommandStatus READ viewportCommandStatus NOTIFY viewportFrameChanged)
    Q_PROPERTY(QSizeF viewportContentSize READ viewportContentSize NOTIFY viewportFrameChanged)
    Q_PROPERTY(QRectF viewportImageRect READ viewportImageRect NOTIFY viewportFrameChanged)
    Q_PROPERTY(bool viewportHorizontallyPannable READ viewportHorizontallyPannable NOTIFY
            viewportFrameChanged)
    Q_PROPERTY(
        bool viewportVerticallyPannable READ viewportVerticallyPannable NOTIFY viewportFrameChanged)
    Q_PROPERTY(bool viewportPannable READ viewportPannable NOTIFY viewportFrameChanged)
    Q_PROPERTY(QRectF visibleItemRect READ visibleItemRect WRITE setVisibleItemRect NOTIFY
            visibleItemRectChanged)
    Q_PROPERTY(QSizeF displaySize READ displaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(double displayWidth READ displayWidth NOTIFY displaySizeChanged)
    Q_PROPERTY(double displayHeight READ displayHeight NOTIFY displaySizeChanged)
    Q_PROPERTY(QSizeF primaryDisplaySize READ primaryDisplaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(double primaryDisplayWidth READ primaryDisplayWidth NOTIFY displaySizeChanged)
    Q_PROPERTY(double primaryDisplayHeight READ primaryDisplayHeight NOTIFY displaySizeChanged)
    Q_PROPERTY(QSizeF secondaryDisplaySize READ secondaryDisplaySize CONSTANT)
    Q_PROPERTY(double secondaryDisplayWidth READ secondaryDisplayWidth CONSTANT)
    Q_PROPERTY(double secondaryDisplayHeight READ secondaryDisplayHeight CONSTANT)
    Q_PROPERTY(double zoomPercent READ zoomPercent WRITE setZoomPercent NOTIFY zoomPercentChanged)
    Q_PROPERTY(ZoomMode zoomMode READ zoomMode NOTIFY zoomModeChanged)
    Q_PROPERTY(int minimumManualZoomPercent READ minimumManualZoomPercent CONSTANT)
    Q_PROPERTY(int maximumManualZoomPercent READ maximumManualZoomPercent CONSTANT)
    Q_PROPERTY(bool secondaryPageVisible READ secondaryPageVisible CONSTANT)
    Q_PROPERTY(bool rightToLeftReadingEnabled READ rightToLeftReadingEnabled CONSTANT)
    Q_PROPERTY(bool rightToLeftReadingAvailable READ rightToLeftReadingAvailable CONSTANT)
    Q_PROPERTY(QObject* primaryDisplaySource READ primaryDisplaySource CONSTANT)
    Q_PROPERTY(QObject* secondaryDisplaySource READ secondaryDisplaySource CONSTANT)

public:
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

    explicit FakeKiriImageDocument(QObject* parent = nullptr)
        : QObject(parent)
        , m_primaryDisplaySource(0, this)
        , m_secondaryDisplaySource(1, this)
    {
        m_secondaryDisplaySource.setVisible(false);
        m_secondaryDisplaySource.setSourceIdentity(QStringLiteral("source-secondary"));
    }

    QUrl sourceUrl() const { return m_sourceUrl; }
    void setSourceUrl(const QUrl& sourceUrl)
    {
        if (m_sourceUrl == sourceUrl) {
            return;
        }

        m_sourceUrl = sourceUrl;
        Q_EMIT sourceUrlChanged();
    }

    Status status() const { return Status::Ready; }

    QSizeF viewportSize() const { return m_viewportSize; }
    void setViewportSize(const QSizeF& viewportSize)
    {
        if (m_viewportSize == viewportSize) {
            return;
        }

        m_viewportSize = viewportSize;
        Q_EMIT viewportSizeChanged();
        Q_EMIT displaySizeChanged();
        Q_EMIT viewportFrameChanged();
    }

    QPointF viewportContentPosition() const { return viewportFrame().contentPosition; }
    void setViewportContentPosition(const QPointF& viewportContentPosition)
    {
        const kiriview::ImageViewportFrame previousFrame = viewportFrame();
        m_viewportContentPosition = viewportContentPosition;
        const kiriview::ImageViewportFrame nextFrame = viewportFrame();
        m_viewportContentPosition = nextFrame.contentPosition;
        if (previousFrame != nextFrame) {
            Q_EMIT viewportFrameChanged();
        }
    }

    QString viewportCommandRevisionToken() const
    {
        return revisionToken(m_viewportCommandRevision);
    }
    QString viewportObservationRevisionToken() const
    {
        return revisionToken(m_viewportObservationRevision);
    }
    int viewportCommandStatus() const { return static_cast<int>(m_viewportCommandStatus); }
    QSizeF viewportContentSize() const { return viewportFrame().contentSize; }
    QRectF viewportImageRect() const { return viewportFrame().imageRect; }
    bool viewportHorizontallyPannable() const { return viewportFrame().horizontalPannable; }
    bool viewportVerticallyPannable() const { return viewportFrame().verticalPannable; }
    bool viewportPannable() const { return viewportFrame().pannable; }

    QRectF visibleItemRect() const { return m_visibleItemRect; }
    void setVisibleItemRect(const QRectF& visibleItemRect)
    {
        if (m_visibleItemRect == visibleItemRect) {
            return;
        }

        m_visibleItemRect = visibleItemRect;
        Q_EMIT visibleItemRectChanged();
    }

    QSizeF displaySize() const
    {
        return kiriview::imageViewportDisplaySizeForZoom(imageSize(), m_zoomPercent, 1.0);
    }

    double displayWidth() const { return displaySize().width(); }
    double displayHeight() const { return displaySize().height(); }
    QSizeF primaryDisplaySize() const { return displaySize(); }
    double primaryDisplayWidth() const { return primaryDisplaySize().width(); }
    double primaryDisplayHeight() const { return primaryDisplaySize().height(); }
    QSizeF secondaryDisplaySize() const { return QSizeF(); }
    double secondaryDisplayWidth() const { return secondaryDisplaySize().width(); }
    double secondaryDisplayHeight() const { return secondaryDisplaySize().height(); }

    double zoomPercent() const { return m_zoomPercent; }
    void setZoomPercent(double zoomPercent)
    {
        const double nextZoomPercent = clampedManualZoomPercent(zoomPercent);
        const bool zoomChanged = std::abs(m_zoomPercent - nextZoomPercent) >= 0.001;
        const bool modeChanged = m_zoomMode != ZoomMode::Manual;
        if (!zoomChanged && !modeChanged) {
            return;
        }

        m_zoomMode = ZoomMode::Manual;
        m_zoomPercent = nextZoomPercent;
        if (modeChanged) {
            Q_EMIT zoomModeChanged();
        }
        if (zoomChanged) {
            Q_EMIT zoomPercentChanged();
            Q_EMIT displaySizeChanged();
        }
        Q_EMIT viewportFrameChanged();
    }

    ZoomMode zoomMode() const { return m_zoomMode; }
    int minimumManualZoomPercent() const { return 10; }
    int maximumManualZoomPercent() const { return 1000; }
    bool secondaryPageVisible() const { return false; }
    bool rightToLeftReadingEnabled() const { return false; }
    bool rightToLeftReadingAvailable() const { return false; }
    QObject* primaryDisplaySource() { return &m_primaryDisplaySource; }
    QObject* secondaryDisplaySource() { return &m_secondaryDisplaySource; }

    Q_INVOKABLE double steppedManualZoomPercent(double stepCount) const
    {
        return std::clamp(m_zoomPercent * std::pow(binaryOctaveZoomStepFactor(), stepCount),
            static_cast<double>(minimumManualZoomPercent()),
            static_cast<double>(maximumManualZoomPercent()));
    }

    Q_INVOKABLE double clampedManualZoomPercent(double zoomPercent) const
    {
        return std::clamp(zoomPercent, static_cast<double>(minimumManualZoomPercent()),
            static_cast<double>(maximumManualZoomPercent()));
    }

    Q_INVOKABLE void setFitMode(ZoomMode zoomMode)
    {
        if (zoomMode == ZoomMode::Manual) {
            return;
        }

        const double nextZoomPercent = fitZoomPercent(zoomMode);
        const bool modeChanged = m_zoomMode != zoomMode;
        const bool zoomChanged = std::abs(m_zoomPercent - nextZoomPercent) >= 0.001;
        if (!modeChanged && !zoomChanged) {
            return;
        }

        m_zoomMode = zoomMode;
        m_zoomPercent = nextZoomPercent;
        if (modeChanged) {
            Q_EMIT zoomModeChanged();
        }
        if (zoomChanged) {
            Q_EMIT zoomPercentChanged();
            Q_EMIT displaySizeChanged();
        }
        Q_EMIT viewportFrameChanged();
    }

    Q_INVOKABLE bool requestManualZoomPercent(double zoomPercent)
    {
        return requestAnchoredManualZoom(clampedManualZoomPercent(zoomPercent),
            QPointF(m_viewportSize.width() / 2.0, m_viewportSize.height() / 2.0));
    }

    Q_INVOKABLE bool requestZoomByStep(double stepCount, const QPointF& viewportAnchorPoint)
    {
        if (!pointIsFinite(viewportAnchorPoint)) {
            return false;
        }

        const QPointF anchorPoint = nearestImageViewportPoint(viewportAnchorPoint);
        if (!pointIsFinite(anchorPoint)) {
            return false;
        }

        return requestAnchoredManualZoom(steppedManualZoomPercent(stepCount), anchorPoint);
    }

    Q_INVOKABLE bool requestFitMode(ZoomMode zoomMode)
    {
        if (zoomMode == ZoomMode::Manual) {
            return false;
        }

        setFitMode(zoomMode);
        return true;
    }

    Q_INVOKABLE bool requestToggleFitOrActualSize(const QPointF& viewportPoint)
    {
        if (!pointIsFinite(viewportPoint)) {
            return false;
        }

        if (m_zoomMode != ZoomMode::Fit) {
            setFitMode(ZoomMode::Fit);
            return true;
        }

        const QPointF anchorPoint = nearestImageViewportPoint(viewportPoint);
        if (!pointIsFinite(anchorPoint)) {
            return false;
        }

        return requestAnchoredManualZoom(clampedManualZoomPercent(100.0), anchorPoint);
    }

    Q_INVOKABLE bool viewportPointInsideImage(const QPointF& viewportPoint) const
    {
        ++m_documentHitTestHelperCallCount;
        return kiriview::imageViewportPointInsideImage(
            viewportContentPosition(), viewportPoint, viewportImageRect());
    }

    Q_INVOKABLE QPointF nearestImageViewportPoint(const QPointF& viewportAnchorPoint) const
    {
        ++m_documentHitTestHelperCallCount;
        return kiriview::imageViewportNearestImagePoint(
            viewportContentPosition(), viewportAnchorPoint, viewportImageRect());
    }

    Q_INVOKABLE void resetHitTestHelperCallCounts() const
    {
        m_documentHitTestHelperCallCount = 0;
        m_legacyViewHitTestHelperCallCount = 0;
    }

    Q_INVOKABLE int documentHitTestHelperCallCount() const
    {
        return m_documentHitTestHelperCallCount;
    }

    Q_INVOKABLE int legacyViewHitTestHelperCallCount() const
    {
        return m_legacyViewHitTestHelperCallCount;
    }

    void noteLegacyViewHitTestHelperCall() const { ++m_legacyViewHitTestHelperCallCount; }

    Q_INVOKABLE bool requestViewportPanBy(double deltaX, double deltaY)
    {
        if (!viewportPannable()) {
            return false;
        }

        return requestViewportInteractionContentPosition(
            kiriview::imageViewportPanPosition(m_viewportSize, viewportImageRect(),
                viewportContentPosition(), QPointF(deltaX, deltaY)));
    }

    Q_INVOKABLE bool requestViewportPanToInitialScanPosition()
    {
        if (!viewportPannable()) {
            return false;
        }

        return requestViewportInteractionContentPosition(
            kiriview::imageViewportInitialZScanPosition(m_viewportSize, viewportImageRect()));
    }

    Q_INVOKABLE bool requestViewportPanToFinalScanPosition()
    {
        if (!viewportPannable()) {
            return false;
        }

        return requestViewportInteractionContentPosition(
            kiriview::imageViewportFinalZScanPosition(m_viewportSize, viewportImageRect()));
    }

    Q_INVOKABLE bool requestViewportScanForward()
    {
        if (!viewportPannable()) {
            return false;
        }

        return requestViewportInteractionContentPosition(kiriview::imageViewportNextZScanPosition(
            m_viewportSize, viewportImageRect(), viewportContentPosition()));
    }

    Q_INVOKABLE bool requestViewportScanBackward()
    {
        if (!viewportPannable()) {
            return false;
        }

        return requestViewportInteractionContentPosition(
            kiriview::imageViewportPreviousZScanPosition(
                m_viewportSize, viewportImageRect(), viewportContentPosition()));
    }

    Q_INVOKABLE void requestNextDisplayedImageStartToFinalScanPosition() { }

    Q_INVOKABLE void requirePrimaryMissingLoadAcknowledgment()
    {
        m_primaryDisplaySource.setProviderUrl(QUrl());
        m_primaryDisplaySource.setRevisionToken(QStringLiteral("42"));
        m_primaryDisplaySource.setSourceIdentity(QStringLiteral("source-primary"));
        m_primaryDisplaySource.setLoadAcknowledgmentRequired(true);
    }

    Q_INVOKABLE void acknowledgeStillImageDisplayLoad(int pageRole, const QUrl& providerUrl,
        const QString& revisionToken, const QString& sourceIdentity, int outcome)
    {
        m_lastStillImageAcknowledgedPageRole = pageRole;
        m_lastStillImageAcknowledgedProviderUrl = providerUrl;
        m_lastStillImageAcknowledgedRevisionToken = revisionToken;
        m_lastStillImageAcknowledgedSourceIdentity = sourceIdentity;
        m_lastStillImageAcknowledgedOutcome = outcome;
        ++m_stillImageLoadAcknowledgmentCount;
    }

    Q_INVOKABLE void acknowledgeDisplayImageLoad(int pageRole, const QUrl& providerUrl,
        const QString& revisionToken, const QString& sourceIdentity, int outcome)
    {
        m_lastAcknowledgedPageRole = pageRole;
        m_lastAcknowledgedProviderUrl = providerUrl;
        m_lastAcknowledgedRevisionToken = revisionToken;
        m_lastAcknowledgedSourceIdentity = sourceIdentity;
        m_lastAcknowledgedOutcome = outcome;
        ++m_displayLoadAcknowledgmentCount;
    }

    Q_INVOKABLE int displayLoadAcknowledgmentCount() const
    {
        return m_displayLoadAcknowledgmentCount;
    }

    Q_INVOKABLE int lastAcknowledgedPageRole() const { return m_lastAcknowledgedPageRole; }

    Q_INVOKABLE QString lastAcknowledgedRevisionToken() const
    {
        return m_lastAcknowledgedRevisionToken;
    }

    Q_INVOKABLE QString lastAcknowledgedSourceIdentity() const
    {
        return m_lastAcknowledgedSourceIdentity;
    }

    Q_INVOKABLE int lastAcknowledgedOutcome() const { return m_lastAcknowledgedOutcome; }

    Q_INVOKABLE bool requestDisplayedImageInitialContentPosition()
    {
        return requestViewportInteractionContentPosition(
            kiriview::imageViewportInitialZScanPosition(m_viewportSize, viewportImageRect()));
    }

    Q_INVOKABLE bool requestViewportContentPosition(const QPointF& viewportContentPosition)
    {
        return issueViewportContentPositionCommand(viewportContentPosition) > 0;
    }

    Q_INVOKABLE bool viewportCommandRevisionNewerThan(const QString& revisionToken) const
    {
        return revisionIsNewerThanToken(m_viewportCommandRevision, revisionToken);
    }

    Q_INVOKABLE bool viewportProjectionNewerThan(
        const QString& commandRevisionToken, const QString& observationRevisionToken) const
    {
        quint64 commandRevision = 0;
        quint64 observationRevision = 0;
        if (!revisionFromToken(commandRevisionToken, &commandRevision)
            || !revisionFromToken(observationRevisionToken, &observationRevision)) {
            return true;
        }

        return m_viewportCommandRevision > commandRevision
            || (m_viewportCommandRevision == commandRevision
                && m_viewportObservationRevision > observationRevision);
    }

    Q_INVOKABLE bool beginViewportCommandApplication(const QString& commandRevisionToken)
    {
        quint64 commandRevision = 0;
        return revisionFromToken(commandRevisionToken, &commandRevision)
            && beginViewportCommandApplication(commandRevision);
    }

    Q_INVOKABLE bool completeViewportCommandApplication(
        const QString& commandRevisionToken, const QPointF& actualContentPosition)
    {
        quint64 commandRevision = 0;
        return revisionFromToken(commandRevisionToken, &commandRevision)
            && completeViewportCommandApplication(commandRevision, actualContentPosition);
    }

    Q_INVOKABLE bool acknowledgeViewportCommand(
        const QString& commandRevisionToken, const QPointF& actualContentPosition)
    {
        quint64 commandRevision = 0;
        return revisionFromToken(commandRevisionToken, &commandRevision)
            && acknowledgeViewportCommand(commandRevision, actualContentPosition);
    }

    Q_INVOKABLE bool observeViewportContentPosition(
        const QPointF& contentPosition, ViewportObservationOrigin)
    {
        ++m_viewportObservationRevision;
        m_viewportCommandStatus = ViewportCommandStatus::Settled;
        setViewportContentPosition(contentPosition);
        return true;
    }

    static QSize imageSize() { return QSize(1000, 1000); }

Q_SIGNALS:
    void sourceUrlChanged();
    void statusChanged();
    void viewportSizeChanged();
    void viewportFrameChanged();
    void visibleItemRectChanged();
    void displaySizeChanged();
    void zoomPercentChanged();
    void zoomModeChanged();

private:
    static bool pointIsFinite(const QPointF& point)
    {
        return std::isfinite(point.x()) && std::isfinite(point.y());
    }

    static QString revisionToken(quint64 revision) { return QString::number(revision); }

    static bool revisionFromToken(const QString& revisionToken, quint64* revision)
    {
        bool ok = false;
        const quint64 parsedRevision = revisionToken.toULongLong(&ok);
        if (!ok) {
            return false;
        }

        *revision = parsedRevision;
        return true;
    }

    static bool revisionIsNewerThanToken(quint64 revision, const QString& revisionToken)
    {
        quint64 tokenRevision = 0;
        return !revisionFromToken(revisionToken, &tokenRevision) || revision > tokenRevision;
    }

    quint64 issueViewportContentPositionCommand(const QPointF& viewportContentPosition)
    {
        const kiriview::ImageViewportFrame previousFrame = viewportFrame();
        m_viewportContentPosition = viewportContentPosition;
        const kiriview::ImageViewportFrame nextFrame = viewportFrame();
        m_viewportContentPosition = nextFrame.contentPosition;
        ++m_viewportCommandRevision;
        m_viewportCommandStatus = ViewportCommandStatus::Pending;
        Q_UNUSED(previousFrame);
        Q_EMIT viewportFrameChanged();
        return m_viewportCommandRevision;
    }

    bool beginViewportCommandApplication(quint64 commandRevision)
    {
        if (!acceptCommandRevision(commandRevision)) {
            return false;
        }

        m_viewportCommandStatus = ViewportCommandStatus::Applying;
        Q_EMIT viewportFrameChanged();
        return true;
    }

    bool completeViewportCommandApplication(
        quint64 commandRevision, const QPointF& actualContentPosition)
    {
        if (!acceptCommandRevision(commandRevision)) {
            return false;
        }

        m_viewportAppliedCommandRevision = commandRevision;
        m_viewportCommandStatus = ViewportCommandStatus::Applied;
        setViewportContentPosition(actualContentPosition);
        Q_EMIT viewportFrameChanged();
        return true;
    }

    bool acknowledgeViewportCommand(quint64 commandRevision, const QPointF& actualContentPosition)
    {
        if (!acceptCommandRevision(commandRevision)) {
            return false;
        }

        m_viewportAppliedCommandRevision = commandRevision;
        ++m_viewportObservationRevision;
        m_viewportCommandStatus = ViewportCommandStatus::Acknowledged;
        setViewportContentPosition(actualContentPosition);
        Q_EMIT viewportFrameChanged();
        return true;
    }

    bool requestViewportInteractionContentPosition(const QPointF& contentPosition)
    {
        if (!pointIsFinite(contentPosition)) {
            return false;
        }

        const QPointF previousContentPosition = viewportContentPosition();
        const quint64 previousCommandRevision = m_viewportCommandRevision;
        const quint64 commandRevision = issueViewportContentPositionCommand(contentPosition);
        return previousContentPosition != viewportContentPosition()
            || commandRevision > previousCommandRevision;
    }

    bool acceptCommandRevision(quint64 commandRevision)
    {
        if (commandRevision == m_viewportCommandRevision) {
            return true;
        }

        m_viewportCommandStatus = commandRevision < m_viewportCommandRevision
            ? ViewportCommandStatus::Superseded
            : ViewportCommandStatus::Rejected;
        Q_EMIT viewportFrameChanged();
        return false;
    }

    bool requestAnchoredManualZoom(double zoomPercent, const QPointF& viewportAnchorPoint)
    {
        if (!pointIsFinite(viewportAnchorPoint)) {
            return false;
        }

        const double nextZoomPercent = clampedManualZoomPercent(zoomPercent);
        if (std::abs(m_zoomPercent - nextZoomPercent) < 0.001) {
            return false;
        }

        const QSizeF nextDisplaySize
            = kiriview::imageViewportDisplaySizeForZoom(imageSize(), nextZoomPercent, 1.0);
        const QPointF nextContentPosition
            = kiriview::imageViewportContentPositionForZoom(m_viewportSize, displaySize(),
                nextDisplaySize, viewportContentPosition(), viewportAnchorPoint);
        setZoomPercent(nextZoomPercent);
        requestViewportInteractionContentPosition(nextContentPosition);
        return true;
    }

    double fitZoomPercent(ZoomMode zoomMode) const
    {
        if (m_viewportSize.isEmpty()) {
            return 100.0;
        }

        switch (zoomMode) {
        case ZoomMode::Fit:
            return std::min(m_viewportSize.width() * 100.0 / imageSize().width(),
                m_viewportSize.height() * 100.0 / imageSize().height());
        case ZoomMode::FitHeight:
            return m_viewportSize.height() * 100.0 / imageSize().height();
        case ZoomMode::FitWidth:
            return m_viewportSize.width() * 100.0 / imageSize().width();
        case ZoomMode::Manual:
            return m_zoomPercent;
        }

        return 100.0;
    }

    kiriview::ImageViewportFrame viewportFrame() const
    {
        return kiriview::projectImageViewportFrame(
            m_viewportSize, displaySize(), m_viewportContentPosition);
    }

    QUrl m_sourceUrl;
    QSizeF m_viewportSize;
    QPointF m_viewportContentPosition;
    quint64 m_viewportCommandRevision = 0;
    quint64 m_viewportAppliedCommandRevision = 0;
    quint64 m_viewportObservationRevision = 0;
    ViewportCommandStatus m_viewportCommandStatus = ViewportCommandStatus::Settled;
    QRectF m_visibleItemRect;
    double m_zoomPercent = 100.0;
    ZoomMode m_zoomMode = ZoomMode::Manual;
    mutable int m_documentHitTestHelperCallCount = 0;
    mutable int m_legacyViewHitTestHelperCallCount = 0;
    FakeKiriImageDisplaySource m_primaryDisplaySource;
    FakeKiriImageDisplaySource m_secondaryDisplaySource;
    int m_stillImageLoadAcknowledgmentCount = 0;
    int m_displayLoadAcknowledgmentCount = 0;
    int m_lastStillImageAcknowledgedPageRole = -1;
    QUrl m_lastStillImageAcknowledgedProviderUrl;
    QString m_lastStillImageAcknowledgedRevisionToken;
    QString m_lastStillImageAcknowledgedSourceIdentity;
    int m_lastStillImageAcknowledgedOutcome = -1;
    int m_lastAcknowledgedPageRole = -1;
    QUrl m_lastAcknowledgedProviderUrl;
    QString m_lastAcknowledgedRevisionToken;
    QString m_lastAcknowledgedSourceIdentity;
    int m_lastAcknowledgedOutcome = -1;
};

class FakeKiriImageViewportContextBridge : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(
        FakeKiriImageDocument* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(
        bool secondaryPage READ secondaryPage WRITE setSecondaryPage NOTIFY secondaryPageChanged)

public:
    explicit FakeKiriImageViewportContextBridge(QQuickItem* parent = nullptr)
        : QQuickItem(parent)
    {
    }

    FakeKiriImageDocument* document() const { return m_document; }
    void setDocument(FakeKiriImageDocument* document)
    {
        if (m_document == document) {
            return;
        }

        m_document = document;
        Q_EMIT documentChanged();
    }

    bool secondaryPage() const { return m_secondaryPage; }
    void setSecondaryPage(bool secondaryPage)
    {
        if (m_secondaryPage == secondaryPage) {
            return;
        }

        m_secondaryPage = secondaryPage;
        Q_EMIT secondaryPageChanged();
    }

Q_SIGNALS:
    void documentChanged();
    void secondaryPageChanged();

private:
    FakeKiriImageDocument* m_document = nullptr;
    bool m_secondaryPage = false;
};

class FakeKiriImageViewportCommandBridge : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(
        FakeKiriImageDocument* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QQuickItem* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool applying READ applying NOTIFY applyingChanged)

public:
    explicit FakeKiriImageViewportCommandBridge(QQuickItem* parent = nullptr)
        : QQuickItem(parent)
    {
    }

    FakeKiriImageDocument* document() const { return m_document; }
    void setDocument(FakeKiriImageDocument* document)
    {
        if (m_document == document) {
            return;
        }

        if (m_document != nullptr) {
            disconnect(m_document, &FakeKiriImageDocument::viewportFrameChanged, this,
                &FakeKiriImageViewportCommandBridge::applyViewportProjection);
        }
        m_document = document;
        if (m_document != nullptr) {
            connect(m_document, &FakeKiriImageDocument::viewportFrameChanged, this,
                &FakeKiriImageViewportCommandBridge::applyViewportProjection);
        }
        Q_EMIT documentChanged();
        applyViewportProjection();
    }

    QQuickItem* target() const { return m_target; }
    void setTarget(QQuickItem* target)
    {
        if (m_target == target) {
            return;
        }

        m_target = target;
        Q_EMIT targetChanged();
        applyViewportProjection();
    }

    bool active() const { return m_active; }
    void setActive(bool active)
    {
        if (m_active == active) {
            return;
        }

        m_active = active;
        Q_EMIT activeChanged();
        applyViewportProjection();
    }

    bool applying() const { return m_applying; }

    Q_INVOKABLE QPointF currentContentPosition() const
    {
        if (m_target == nullptr) {
            return {};
        }

        return QPointF(
            m_target->property("contentX").toReal(), m_target->property("contentY").toReal());
    }

    Q_INVOKABLE bool requestContentPosition(const QPointF& contentPosition)
    {
        if (!ready()) {
            return false;
        }

        const QPointF previousContentPosition = currentContentPosition();
        m_document->requestViewportContentPosition(contentPosition);
        const bool commandAdvanced
            = m_document->viewportCommandRevisionNewerThan(m_appliedCommandRevisionToken);
        return previousContentPosition != currentContentPosition() || commandAdvanced;
    }

    Q_INVOKABLE bool observeViewportContentPosition(
        FakeKiriImageDocument::ViewportObservationOrigin origin)
    {
        if (!ready() || m_applying) {
            return false;
        }

        return m_document->observeViewportContentPosition(currentContentPosition(), origin);
    }

    Q_INVOKABLE void applyViewportProjection()
    {
        if (!ready() || m_applying) {
            return;
        }

        const QString commandRevisionToken = m_document->viewportCommandRevisionToken();
        if (m_document->viewportCommandStatus()
            == static_cast<int>(FakeKiriImageDocument::ViewportCommandStatus::Rejected)) {
            return;
        }
        if (!m_document->viewportProjectionNewerThan(
                m_appliedCommandRevisionToken, m_appliedObservationRevisionToken)) {
            return;
        }

        setApplying(true);
        const bool commandNewer
            = m_document->viewportCommandRevisionNewerThan(m_appliedCommandRevisionToken);
        if (commandNewer) {
            m_document->beginViewportCommandApplication(commandRevisionToken);
        }
        m_target->setProperty("contentX", m_document->viewportContentPosition().x());
        m_target->setProperty("contentY", m_document->viewportContentPosition().y());
        if (commandNewer) {
            const QPointF actualContentPosition = currentContentPosition();
            m_document->completeViewportCommandApplication(
                commandRevisionToken, actualContentPosition);
            m_appliedCommandRevisionToken = commandRevisionToken;
            m_document->acknowledgeViewportCommand(commandRevisionToken, actualContentPosition);
        }
        m_appliedObservationRevisionToken = m_document->viewportObservationRevisionToken();
        setApplying(false);
    }

Q_SIGNALS:
    void documentChanged();
    void targetChanged();
    void activeChanged();
    void applyingChanged();

private:
    bool ready() const { return m_active && m_document != nullptr && m_target != nullptr; }

    void setApplying(bool applying)
    {
        if (m_applying == applying) {
            return;
        }

        m_applying = applying;
        Q_EMIT applyingChanged();
    }

    FakeKiriImageDocument* m_document = nullptr;
    QQuickItem* m_target = nullptr;
    bool m_active = false;
    bool m_applying = false;
    QString m_appliedCommandRevisionToken = QStringLiteral("0");
    QString m_appliedObservationRevisionToken = QStringLiteral("0");
};

struct ImageViewportFixture
{
    std::unique_ptr<QQuickView> view;
    std::unique_ptr<QTemporaryDir> qmlDirectory;
    QObject* root = nullptr;
    QString errorString;

    bool isValid() const { return view != nullptr && root != nullptr; }
};

void addEnvironmentImportPaths(QQmlEngine& engine)
{
    const QString paths = qEnvironmentVariable("NIXPKGS_QML_SEARCH_PATHS");
    for (const QString& path : paths.split(QLatin1Char(':'), Qt::SkipEmptyParts)) {
        engine.addImportPath(path);
    }
}

void registerKiriViewQmlTypes()
{
    static bool registered = false;
    if (registered) {
        return;
    }

    const char* uri = "org.hnjae.kiriview.tests";
    qmlRegisterType<FakeKiriImageDocument>(uri, 1, 0, "KiriImageDocument");
    qmlRegisterType<FakeKiriImageViewportCommandBridge>(
        uri, 1, 0, "KiriImageViewportCommandBridge");
    qmlRegisterType<FakeKiriImageViewportContextBridge>(
        uri, 1, 0, "KiriImageViewportContextBridge");
    registered = true;
}

QString sourceQmlPath(const QString& fileName)
{
    return QDir(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR))
        .absoluteFilePath(QStringLiteral("../../src/qml/%1").arg(fileName));
}

bool writeFile(const QString& path, const QByteArray& content, QString* errorString)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *errorString = QStringLiteral("failed to write %1: %2").arg(path, file.errorString());
        return false;
    }
    if (file.write(content) != content.size()) {
        *errorString
            = QStringLiteral("failed to write complete file %1: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

std::unique_ptr<QTemporaryDir> createTestQmlDirectory(QString* errorString)
{
    auto directory = std::make_unique<QTemporaryDir>();
    if (!directory->isValid()) {
        *errorString = QStringLiteral("temporary QML directory was not created");
        return nullptr;
    }

    const QStringList files {
        QStringLiteral("DisplayImagePage.qml"),
        QStringLiteral("ImageViewport.qml"),
        QStringLiteral("ImageViewportInteractionSurface.qml"),
        QStringLiteral("MediaViewportDelegate.qml"),
        QStringLiteral("ZoomWheelStepPolicy.qml"),
    };
    for (const QString& fileName : files) {
        QFile sourceFile(sourceQmlPath(fileName));
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            *errorString = QStringLiteral("failed to read %1: %2")
                               .arg(sourceFile.fileName(), sourceFile.errorString());
            return nullptr;
        }

        QByteArray content = sourceFile.readAll();
        if (fileName == QStringLiteral("ImageViewport.qml")) {
            const QByteArray productionImport("import org.hnjae.kiriview\n");
            const QByteArray testImport("import org.hnjae.kiriview.tests\n");
            if (!content.contains(productionImport)) {
                *errorString = QStringLiteral("ImageViewport.qml production import was not found");
                return nullptr;
            }
            content.replace(productionImport, testImport);
        }

        const QString destinationPath = QDir(directory->path()).filePath(fileName);
        if (!writeFile(destinationPath, content, errorString)) {
            return nullptr;
        }
    }

    return directory;
}

QString qmlDirectoryImport(const QString& path) { return QUrl::fromLocalFile(path).toString(); }

QString fixtureQml(const QString& qmlImportPath)
{
    return QStringLiteral(R"(
import QtQuick
import QtQuick.Controls as Controls
import QtQml
import org.hnjae.kiriview.tests
import "%1" as KiriViewQml

Item {
    id: root

    width: 200
    height: 160
    property var testImageDocument: imageDocument
    property int viewerClickCount: 0

    function contentY() {
        return imageViewport.flickable.contentY;
    }

    function contentX() {
        return imageViewport.flickable.contentX;
    }

    function documentReady() {
        return imageViewport.imageDocument.status === KiriImageDocument.Ready;
    }

    function imagePannable() {
        return imageViewport.imagePannable;
    }

    function imageHorizontallyPannable() {
        return imageViewport.imageHorizontallyPannable;
    }

    function horizontalContentFitsViewport() {
        return imageViewport.flickable.contentWidth <= imageViewport.flickable.width;
    }

    function setContentY(contentY) {
        imageViewport.flickable.contentY = contentY;
    }

    function setContentX(contentX) {
        imageViewport.flickable.contentX = contentX;
    }

    function setManualZoom(zoomPercent) {
        imageViewport.imageDocument.requestManualZoomPercent(zoomPercent);
    }

    function setFitZoom() {
        imageViewport.imageDocument.requestFitMode(KiriImageDocument.Fit);
    }

    function setFitHeightZoom() {
        imageViewport.imageDocument.requestFitMode(KiriImageDocument.FitHeight);
    }

    function zoomMode() {
        return imageViewport.imageDocument.zoomMode;
    }

    function zoomPercent() {
        return imageViewport.imageDocument.zoomPercent;
    }

    function resetViewerClickCount() {
        root.viewerClickCount = 0;
    }

    function resetHitTestHelperCallCounts() {
        imageViewport.imageDocument.resetHitTestHelperCallCounts();
    }

    function documentHitTestHelperCallCount() {
        return imageViewport.imageDocument.documentHitTestHelperCallCount();
    }

    function legacyViewHitTestHelperCallCount() {
        return imageViewport.imageDocument.legacyViewHitTestHelperCallCount();
    }

    function viewportCenterInsideImage() {
        return imageViewport.viewportPointInsideImage(100, 80);
    }

    function nearestMarginImageViewportPointX() {
        const point = imageViewport.nearestImageViewportPoint(10, 10);
        return point === null ? Number.NaN : point.x;
    }

    function requirePrimaryMissingLoadAcknowledgment() {
        imageViewport.imageDocument.requirePrimaryMissingLoadAcknowledgment();
    }

    function displayLoadAcknowledgmentCount() {
        return imageViewport.imageDocument.displayLoadAcknowledgmentCount();
    }

    function lastAcknowledgedPageRole() {
        return imageViewport.imageDocument.lastAcknowledgedPageRole();
    }

    function lastAcknowledgedRevisionToken() {
        return imageViewport.imageDocument.lastAcknowledgedRevisionToken();
    }

    function lastAcknowledgedSourceIdentity() {
        return imageViewport.imageDocument.lastAcknowledgedSourceIdentity();
    }

    function lastAcknowledgedOutcome() {
        return imageViewport.imageDocument.lastAcknowledgedOutcome();
    }

    KiriImageDocument {
        id: imageDocument
    }

    QtObject {
        id: documentSession

        readonly property bool activeImageReady: root.testImageDocument.status === KiriImageDocument.Ready
        property var imageDocument: root.testImageDocument
        property int scopeKind: 0
    }

    KiriViewQml.ImageViewport {
        id: imageViewport

        anchors.fill: parent
        documentSession: documentSession

        onViewerClicked: root.viewerClickCount += 1
    }
}
)")
        .arg(qmlImportPath);
}

ImageViewportFixture createFixture()
{
    ImageViewportFixture fixture;
    registerKiriViewQmlTypes();
    fixture.qmlDirectory = createTestQmlDirectory(&fixture.errorString);
    if (fixture.qmlDirectory == nullptr) {
        return fixture;
    }

    fixture.view = std::make_unique<QQuickView>();
    fixture.view->resize(200, 160);
    fixture.view->setResizeMode(QQuickView::SizeRootObjectToView);
    addEnvironmentImportPaths(*fixture.view->engine());

    QQmlComponent component(fixture.view->engine());
    component.setData(fixtureQml(qmlDirectoryImport(fixture.qmlDirectory->path())).toUtf8(),
        QUrl(QStringLiteral("memory:test_imageviewport.qml")));
    if (!waitForQmlComponentReady(component)) {
        fixture.errorString = QStringLiteral("QML component did not become ready");
        return fixture;
    }
    if (component.isError()) {
        fixture.errorString = component.errorString();
        return fixture;
    }

    QObject* root = component.create();
    if (root == nullptr) {
        fixture.errorString = component.errorString();
        return fixture;
    }

    fixture.view->setContent(
        QUrl(QStringLiteral("memory:test_imageviewport.qml")), &component, root);
    fixture.view->show();
    if (!QTest::qWaitForWindowExposed(fixture.view.get())) {
        fixture.errorString = QStringLiteral("test window was not exposed");
        return fixture;
    }

    fixture.root = root;
    return fixture;
}

bool invokeBool(QObject* object, const char* method)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        object, method, Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked && result.toBool();
}

qreal invokeReal(QObject* object, const char* method)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        object, method, Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked ? result.toReal() : std::numeric_limits<qreal>::quiet_NaN();
}

int invokeInt(QObject* object, const char* method)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        object, method, Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked ? result.toInt() : std::numeric_limits<int>::min();
}

QString invokeString(QObject* object, const char* method)
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(
        object, method, Qt::DirectConnection, Q_RETURN_ARG(QVariant, result));
    return invoked ? result.toString() : QString();
}

void invokeVoid(QObject* object, const char* method)
{
    QVERIFY(QMetaObject::invokeMethod(object, method, Qt::DirectConnection));
}

void invokeSetReal(QObject* object, const char* method, qreal value)
{
    QVERIFY(QMetaObject::invokeMethod(object, method, Q_ARG(QVariant, QVariant::fromValue(value))));
}

void sendLeftClick(QQuickView* view, const QPointF& position)
{
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, position.toPoint());
    QCoreApplication::processEvents();
}

void sendLeftDoubleClick(QQuickView* view, const QPointF& position)
{
    QTest::mouseDClick(view, Qt::LeftButton, Qt::NoModifier, position.toPoint());
    QCoreApplication::processEvents();
}

void sendWheel(QQuickView* view, const QPointF& position, int angleDeltaY,
    Qt::KeyboardModifiers modifiers = Qt::NoModifier, Qt::MouseButtons buttons = Qt::NoButton)
{
    QWheelEvent event(position, view->mapToGlobal(position.toPoint()), QPoint(),
        QPoint(0, angleDeltaY), buttons, modifiers, Qt::ScrollUpdate, false);
    QCoreApplication::sendEvent(view, &event);
    QCoreApplication::processEvents();
}

void sendRightButtonWheel(QQuickView* view, const QPointF& position, int angleDeltaY)
{
    const QPoint point = position.toPoint();
    QTest::mousePress(view, Qt::RightButton, Qt::NoModifier, point);
    sendWheel(view, position, angleDeltaY, Qt::NoModifier, Qt::RightButton);
    QTest::mouseRelease(view, Qt::RightButton, Qt::NoModifier, point);
    QCoreApplication::processEvents();
}

void preparePannableImage(ImageViewportFixture& fixture)
{
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    invokeSetReal(fixture.root, "setManualZoom", 100.0);
    QTRY_VERIFY(invokeBool(fixture.root, "imagePannable"));
}
}

void TestImageViewport::ctrlWheelZoomsWhileImageIsPannable()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    preparePannableImage(fixture);

    const qreal initialZoomPercent = invokeReal(fixture.root, "zoomPercent");
    sendWheel(fixture.view.get(), QPointF(100.0, 80.0), 120, Qt::ControlModifier);

    QTRY_VERIFY(invokeReal(fixture.root, "zoomPercent") > initialZoomPercent);
}

void TestImageViewport::ctrlWheelZoomsWhileImageIsPanned()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    preparePannableImage(fixture);

    invokeSetReal(fixture.root, "setContentX", 300.0);
    invokeSetReal(fixture.root, "setContentY", 400.0);
    QTRY_VERIFY(std::abs(invokeReal(fixture.root, "contentX") - 300.0) < 0.001);
    QTRY_VERIFY(std::abs(invokeReal(fixture.root, "contentY") - 400.0) < 0.001);

    const qreal initialZoomPercent = invokeReal(fixture.root, "zoomPercent");
    sendWheel(fixture.view.get(), QPointF(100.0, 80.0), 120, Qt::ControlModifier);

    QTRY_VERIFY(invokeReal(fixture.root, "zoomPercent") > initialZoomPercent);
}

void TestImageViewport::ctrlWheelZoomsFromViewportMarginAroundNearestImagePoint()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    invokeSetReal(fixture.root, "setManualZoom", 10.0);
    QTRY_VERIFY(!invokeBool(fixture.root, "imagePannable"));

    const qreal initialZoomPercent = invokeReal(fixture.root, "zoomPercent");
    sendWheel(fixture.view.get(), QPointF(10.0, 10.0), 120, Qt::ControlModifier);

    QTRY_VERIFY(invokeReal(fixture.root, "zoomPercent") > initialZoomPercent);
}

void TestImageViewport::rightButtonWheelZoomsWhileImageIsPannable()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    preparePannableImage(fixture);

    const qreal initialZoomPercent = invokeReal(fixture.root, "zoomPercent");
    sendRightButtonWheel(fixture.view.get(), QPointF(100.0, 80.0), 120);

    QTRY_VERIFY(invokeReal(fixture.root, "zoomPercent") > initialZoomPercent);
}

void TestImageViewport::rightButtonWheelZoomsFromViewportMarginAroundNearestImagePoint()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    invokeSetReal(fixture.root, "setManualZoom", 10.0);
    QTRY_VERIFY(!invokeBool(fixture.root, "imagePannable"));

    const qreal initialZoomPercent = invokeReal(fixture.root, "zoomPercent");
    sendRightButtonWheel(fixture.view.get(), QPointF(10.0, 10.0), 120);

    QTRY_VERIFY(invokeReal(fixture.root, "zoomPercent") > initialZoomPercent);
}

void TestImageViewport::plainWheelStillPansWhileImageIsPannable()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    preparePannableImage(fixture);

    const qreal initialZoomPercent = invokeReal(fixture.root, "zoomPercent");
    invokeSetReal(fixture.root, "setContentY", 400.0);
    QTRY_VERIFY(std::abs(invokeReal(fixture.root, "contentY") - 400.0) < 0.001);

    sendWheel(fixture.view.get(), QPointF(100.0, 80.0), -120);

    QTRY_VERIFY(std::abs(invokeReal(fixture.root, "contentY") - 400.0) > 0.001);
    QCOMPARE(invokeReal(fixture.root, "zoomPercent"), initialZoomPercent);
}

void TestImageViewport::fittedFractionalDisplayDoesNotEnableHorizontalPanning()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));

    invokeSetReal(fixture.root, "setManualZoom", 20.00005);

    QTRY_VERIFY(!invokeBool(fixture.root, "imageHorizontallyPannable"));
    QCOMPARE(invokeReal(fixture.root, "contentX"), 0.0);
    QTRY_VERIFY(invokeBool(fixture.root, "horizontalContentFitsViewport"));
}

void TestImageViewport::doubleClickTogglesFitToActualSize()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    invokeVoid(fixture.root, "setFitZoom");
    QTRY_COMPARE(invokeInt(fixture.root, "zoomMode"),
        static_cast<int>(FakeKiriImageDocument::ZoomMode::Fit));

    sendLeftDoubleClick(fixture.view.get(), QPointF(100.0, 80.0));

    QTRY_COMPARE(invokeInt(fixture.root, "zoomMode"),
        static_cast<int>(FakeKiriImageDocument::ZoomMode::Manual));
    QCOMPARE(invokeReal(fixture.root, "zoomPercent"), 100.0);
    QTRY_VERIFY(invokeReal(fixture.root, "contentX") > 0.0);
    QTRY_VERIFY(invokeReal(fixture.root, "contentY") > 0.0);
    QCOMPARE(fixture.root->property("viewerClickCount").toInt(), 0);
}

void TestImageViewport::doubleClickTogglesManualZoomToFit()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    preparePannableImage(fixture);

    sendLeftDoubleClick(fixture.view.get(), QPointF(100.0, 80.0));

    QTRY_COMPARE(invokeInt(fixture.root, "zoomMode"),
        static_cast<int>(FakeKiriImageDocument::ZoomMode::Fit));
    QVERIFY(invokeReal(fixture.root, "zoomPercent") < 100.0);
}

void TestImageViewport::doubleClickTogglesFitHeightToFit()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    invokeVoid(fixture.root, "setFitHeightZoom");
    QTRY_COMPARE(invokeInt(fixture.root, "zoomMode"),
        static_cast<int>(FakeKiriImageDocument::ZoomMode::FitHeight));

    sendLeftDoubleClick(fixture.view.get(), QPointF(100.0, 80.0));

    QTRY_COMPARE(invokeInt(fixture.root, "zoomMode"),
        static_cast<int>(FakeKiriImageDocument::ZoomMode::Fit));
}

void TestImageViewport::doubleClickFromViewportMarginZoomsAroundNearestImagePoint()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    invokeVoid(fixture.root, "setFitZoom");
    QTRY_VERIFY(!invokeBool(fixture.root, "imagePannable"));

    sendLeftDoubleClick(fixture.view.get(), QPointF(10.0, 10.0));

    QTRY_COMPARE(invokeInt(fixture.root, "zoomMode"),
        static_cast<int>(FakeKiriImageDocument::ZoomMode::Manual));
    QCOMPARE(invokeReal(fixture.root, "zoomPercent"), 100.0);
    QCOMPARE(invokeReal(fixture.root, "contentX"), 0.0);
    QTRY_VERIFY(invokeReal(fixture.root, "contentY") > 0.0);
}

void TestImageViewport::viewportUsesDisplayImagePagesWithContextBridge()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));

    QObject* primaryBridge
        = fixture.root->findChild<QObject*>(QStringLiteral("primaryContextBridge"));
    QObject* secondaryBridge
        = fixture.root->findChild<QObject*>(QStringLiteral("secondaryContextBridge"));
    QVERIFY(primaryBridge != nullptr);
    QVERIFY(secondaryBridge != nullptr);
    QCOMPARE(primaryBridge->property("secondaryPage").toBool(), false);
    QCOMPARE(secondaryBridge->property("secondaryPage").toBool(), true);
    QVERIFY(
        fixture.root->findChild<QObject*>(QStringLiteral("primaryDisplayImagePage")) != nullptr);
    QVERIFY(
        fixture.root->findChild<QObject*>(QStringLiteral("secondaryDisplayImagePage")) != nullptr);
    QVERIFY(fixture.root->findChild<QObject*>(QStringLiteral("primaryImageView")) == nullptr);
    QVERIFY(fixture.root->findChild<QObject*>(QStringLiteral("secondaryImageView")) == nullptr);
}

void TestImageViewport::displayImagePageLoadAcknowledgmentReachesDocument()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));

    invokeVoid(fixture.root, "requirePrimaryMissingLoadAcknowledgment");

    QTRY_COMPARE(invokeInt(fixture.root, "displayLoadAcknowledgmentCount"), 1);
    QCOMPARE(invokeInt(fixture.root, "lastAcknowledgedPageRole"), 0);
    QCOMPARE(invokeInt(fixture.root, "lastAcknowledgedOutcome"), 2);
    QCOMPARE(invokeString(fixture.root, "lastAcknowledgedRevisionToken"), QStringLiteral("42"));
    QCOMPARE(invokeString(fixture.root, "lastAcknowledgedSourceIdentity"),
        QStringLiteral("source-primary"));
}

void TestImageViewport::viewportHitTestingUsesDocumentFacade()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    invokeVoid(fixture.root, "resetHitTestHelperCallCounts");

    QVERIFY(invokeBool(fixture.root, "viewportCenterInsideImage"));
    QVERIFY(std::isfinite(invokeReal(fixture.root, "nearestMarginImageViewportPointX")));

    QCOMPARE(invokeInt(fixture.root, "legacyViewHitTestHelperCallCount"), 0);
    QVERIFY(invokeInt(fixture.root, "documentHitTestHelperCallCount") >= 2);
}

void TestImageViewport::singleClickStillEmitsViewerClicked()
{
    ImageViewportFixture fixture = createFixture();
    QVERIFY2(fixture.isValid(), qPrintable(fixture.errorString));
    QTRY_VERIFY(invokeBool(fixture.root, "documentReady"));
    invokeVoid(fixture.root, "resetViewerClickCount");

    sendLeftClick(fixture.view.get(), QPointF(100.0, 80.0));

    QTRY_COMPARE(fixture.root->property("viewerClickCount").toInt(), 1);
}

QTEST_MAIN(TestImageViewport)

#include "test_imageviewport.moc"

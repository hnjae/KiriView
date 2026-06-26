// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriviewapplication.h"

#include "application/applicationactionhost.h"
#include "application/applicationactionruntime.h"
#include "application/applicationactionsourceattachment.h"
#include "application/applicationcommandportsource.h"
#include "application/applicationcommandrouter.h"
#include "facade/kiridocumentsession.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"

#include <KLocalizedString>

#include <functional>
#include <utility>
#include <vector>

namespace Actions = kiriview::ApplicationActions;

namespace kiriview::ApplicationActions {
class KiriViewApplicationActionHost final : public ApplicationActionHost
{
public:
    explicit KiriViewApplicationActionHost(KiriViewApplication& application)
        : m_application(application)
    {
    }

    QObject* actionContext() override { return &m_application; }
    KirigamiActionCollection* mainActionCollection() override
    {
        return m_application.applicationMainActionCollection();
    }
    QAction* inheritedAction(const QString& actionName) override
    {
        return m_application.inheritedApplicationAction(actionName);
    }
    void readActionSettings() override { m_application.readApplicationActionSettings(); }

private:
    KiriViewApplication& m_application;
    Q_DISABLE_COPY(KiriViewApplicationActionHost)
};

class KiriViewApplicationActionStateSource final : public ApplicationActionStateSource
{
public:
    KiriViewApplicationActionStateSource() = default;

public:
    void setDocumentSession(KiriDocumentSession* session) { m_documentSession = session; }
    void setUiGateSnapshot(ApplicationActionUiGateSnapshot snapshot)
    {
        ++m_uiGateRevision;
        m_uiGateSnapshot = snapshot;
    }

    ApplicationActionStateSnapshot actionStateSnapshot() const override;
    std::vector<QMetaObject::Connection> connectActionStateChanged(
        QObject* context, std::function<void()> refresh) override;

private:
    KiriImageDocument* imageDocument() const;
    bool imageMode() const;
    bool videoMode() const;
    bool sharedImagePannable() const;

    QPointer<KiriDocumentSession> m_documentSession;
    ApplicationActionUiGateSnapshot m_uiGateSnapshot;
    quint64 m_uiGateRevision = 0;
    Q_DISABLE_COPY(KiriViewApplicationActionStateSource)
};

class KiriViewApplicationCommandPortSource final : public ApplicationCommandPortSource
{
public:
    explicit KiriViewApplicationCommandPortSource(KiriViewApplication& application)
        : m_application(application)
    {
    }

    ApplicationCommandRouterShellPorts commandRouterShellPorts() override;
    ApplicationCommandRouterSessionPorts commandRouterSessionPorts() override;
    ApplicationCommandRouterImageDocumentPorts commandRouterImageDocumentPorts() override;
    ApplicationCommandRouterImagePresentationPorts commandRouterImagePresentationPorts() override;
    ApplicationCommandRouterPanelPorts commandRouterPanelPorts() override;
    ApplicationCommandRouterWindowPorts commandRouterWindowPorts() override;
    ApplicationCommandRouterHelpPorts commandRouterHelpPorts() override;
    ApplicationCommandRouterVideoPorts commandRouterVideoPorts() override;

private:
    KiriViewApplication& m_application;
    Q_DISABLE_COPY(KiriViewApplicationCommandPortSource)
};
}

static_assert(static_cast<int>(Actions::MenuPresentation::HamburgerMenu)
    == static_cast<int>(KiriViewApplication::HamburgerMenu));
static_assert(static_cast<int>(Actions::MenuPresentation::MenuBar)
    == static_cast<int>(KiriViewApplication::MenuBar));
static_assert(static_cast<int>(Actions::ActionId::FileOpenAction)
    == static_cast<int>(KiriViewApplication::FileOpenAction));
static_assert(static_cast<int>(Actions::ActionId::ActionCount)
    == static_cast<int>(KiriViewApplication::ActionCount));

KiriViewApplication::KiriViewApplication(QObject* parent)
    : AbstractKirigamiApplication(parent)
    , m_actionHost(std::make_unique<Actions::KiriViewApplicationActionHost>(*this))
    , m_actionStateSource(std::make_unique<Actions::KiriViewApplicationActionStateSource>())
    , m_commandPortSource(std::make_unique<Actions::KiriViewApplicationCommandPortSource>(*this))
    , m_actionRuntime(std::make_unique<Actions::ApplicationActionRuntime>(*m_actionHost,
          Actions::ApplicationActionRuntime::Callbacks {
              [this]() { Q_EMIT menuPresentationChanged(); },
              [this]() { Q_EMIT shortcutRevisionChanged(); },
              [this]() { Q_EMIT actionStateRevisionChanged(); },
              [this](Actions::ActionId actionId) {
                  Q_EMIT unsupportedVideoActionTriggered(facadeActionId(actionId));
              },
              [this](Actions::ActionId actionId) {
                  Q_EMIT unsupportedImageActionTriggered(facadeActionId(actionId));
              },
          }))
    , m_actionSourceAttachment(
          std::make_unique<Actions::ApplicationActionSourceAttachment>(*m_actionRuntime, *this))
{
    m_actionRuntime->setCommandPortSource(m_commandPortSource.get());
    KiriViewApplication::setupActions();
    m_actionSourceAttachment->setSource(m_actionStateSource.get());
}

KiriViewApplication::~KiriViewApplication() = default;

KiriViewApplication::MenuPresentation KiriViewApplication::menuPresentation() const
{
    return facadeMenuPresentation(m_actionRuntime->menuPresentation());
}

void KiriViewApplication::setMenuPresentation(MenuPresentation presentation)
{
    m_actionRuntime->setMenuPresentation(domainMenuPresentation(presentation));
}

int KiriViewApplication::shortcutRevision() const { return m_actionRuntime->shortcutRevision(); }

int KiriViewApplication::actionStateRevision() const
{
    return m_actionRuntime->actionStateRevision();
}

QAbstractListModel* KiriViewApplication::shortcutHelpModel() const
{
    return m_actionRuntime->shortcutHelpModel();
}

QAbstractListModel* KiriViewApplication::shortcutRouteModel() const
{
    return m_actionRuntime->shortcutRouteModel();
}

Actions::MenuPresentation KiriViewApplication::domainMenuPresentation(MenuPresentation presentation)
{
    if (presentation == MenuBar) {
        return Actions::MenuPresentation::MenuBar;
    }

    return Actions::MenuPresentation::HamburgerMenu;
}

KiriViewApplication::MenuPresentation KiriViewApplication::facadeMenuPresentation(
    Actions::MenuPresentation presentation)
{
    if (presentation == Actions::MenuPresentation::MenuBar) {
        return MenuBar;
    }

    return HamburgerMenu;
}

Actions::ActionId KiriViewApplication::domainActionId(ActionId actionId)
{
    return static_cast<Actions::ActionId>(static_cast<int>(actionId));
}

KiriViewApplication::ActionId KiriViewApplication::facadeActionId(Actions::ActionId actionId)
{
    return static_cast<ActionId>(static_cast<int>(actionId));
}

QAction* KiriViewApplication::action(const QString& actionName)
{
    return m_actionRuntime->action(actionName);
}

QAction* KiriViewApplication::actionForId(ActionId actionId)
{
    return m_actionRuntime->actionForId(domainActionId(actionId));
}

QString KiriViewApplication::actionName(ActionId actionId) const
{
    return m_actionRuntime->actionName(domainActionId(actionId));
}

QList<QKeySequence> KiriViewApplication::shortcuts(const QString& actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcuts;
}

QList<QKeySequence> KiriViewApplication::shortcutsForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).shortcuts;
}

QList<QKeySequence> KiriViewApplication::programWideShortcuts(const QString& actionName) const
{
    return m_actionRuntime->programWideShortcuts(actionName);
}

QList<QKeySequence> KiriViewApplication::programWideShortcutsForId(ActionId actionId) const
{
    return m_actionRuntime->programWideShortcutsForId(domainActionId(actionId));
}

QList<QKeySequence> KiriViewApplication::viewerLocalShortcuts(const QString& actionName) const
{
    return m_actionRuntime->viewerLocalShortcuts(actionName);
}

QList<QKeySequence> KiriViewApplication::viewerLocalShortcutsForId(ActionId actionId) const
{
    return m_actionRuntime->viewerLocalShortcutsForId(domainActionId(actionId));
}

bool KiriViewApplication::setViewerLocalShortcuts(
    const QString& actionName, const QList<QKeySequence>& shortcuts)
{
    return m_actionRuntime->setViewerLocalShortcuts(actionName, shortcuts);
}

bool KiriViewApplication::setViewerLocalShortcutsForId(
    ActionId actionId, const QList<QKeySequence>& shortcuts)
{
    return m_actionRuntime->setViewerLocalShortcutsForId(domainActionId(actionId), shortcuts);
}

QString KiriViewApplication::shortcutText(const QString& actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutText;
}

QString KiriViewApplication::shortcutTextForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).shortcutText;
}

QKeySequence KiriViewApplication::menuShortcut(const QString& actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).menuShortcut;
}

QKeySequence KiriViewApplication::menuShortcutForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).menuShortcut;
}

QString KiriViewApplication::menuShortcutText(const QString& actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).menuShortcutText;
}

QString KiriViewApplication::menuShortcutTextForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).menuShortcutText;
}

bool KiriViewApplication::actionPlacementEnabled(ActionId actionId) const
{
    return m_actionRuntime->actionPlacementEnabled(domainActionId(actionId));
}

QString KiriViewApplication::actionMenuTextForId(ActionId actionId) const
{
    return m_actionRuntime->actionMenuText(domainActionId(actionId));
}

QString KiriViewApplication::actionToolbarTextForId(ActionId actionId) const
{
    return m_actionRuntime->actionToolbarText(domainActionId(actionId));
}

QString KiriViewApplication::actionToolbarTooltipTextForId(ActionId actionId) const
{
    return m_actionRuntime->actionToolbarTooltipText(domainActionId(actionId));
}

void KiriViewApplication::setDocumentSession(QObject* session)
{
    auto* documentSession = qobject_cast<KiriDocumentSession*>(session);
    if (m_documentSession == documentSession) {
        return;
    }

    m_documentSession = documentSession;
    m_actionStateSource->setDocumentSession(documentSession);
    m_actionSourceAttachment->reattach();
}

void KiriViewApplication::updateActionUiGateSnapshot(bool helpDialogOpen, bool textInputFocused,
    bool infoPanelVisible, bool thumbnailPanelVisible, bool fullscreen,
    bool applicationMenuShortcutEnabled, bool showMenubarActionEnabled)
{
    m_actionStateSource->setUiGateSnapshot(Actions::ApplicationActionUiGateSnapshot {
        helpDialogOpen,
        textInputFocused,
        infoPanelVisible,
        thumbnailPanelVisible,
        fullscreen,
        applicationMenuShortcutEnabled,
        showMenubarActionEnabled,
    });
    m_actionSourceAttachment->refresh();
}

void KiriViewApplication::setShortcutHost(QObject* host) { m_actionRuntime->setShortcutHost(host); }

bool KiriViewApplication::videoActionUnsupported(ActionId actionId) const
{
    return m_actionRuntime->videoActionUnsupported(domainActionId(actionId));
}

bool KiriViewApplication::imageActionUnsupported(ActionId actionId) const
{
    return m_actionRuntime->imageActionUnsupported(domainActionId(actionId));
}

bool KiriViewApplication::mediaHorizontalArrowShortcutsEnabled(bool videoMode,
    bool imageReadyViewerShortcutsEnabled, bool videoViewerShortcutsEnabled,
    bool videoDirectMediaNavigationActive, bool videoFileDeletionInProgress) const
{
    return m_actionRuntime->mediaHorizontalArrowShortcutsEnabled(videoMode,
        imageReadyViewerShortcutsEnabled, videoViewerShortcutsEnabled,
        videoDirectMediaNavigationActive, videoFileDeletionInProgress);
}

void KiriViewApplication::setupActions()
{
    AbstractKirigamiApplication::setupActions();
    if (m_actionRuntime != nullptr) {
        m_actionRuntime->setupActions();
    }
}

KirigamiActionCollection* KiriViewApplication::applicationMainActionCollection()
{
    return mainCollection();
}

QAction* KiriViewApplication::inheritedApplicationAction(const QString& actionName)
{
    return AbstractKirigamiApplication::action(actionName);
}

void KiriViewApplication::readApplicationActionSettings() { readSettings(); }

std::vector<QMetaObject::Connection>
Actions::KiriViewApplicationActionStateSource::connectActionStateChanged(
    QObject* context, std::function<void()> refresh)
{
    std::vector<QMetaObject::Connection> connections;
    if (m_documentSession == nullptr) {
        return connections;
    }

    const auto connectRefresh = [&connections, context, refresh](auto* sender, auto signal) {
        connections.push_back(
            QObject::connect(sender, signal, context, [refresh]() { refresh(); }));
    };

    KiriDocumentSession* session = m_documentSession.data();
    connectRefresh(session, &KiriDocumentSession::publicProjectionRevisionChanged);
    connectRefresh(session, &KiriDocumentSession::documentKindChanged);
    connectRefresh(session, &KiriDocumentSession::displayedFileDeletionAvailabilityChanged);
    connectRefresh(session, &KiriDocumentSession::displayedMediaOpenWithAvailabilityChanged);
    connectRefresh(session, &KiriDocumentSession::fileDeletionInProgressChanged);
    connectRefresh(session, &KiriDocumentSession::activeMediaReadinessChanged);
    connectRefresh(session, &KiriDocumentSession::activeNavigationChanged);
    if (KiriImageDocument* image = session->imageDocument()) {
        connectRefresh(image, &KiriImageDocument::viewportFrameChanged);
    }
    if (KiriVideoDocument* video = session->videoDocument()) {
        connectRefresh(video, &KiriVideoDocument::seekableChanged);
        connectRefresh(video, &KiriVideoDocument::durationChanged);
    }

    return connections;
}

KiriImageDocument* KiriViewApplication::imageDocument() const
{
    return m_documentSession == nullptr ? nullptr : m_documentSession->imageDocument();
}

KiriImageDocument* Actions::KiriViewApplicationActionStateSource::imageDocument() const
{
    return m_documentSession == nullptr ? nullptr : m_documentSession->imageDocument();
}

bool Actions::KiriViewApplicationActionStateSource::imageMode() const
{
    return m_documentSession != nullptr
        && m_documentSession->documentKind() == KiriDocumentSession::DocumentKind::Image;
}

bool Actions::KiriViewApplicationActionStateSource::videoMode() const
{
    return m_documentSession != nullptr
        && m_documentSession->documentKind() == KiriDocumentSession::DocumentKind::Video;
}

bool Actions::KiriViewApplicationActionStateSource::sharedImagePannable() const
{
    const KiriImageDocument* image = imageDocument();
    return imageMode() && image != nullptr && image->viewportPannable();
}

Actions::ApplicationActionStateSnapshot
Actions::KiriViewApplicationActionStateSource::actionStateSnapshot() const
{
    Actions::ApplicationActionStateSnapshot snapshot;
    snapshot.uiGateRevision = m_uiGateRevision;
    snapshot.sessionActionAvailability = m_documentSession == nullptr
        ? kiriview::DocumentSessionActionAvailabilityFacts {}
        : m_documentSession->actionAvailabilityFacts();
    snapshot.displayedMediaOpenWithAvailable
        = m_documentSession != nullptr && m_documentSession->displayedMediaOpenWithAvailable();
    snapshot.displayedFileDeletionAvailable
        = m_documentSession != nullptr && m_documentSession->displayedFileDeletionAvailable();
    snapshot.fileDeletionInProgress
        = m_documentSession != nullptr && m_documentSession->fileDeletionInProgress();
    snapshot.activeNavigationAvailable
        = m_documentSession != nullptr && m_documentSession->activeNavigationAvailable();
    snapshot.activeNavigationKnown
        = m_documentSession != nullptr && m_documentSession->activeNavigationKnown();
    snapshot.activeNavigationHasTargets
        = m_documentSession != nullptr && m_documentSession->activeNavigationHasTargets();
    snapshot.canOpenPreviousActiveNavigation
        = m_documentSession != nullptr && m_documentSession->canOpenPreviousActiveNavigation();
    snapshot.canOpenNextActiveNavigation
        = m_documentSession != nullptr && m_documentSession->canOpenNextActiveNavigation();
    snapshot.directMediaNavigationBoundaryActive
        = m_documentSession != nullptr && m_documentSession->directMediaNavigationBoundaryActive();
    snapshot.activeNavigationDispatchAvailable
        = m_documentSession != nullptr && m_documentSession->activeNavigationDispatchAvailable();
    snapshot.imageDocumentPageNavigationActive = m_documentSession != nullptr
        && m_documentSession->activeNavigationBoundaryScope()
            == KiriDocumentSession::ActiveNavigationBoundaryScope::
                ImageDocumentPageNavigationBoundary;
    snapshot.atKnownFirstActiveNavigation
        = m_documentSession != nullptr && m_documentSession->atKnownFirstActiveNavigation();
    snapshot.videoMode = videoMode();
    snapshot.helpDialogOpen = m_uiGateSnapshot.helpDialogOpen;
    snapshot.textInputFocused = m_uiGateSnapshot.textInputFocused;
    snapshot.imagePannable = sharedImagePannable();
    snapshot.infoPanelVisible = m_uiGateSnapshot.infoPanelVisible;
    snapshot.thumbnailPanelVisible = m_uiGateSnapshot.thumbnailPanelVisible;
    snapshot.fullscreen = m_uiGateSnapshot.fullscreen;
    snapshot.applicationMenuShortcutEnabled = m_uiGateSnapshot.applicationMenuShortcutEnabled;
    snapshot.showMenubarActionEnabled = m_uiGateSnapshot.showMenubarActionEnabled;
    if (KiriVideoDocument* video
        = m_documentSession == nullptr ? nullptr : m_documentSession->videoDocument()) {
        snapshot.videoSeekable = video->seekable();
        snapshot.videoDuration = video->duration();
    }
    return snapshot;
}

Actions::ApplicationCommandRouterShellPorts
Actions::KiriViewApplicationCommandPortSource::commandRouterShellPorts()
{
    Actions::ApplicationCommandRouterShellPorts ports;
    ports.requestOpenDialog = [this]() { Q_EMIT m_application.openDialogRequested(); };
    ports.openApplicationMenu = [this]() { Q_EMIT m_application.openApplicationMenuRequested(); };
    return ports;
}

Actions::ApplicationCommandRouterSessionPorts
Actions::KiriViewApplicationCommandPortSource::commandRouterSessionPorts()
{
    Actions::ApplicationCommandRouterSessionPorts ports;
    ports.openCurrentMediaWith = [this]() {
        if (m_application.m_documentSession != nullptr) {
            m_application.m_documentSession->openCurrentMediaWith();
        }
    };
    ports.moveDisplayedFileToTrash = [this]() { m_application.moveDisplayedFileToTrash(); };
    ports.deleteDisplayedFilePermanently
        = [this]() { m_application.deleteDisplayedFilePermanently(); };
    ports.requestPreviousActiveNavigationWithBoundary
        = [this]() { m_application.requestPreviousActiveNavigationWithBoundary(); };
    ports.requestNextActiveNavigationWithBoundary
        = [this]() { m_application.requestNextActiveNavigationWithBoundary(); };
    ports.openFirstActiveNavigation = [this]() {
        if (m_application.m_documentSession != nullptr) {
            m_application.m_documentSession->openFirstActiveNavigation();
        }
    };
    ports.openLastActiveNavigation = [this]() {
        if (m_application.m_documentSession != nullptr) {
            m_application.m_documentSession->openLastActiveNavigation();
        }
    };
    ports.showFirstImageBoundary = [this]() {
        Q_EMIT m_application.imageBoundaryReached(i18nc("@info:status", "First image"));
    };
    return ports;
}

Actions::ApplicationCommandRouterImageDocumentPorts
Actions::KiriViewApplicationCommandPortSource::commandRouterImageDocumentPorts()
{
    Actions::ApplicationCommandRouterImageDocumentPorts ports;
    ports.imageAvailable = [this]() { return m_application.imageDocument() != nullptr; };
    ports.openPreviousContainer = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->openPreviousContainer();
        }
    };
    ports.openNextContainer = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->openNextContainer();
        }
    };
    ports.openPreviousSinglePage = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->openPreviousSinglePage();
        }
    };
    ports.openNextSinglePage = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->openNextSinglePage();
        }
    };
    ports.rotateClockwise = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->rotateClockwise();
        }
    };
    ports.rotateCounterclockwise = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->rotateCounterclockwise();
        }
    };
    ports.requestToggleTwoPageMode = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->requestToggleTwoPageMode();
        }
    };
    ports.requestToggleRightToLeftReading = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->requestToggleRightToLeftReading();
        }
    };
    return ports;
}

Actions::ApplicationCommandRouterImagePresentationPorts
Actions::KiriViewApplicationCommandPortSource::commandRouterImagePresentationPorts()
{
    Actions::ApplicationCommandRouterImagePresentationPorts ports;
    ports.imageViewportHorizontallyPannable = [this]() {
        KiriImageDocument* image = m_application.imageDocument();
        return image != nullptr && image->viewportHorizontallyPannable();
    };
    ports.requestViewportPanBy = [this](double dx, double dy) {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->requestViewportPanBy(dx, dy);
        }
    };
    ports.requestViewportScanForward = [this]() {
        KiriImageDocument* image = m_application.imageDocument();
        return image != nullptr && image->requestViewportScanForward();
    };
    ports.requestViewportScanBackward = [this]() {
        KiriImageDocument* image = m_application.imageDocument();
        return image != nullptr && image->requestViewportScanBackward();
    };
    ports.requestNextDisplayedImageStartToFinalScanPosition = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->requestNextDisplayedImageStartToFinalScanPosition();
        }
    };
    ports.requestViewportPanToInitialScanPosition = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->requestViewportPanToInitialScanPosition();
        }
    };
    ports.requestViewportPanToFinalScanPosition = [this]() {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->requestViewportPanToFinalScanPosition();
        }
    };
    ports.requestZoomByStepAtCenter = [this](double stepCount) {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->requestZoomByStepAtCenter(stepCount);
        }
    };
    ports.requestManualZoomPercent = [this](double zoomPercent) {
        if (KiriImageDocument* image = m_application.imageDocument()) {
            image->requestManualZoomPercent(zoomPercent);
        }
    };
    ports.requestFitMode = [this]() { m_application.requestImageFitMode(); };
    ports.requestFitHeightMode = [this]() { m_application.requestImageFitHeightMode(); };
    ports.requestFitWidthMode = [this]() { m_application.requestImageFitWidthMode(); };
    return ports;
}

Actions::ApplicationCommandRouterPanelPorts
Actions::KiriViewApplicationCommandPortSource::commandRouterPanelPorts()
{
    Actions::ApplicationCommandRouterPanelPorts ports;
    ports.toggleInfoPanel = [this]() { Q_EMIT m_application.toggleInfoPanelRequested(); };
    ports.toggleThumbnailPanel = [this]() { Q_EMIT m_application.toggleThumbnailPanelRequested(); };
    return ports;
}

Actions::ApplicationCommandRouterWindowPorts
Actions::KiriViewApplicationCommandPortSource::commandRouterWindowPorts()
{
    Actions::ApplicationCommandRouterWindowPorts ports;
    ports.toggleFullScreen = [this]() { Q_EMIT m_application.toggleFullScreenRequested(); };
    return ports;
}

Actions::ApplicationCommandRouterHelpPorts
Actions::KiriViewApplicationCommandPortSource::commandRouterHelpPorts()
{
    Actions::ApplicationCommandRouterHelpPorts ports;
    ports.requestShortcutHelp = [this]() { Q_EMIT m_application.shortcutHelpRequested(); };
    return ports;
}

Actions::ApplicationCommandRouterVideoPorts
Actions::KiriViewApplicationCommandPortSource::commandRouterVideoPorts()
{
    Actions::ApplicationCommandRouterVideoPorts ports;
    ports.videoAvailable = [this]() {
        return m_application.m_documentSession != nullptr
            && m_application.m_documentSession->videoDocument() != nullptr;
    };
    ports.videoSeekable = [this]() {
        KiriVideoDocument* video = m_application.m_documentSession == nullptr
            ? nullptr
            : m_application.m_documentSession->videoDocument();
        return video != nullptr && video->seekable();
    };
    ports.videoDuration = [this]() {
        KiriVideoDocument* video = m_application.m_documentSession == nullptr
            ? nullptr
            : m_application.m_documentSession->videoDocument();
        return video == nullptr ? qint64(0) : video->duration();
    };
    ports.seekVideoBy = [this](qint64 deltaMilliseconds) {
        if (KiriVideoDocument* video = m_application.m_documentSession == nullptr
                ? nullptr
                : m_application.m_documentSession->videoDocument()) {
            video->seekBy(deltaMilliseconds);
        }
    };
    ports.setVideoPosition = [this](qint64 positionMilliseconds) {
        if (KiriVideoDocument* video = m_application.m_documentSession == nullptr
                ? nullptr
                : m_application.m_documentSession->videoDocument()) {
            video->setPosition(positionMilliseconds);
        }
    };
    ports.toggleVideoPlayback = [this]() {
        if (KiriVideoDocument* video = m_application.m_documentSession == nullptr
                ? nullptr
                : m_application.m_documentSession->videoDocument()) {
            video->togglePlayback();
        }
    };
    return ports;
}

void KiriViewApplication::moveDisplayedFileToTrash()
{
    if (m_documentSession != nullptr) {
        const auto mode = KiriDocumentSession::DeletionMode::MoveToTrash;
        m_documentSession->deleteDisplayedFile(mode);
    }
}

void KiriViewApplication::deleteDisplayedFilePermanently()
{
    if (m_documentSession != nullptr) {
        const auto mode = KiriDocumentSession::DeletionMode::DeletePermanently;
        m_documentSession->deleteDisplayedFile(mode);
    }
}

void KiriViewApplication::requestImageFitMode()
{
    if (KiriImageDocument* image = imageDocument()) {
        const auto mode = KiriImageDocument::ZoomMode::Fit;
        image->requestFitMode(mode);
    }
}

void KiriViewApplication::requestImageFitHeightMode()
{
    if (KiriImageDocument* image = imageDocument()) {
        const auto mode = KiriImageDocument::ZoomMode::FitHeight;
        image->requestFitMode(mode);
    }
}

void KiriViewApplication::requestImageFitWidthMode()
{
    if (KiriImageDocument* image = imageDocument()) {
        const auto mode = KiriImageDocument::ZoomMode::FitWidth;
        image->requestFitMode(mode);
    }
}

void KiriViewApplication::emitBoundaryText(const QString& message)
{
    if (!message.isEmpty()) {
        Q_EMIT imageBoundaryReached(message);
    }
}

void KiriViewApplication::requestPreviousActiveNavigationWithBoundary()
{
    if (m_documentSession != nullptr) {
        emitBoundaryText(m_documentSession->requestPreviousActiveNavigationBoundaryText());
    }
}

void KiriViewApplication::requestNextActiveNavigationWithBoundary()
{
    if (m_documentSession != nullptr) {
        emitBoundaryText(m_documentSession->requestNextActiveNavigationBoundaryText());
    }
}

void KiriViewApplication::handleScanForwardAction() { m_actionRuntime->handleScanForwardAction(); }

void KiriViewApplication::handleScanBackwardAction()
{
    m_actionRuntime->handleScanBackwardAction();
}

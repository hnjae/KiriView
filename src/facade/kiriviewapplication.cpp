// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriviewapplication.h"

#include "application/applicationactionhost.h"
#include "application/applicationactionruntime.h"
#include "facade/kiridocumentsession.h"

#include <KLocalizedString>

#include <utility>

namespace Actions = KiriView::ApplicationActions;

namespace {
bool sharedImagePannabilityActionGate(
    const KiriView::DocumentSessionActionAvailabilityFacts &facts, bool viewportLocalPannable)
{
    return facts.imageReady && viewportLocalPannable;
}
}

namespace KiriView::ApplicationActions {
class KiriViewApplicationActionHost final : public ApplicationActionHost
{
public:
    explicit KiriViewApplicationActionHost(KiriViewApplication &application)
        : m_application(application)
    {
    }

    QObject *actionContext() override { return &m_application; }
    KirigamiActionCollection *mainActionCollection() override
    {
        return m_application.applicationMainActionCollection();
    }
    QAction *inheritedAction(const QString &actionName) override
    {
        return m_application.inheritedApplicationAction(actionName);
    }
    void readActionSettings() override { m_application.readApplicationActionSettings(); }

private:
    KiriViewApplication &m_application;
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

KiriViewApplication::KiriViewApplication(QObject *parent)
    : AbstractKirigamiApplication(parent)
    , m_actionHost(std::make_unique<Actions::KiriViewApplicationActionHost>(*this))
    , m_actionRuntime(std::make_unique<Actions::ApplicationActionRuntime>(*m_actionHost,
          Actions::ApplicationActionRuntime::Callbacks {
              [this]() { Q_EMIT menuPresentationChanged(); },
              [this]() { Q_EMIT shortcutRevisionChanged(); },
              [this]() { Q_EMIT actionStateRevisionChanged(); },
              [this](Actions::ActionId actionId) { handleRuntimeActionTriggered(actionId); },
              [this](Actions::ActionId actionId) {
                  Q_EMIT unsupportedVideoActionTriggered(facadeActionId(actionId));
              },
              [this](Actions::ActionId actionId) {
                  Q_EMIT unsupportedImageActionTriggered(facadeActionId(actionId));
              },
              [this](bool leftArrow) { return executeHorizontalArrowShortcut(leftArrow); },
              [this](bool leftArrow) { return executeSinglePageArrowShortcut(leftArrow); },
              [this](bool up) { return executeVerticalPanShortcut(up); },
              [this](
                  qint64 deltaMilliseconds) { return executeVideoSeekShortcut(deltaMilliseconds); },
          }))
{
    KiriViewApplication::setupActions();
    rebuildActionState();
}

KiriViewApplication::~KiriViewApplication() { disconnectActionStateSources(); }

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

QAbstractListModel *KiriViewApplication::shortcutHelpModel() const
{
    return m_actionRuntime->shortcutHelpModel();
}

QAbstractListModel *KiriViewApplication::shortcutRouteModel() const
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

QAction *KiriViewApplication::action(const QString &actionName)
{
    return m_actionRuntime->action(actionName);
}

QAction *KiriViewApplication::actionForId(ActionId actionId)
{
    return m_actionRuntime->actionForId(domainActionId(actionId));
}

QString KiriViewApplication::actionName(ActionId actionId) const
{
    return m_actionRuntime->actionName(domainActionId(actionId));
}

QList<QKeySequence> KiriViewApplication::shortcuts(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcuts;
}

QList<QKeySequence> KiriViewApplication::shortcutsForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).shortcuts;
}

QList<QKeySequence> KiriViewApplication::programWideShortcuts(const QString &actionName) const
{
    return m_actionRuntime->programWideShortcuts(actionName);
}

QList<QKeySequence> KiriViewApplication::programWideShortcutsForId(ActionId actionId) const
{
    return m_actionRuntime->programWideShortcutsForId(domainActionId(actionId));
}

QList<QKeySequence> KiriViewApplication::viewerLocalShortcuts(const QString &actionName) const
{
    return m_actionRuntime->viewerLocalShortcuts(actionName);
}

QList<QKeySequence> KiriViewApplication::viewerLocalShortcutsForId(ActionId actionId) const
{
    return m_actionRuntime->viewerLocalShortcutsForId(domainActionId(actionId));
}

bool KiriViewApplication::setViewerLocalShortcuts(
    const QString &actionName, const QList<QKeySequence> &shortcuts)
{
    return m_actionRuntime->setViewerLocalShortcuts(actionName, shortcuts);
}

bool KiriViewApplication::setViewerLocalShortcutsForId(
    ActionId actionId, const QList<QKeySequence> &shortcuts)
{
    return m_actionRuntime->setViewerLocalShortcutsForId(domainActionId(actionId), shortcuts);
}

QString KiriViewApplication::shortcutText(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutText;
}

QString KiriViewApplication::shortcutTextForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).shortcutText;
}

QKeySequence KiriViewApplication::menuShortcut(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).menuShortcut;
}

QKeySequence KiriViewApplication::menuShortcutForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).menuShortcut;
}

QString KiriViewApplication::menuShortcutText(const QString &actionName) const
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

void KiriViewApplication::setDocumentSession(QObject *session)
{
    auto *documentSession = qobject_cast<KiriDocumentSession *>(session);
    if (m_documentSession == documentSession) {
        return;
    }

    disconnectActionStateSources();
    m_documentSession = documentSession;
    connectActionStateSources();
    rebuildActionState();
}

void KiriViewApplication::updateActionUiGateSnapshot(bool helpDialogOpen, bool textInputFocused,
    bool infoPanelVisible, bool thumbnailPanelVisible, bool fullscreen,
    bool applicationMenuShortcutEnabled, bool showMenubarActionEnabled)
{
    updateActionUiGateSnapshot(ActionUiGateSnapshot {
        helpDialogOpen,
        textInputFocused,
        infoPanelVisible,
        thumbnailPanelVisible,
        fullscreen,
        applicationMenuShortcutEnabled,
        showMenubarActionEnabled,
    });
}

void KiriViewApplication::updateActionUiGateSnapshot(ActionUiGateSnapshot snapshot)
{
    applyActionUiGateSnapshot(snapshot);
}

void KiriViewApplication::applyActionUiGateSnapshot(const ActionUiGateSnapshot &snapshot)
{
    ++m_actionUiGateRevision;
    m_helpDialogOpen = snapshot.helpDialogOpen;
    m_textInputFocused = snapshot.textInputFocused;
    m_infoPanelVisible = snapshot.infoPanelVisible;
    m_thumbnailPanelVisible = snapshot.thumbnailPanelVisible;
    m_fullscreen = snapshot.fullscreen;
    m_applicationMenuShortcutEnabled = snapshot.applicationMenuShortcutEnabled;
    m_showMenubarActionEnabled = snapshot.showMenubarActionEnabled;
    rebuildActionState();
}

void KiriViewApplication::setShortcutHost(QObject *host) { m_actionRuntime->setShortcutHost(host); }

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

KirigamiActionCollection *KiriViewApplication::applicationMainActionCollection()
{
    return mainCollection();
}

QAction *KiriViewApplication::inheritedApplicationAction(const QString &actionName)
{
    return AbstractKirigamiApplication::action(actionName);
}

void KiriViewApplication::readApplicationActionSettings() { readSettings(); }

void KiriViewApplication::rebuildActionState()
{
    m_imageActionProjection = imageActionAvailabilityProjection(imageActionAvailabilityInput());
    m_actionStateInput = actionStateInput();
    if (m_actionRuntime != nullptr) {
        m_actionRuntime->setActionStateInput(m_actionStateInput);
    }
}

void KiriViewApplication::connectActionStateSources()
{
    if (m_documentSession == nullptr) {
        return;
    }

    const auto connectRebuild = [this](auto *sender, auto signal) {
        m_actionStateConnections.push_back(
            connect(sender, signal, this, [this]() { rebuildActionState(); }));
    };

    KiriDocumentSession *session = m_documentSession.data();
    connectRebuild(session, &KiriDocumentSession::publicProjectionRevisionChanged);
    connectRebuild(session, &KiriDocumentSession::documentKindChanged);
    connectRebuild(session, &KiriDocumentSession::displayedFileDeletionAvailabilityChanged);
    connectRebuild(session, &KiriDocumentSession::displayedMediaOpenWithAvailabilityChanged);
    connectRebuild(session, &KiriDocumentSession::fileDeletionInProgressChanged);
    connectRebuild(session, &KiriDocumentSession::activeMediaReadinessChanged);
    connectRebuild(session, &KiriDocumentSession::activeNavigationChanged);
    if (KiriImageDocument *image = session->imageDocument()) {
        connectRebuild(image, &KiriImageDocument::viewportFrameChanged);
    }
}

void KiriViewApplication::disconnectActionStateSources()
{
    for (const QMetaObject::Connection &connection : m_actionStateConnections) {
        QObject::disconnect(connection);
    }
    m_actionStateConnections.clear();
}

KiriImageDocument *KiriViewApplication::imageDocument() const
{
    return m_documentSession == nullptr ? nullptr : m_documentSession->imageDocument();
}

bool KiriViewApplication::imageMode() const
{
    return m_documentSession != nullptr
        && m_documentSession->documentKind() == KiriDocumentSession::DocumentKind::Image;
}

bool KiriViewApplication::videoMode() const
{
    return m_documentSession != nullptr
        && m_documentSession->documentKind() == KiriDocumentSession::DocumentKind::Video;
}

bool KiriViewApplication::sharedImagePannable() const
{
    const KiriImageDocument *image = imageDocument();
    return imageMode() && image != nullptr && image->viewportPannable();
}

ImageActionAvailabilityInput KiriViewApplication::imageActionAvailabilityInput() const
{
    const KiriView::DocumentSessionActionAvailabilityFacts facts = m_documentSession == nullptr
        ? KiriView::DocumentSessionActionAvailabilityFacts {}
        : m_documentSession->actionAvailabilityFacts();

    return ImageActionAvailabilityInput {
        facts.imageReady,
        m_documentSession != nullptr && m_documentSession->fileDeletionInProgress(),
        m_helpDialogOpen,
        m_textInputFocused,
        sharedImagePannabilityActionGate(facts, sharedImagePannable()),
        facts.containerNavigationAvailable,
        facts.twoPageModeActive,
        facts.twoPageModeAvailable,
        facts.rightToLeftReadingActive,
        facts.rightToLeftReadingAvailable,
    };
}

Actions::ApplicationActionStateInput KiriViewApplication::actionStateInput() const
{
    Actions::ApplicationActionStateInput input;
    const KiriView::DocumentSessionActionAvailabilityFacts facts = m_documentSession == nullptr
        ? KiriView::DocumentSessionActionAvailabilityFacts {}
        : m_documentSession->actionAvailabilityFacts();
    const bool activeVideoMode = videoMode();
    const bool activeNavigationActionsAvailable = m_documentSession != nullptr
        && m_documentSession->activeNavigationDispatchAvailable()
        && m_imageActionProjection.helpShortcutsEnabled;

    input.uiGateRevision = m_actionUiGateRevision;
    input.helpActionsEnabled = m_imageActionProjection.helpShortcutsEnabled;
    input.readyActionsEnabled = m_imageActionProjection.canUseReadyActions;
    input.rotateActionsEnabled = m_imageActionProjection.canUseRotateActions;
    input.twoPageModeActionsEnabled = m_imageActionProjection.canUseTwoPageModeActions;
    input.rightToLeftReadingActionsEnabled
        = m_imageActionProjection.canUseRightToLeftReadingActions;
    input.containerNavigationActionsEnabled = m_imageActionProjection.containerShortcutsEnabled;
    input.displayedMediaOpenWithAvailable
        = m_documentSession != nullptr && m_documentSession->displayedMediaOpenWithAvailable();
    input.displayedFileDeletionAvailable
        = m_documentSession != nullptr && m_documentSession->displayedFileDeletionAvailable();
    input.fileDeletionInProgress
        = m_documentSession != nullptr && m_documentSession->fileDeletionInProgress();
    input.activeNavigationAvailable
        = m_documentSession != nullptr && m_documentSession->activeNavigationAvailable();
    input.activeNavigationKnown
        = m_documentSession != nullptr && m_documentSession->activeNavigationKnown();
    input.activeNavigationHasTargets
        = m_documentSession != nullptr && m_documentSession->activeNavigationHasTargets();
    input.canOpenPreviousActiveNavigation
        = m_documentSession != nullptr && m_documentSession->canOpenPreviousActiveNavigation();
    input.canOpenNextActiveNavigation
        = m_documentSession != nullptr && m_documentSession->canOpenNextActiveNavigation();
    input.fitModeSelected = facts.fitModeSelected;
    input.fitHeightModeSelected = facts.fitHeightModeSelected;
    input.fitWidthModeSelected = facts.fitWidthModeSelected;
    input.twoPageModeActive = m_imageActionProjection.twoPageModeActive;
    input.rightToLeftReadingActive = m_imageActionProjection.rightToLeftReadingActive;
    input.infoPanelVisible = m_infoPanelVisible;
    input.thumbnailPanelVisible = m_thumbnailPanelVisible;
    input.fullscreen = m_fullscreen;
    input.applicationMenuShortcutEnabled = m_applicationMenuShortcutEnabled;
    input.showMenubarActionEnabled = m_showMenubarActionEnabled;
    input.directMediaNavigationBoundaryActive
        = m_documentSession != nullptr && m_documentSession->directMediaNavigationBoundaryActive();
    input.viewerShortcutsEnabled = m_imageActionProjection.viewerShortcutsEnabled;
    input.readyShortcutsEnabled = m_imageActionProjection.readyShortcutsEnabled;
    input.readyViewerShortcutsEnabled = m_imageActionProjection.readyViewerShortcutsEnabled;
    input.twoPageViewerShortcutsEnabled = m_imageActionProjection.twoPageViewerShortcutsEnabled;
    input.rightToLeftReadingShortcutsEnabled
        = m_imageActionProjection.rightToLeftReadingShortcutsEnabled;
    input.rightToLeftReadingViewerShortcutsEnabled
        = m_imageActionProjection.rightToLeftReadingViewerShortcutsEnabled;
    input.rotateShortcutsEnabled = m_imageActionProjection.rotateShortcutsEnabled;
    input.rotateViewerShortcutsEnabled = m_imageActionProjection.rotateViewerShortcutsEnabled;
    input.pannableShortcutsEnabled = m_imageActionProjection.pannableShortcutsEnabled;
    input.pannableViewerShortcutsEnabled = m_imageActionProjection.pannableViewerShortcutsEnabled;
    input.containerViewerShortcutsEnabled = m_imageActionProjection.containerViewerShortcutsEnabled;
    input.activeNavigationActionsAvailable = activeNavigationActionsAvailable;
    input.videoMode = activeVideoMode;
    input.videoFileDeletionInProgress
        = m_documentSession != nullptr && m_documentSession->fileDeletionInProgress();
    return input;
}

Actions::ApplicationCommandRouterInput KiriViewApplication::commandRouterInput() const
{
    Actions::ApplicationCommandRouterInput input;
    input.imagePannable = sharedImagePannable();
    input.rightToLeftReadingActive = m_imageActionProjection.rightToLeftReadingActive;
    input.videoMode = videoMode();
    input.imageDocumentPageNavigationActive = m_documentSession != nullptr
        && m_documentSession->activeNavigationBoundaryScope()
            == KiriDocumentSession::ActiveNavigationBoundaryScope::
                ImageDocumentPageNavigationBoundary;
    input.atKnownFirstActiveNavigation
        = m_documentSession != nullptr && m_documentSession->atKnownFirstActiveNavigation();
    input.canOpenPreviousActiveNavigation
        = m_documentSession != nullptr && m_documentSession->canOpenPreviousActiveNavigation();
    return input;
}

Actions::ApplicationCommandRouterPorts KiriViewApplication::commandRouterPorts()
{
    Actions::ApplicationCommandRouterPorts ports;
    ports.requestOpenDialog = [this]() { Q_EMIT openDialogRequested(); };
    ports.openCurrentMediaWith = [this]() {
        if (m_documentSession != nullptr) {
            m_documentSession->openCurrentMediaWith();
        }
    };
    ports.moveDisplayedFileToTrash = [this]() { moveDisplayedFileToTrash(); };
    ports.deleteDisplayedFilePermanently = [this]() { deleteDisplayedFilePermanently(); };
    ports.imageAvailable = [this]() { return imageDocument() != nullptr; };
    ports.openPreviousContainer = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->openPreviousContainer();
        }
    };
    ports.openNextContainer = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->openNextContainer();
        }
    };
    ports.imageViewportHorizontallyPannable = [this]() {
        KiriImageDocument *image = imageDocument();
        return image != nullptr && image->viewportHorizontallyPannable();
    };
    ports.requestViewportPanBy = [this](double dx, double dy) {
        if (KiriImageDocument *image = imageDocument()) {
            image->requestViewportPanBy(dx, dy);
        }
    };
    ports.requestViewportScanForward = [this]() {
        KiriImageDocument *image = imageDocument();
        return image != nullptr && image->requestViewportScanForward();
    };
    ports.requestViewportScanBackward = [this]() {
        KiriImageDocument *image = imageDocument();
        return image != nullptr && image->requestViewportScanBackward();
    };
    ports.requestNextDisplayedImageStartToFinalScanPosition = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->requestNextDisplayedImageStartToFinalScanPosition();
        }
    };
    ports.requestViewportPanToInitialScanPosition = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->requestViewportPanToInitialScanPosition();
        }
    };
    ports.requestViewportPanToFinalScanPosition = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->requestViewportPanToFinalScanPosition();
        }
    };
    ports.openPreviousSinglePage = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->openPreviousSinglePage();
        }
    };
    ports.openNextSinglePage = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->openNextSinglePage();
        }
    };
    ports.requestPreviousActiveNavigationWithBoundary
        = [this]() { requestPreviousActiveNavigationWithBoundary(); };
    ports.requestNextActiveNavigationWithBoundary
        = [this]() { requestNextActiveNavigationWithBoundary(); };
    ports.openFirstActiveNavigation = [this]() {
        if (m_documentSession != nullptr) {
            m_documentSession->openFirstActiveNavigation();
        }
    };
    ports.openLastActiveNavigation = [this]() {
        if (m_documentSession != nullptr) {
            m_documentSession->openLastActiveNavigation();
        }
    };
    ports.requestZoomByStepAtCenter = [this](double stepCount) {
        if (KiriImageDocument *image = imageDocument()) {
            image->requestZoomByStepAtCenter(stepCount);
        }
    };
    ports.requestManualZoomPercent = [this](double zoomPercent) {
        if (KiriImageDocument *image = imageDocument()) {
            image->requestManualZoomPercent(zoomPercent);
        }
    };
    ports.requestFitMode = [this]() { requestImageFitMode(); };
    ports.requestFitHeightMode = [this]() { requestImageFitHeightMode(); };
    ports.requestFitWidthMode = [this]() { requestImageFitWidthMode(); };
    ports.rotateClockwise = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->rotateClockwise();
        }
    };
    ports.rotateCounterclockwise = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->rotateCounterclockwise();
        }
    };
    ports.requestToggleTwoPageMode = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->requestToggleTwoPageMode();
        }
    };
    ports.requestToggleRightToLeftReading = [this]() {
        if (KiriImageDocument *image = imageDocument()) {
            image->requestToggleRightToLeftReading();
        }
    };
    ports.toggleInfoPanel = [this]() { Q_EMIT toggleInfoPanelRequested(); };
    ports.toggleThumbnailPanel = [this]() { Q_EMIT toggleThumbnailPanelRequested(); };
    ports.showFirstImageBoundary
        = [this]() { Q_EMIT imageBoundaryReached(i18nc("@info:status", "First image")); };
    ports.toggleFullScreen = [this]() { Q_EMIT toggleFullScreenRequested(); };
    ports.requestShortcutHelp = [this]() { Q_EMIT shortcutHelpRequested(); };
    ports.openApplicationMenu = [this]() { Q_EMIT openApplicationMenuRequested(); };
    ports.videoAvailable = [this]() {
        return m_documentSession != nullptr && m_documentSession->videoDocument() != nullptr;
    };
    ports.videoSeekable = [this]() {
        KiriVideoDocument *video
            = m_documentSession == nullptr ? nullptr : m_documentSession->videoDocument();
        return video != nullptr && video->seekable();
    };
    ports.seekVideoBy = [this](qint64 deltaMilliseconds) {
        if (KiriVideoDocument *video
            = m_documentSession == nullptr ? nullptr : m_documentSession->videoDocument()) {
            video->seekBy(deltaMilliseconds);
        }
    };
    ports.toggleVideoPlayback = [this]() {
        if (KiriVideoDocument *video
            = m_documentSession == nullptr ? nullptr : m_documentSession->videoDocument()) {
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
    if (KiriImageDocument *image = imageDocument()) {
        const auto mode = KiriImageDocument::ZoomMode::Fit;
        image->requestFitMode(mode);
    }
}

void KiriViewApplication::requestImageFitHeightMode()
{
    if (KiriImageDocument *image = imageDocument()) {
        const auto mode = KiriImageDocument::ZoomMode::FitHeight;
        image->requestFitMode(mode);
    }
}

void KiriViewApplication::requestImageFitWidthMode()
{
    if (KiriImageDocument *image = imageDocument()) {
        const auto mode = KiriImageDocument::ZoomMode::FitWidth;
        image->requestFitMode(mode);
    }
}

void KiriViewApplication::handleRuntimeActionTriggered(Actions::ActionId actionId)
{
    m_commandRouter.handleActionTriggered(actionId, commandRouterInput(), commandRouterPorts());
}

void KiriViewApplication::emitBoundaryText(const QString &message)
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

void KiriViewApplication::handleScanForwardAction()
{
    m_commandRouter.handleScanForwardAction(commandRouterInput(), commandRouterPorts());
}

void KiriViewApplication::handleScanBackwardAction()
{
    m_commandRouter.handleScanBackwardAction(commandRouterInput(), commandRouterPorts());
}

bool KiriViewApplication::executeHorizontalArrowShortcut(bool leftArrow)
{
    return m_commandRouter.executeHorizontalArrowShortcut(
        commandRouterInput(), commandRouterPorts(), leftArrow);
}

bool KiriViewApplication::executeSinglePageArrowShortcut(bool leftArrow)
{
    return m_commandRouter.executeSinglePageArrowShortcut(
        commandRouterInput(), commandRouterPorts(), leftArrow);
}

bool KiriViewApplication::executeVerticalPanShortcut(bool up)
{
    return m_commandRouter.executeVerticalPanShortcut(
        commandRouterInput(), commandRouterPorts(), up);
}

bool KiriViewApplication::executeVideoSeekShortcut(qint64 deltaMilliseconds)
{
    return m_commandRouter.executeVideoSeekShortcut(
        commandRouterInput(), commandRouterPorts(), deltaMilliseconds);
}

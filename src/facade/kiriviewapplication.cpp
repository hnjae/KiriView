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
constexpr double keyboardPanDistance = 64.0;
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
              [this](bool leftArrow) { return executeHorizontalArrowShortcut(leftArrow); },
              [this](bool leftArrow) { return executeSinglePageArrowShortcut(leftArrow); },
              [this](bool up) { return executeVerticalPanShortcut(up); },
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

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutsWithCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifierForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId))
        .shortcutsWithCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutsWithoutCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifierForId(
    ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId))
        .shortcutsWithoutCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutAliases(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutAliases;
}

QList<QKeySequence> KiriViewApplication::shortcutAliasesForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).shortcutAliases;
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
    bool imagePannable, bool infoPanelVisible, bool thumbnailPanelVisible, bool fullscreen,
    bool applicationMenuShortcutEnabled, bool showMenubarActionEnabled)
{
    updateActionUiGateSnapshot(ActionUiGateSnapshot {
        helpDialogOpen,
        textInputFocused,
        imagePannable,
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
    m_imagePannable = snapshot.imagePannable;
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
        facts.imageReady && m_imagePannable,
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

void KiriViewApplication::handleRuntimeActionTriggered(Actions::ActionId actionId)
{
    KiriImageDocument *image = imageDocument();
    switch (actionId) {
    case Actions::ActionId::FileOpenAction:
        Q_EMIT openDialogRequested();
        return;
    case Actions::ActionId::FileOpenWithAction:
        if (m_documentSession != nullptr) {
            m_documentSession->openCurrentMediaWith();
        }
        return;
    case Actions::ActionId::FileMoveToTrashAction:
        if (m_documentSession != nullptr) {
            m_documentSession->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);
        }
        return;
    case Actions::ActionId::FileDeleteAction:
        if (m_documentSession != nullptr) {
            m_documentSession->deleteDisplayedFile(
                KiriDocumentSession::DeletionMode::DeletePermanently);
        }
        return;
    case Actions::ActionId::GoPreviousArchiveAction:
        if (image != nullptr) {
            image->openPreviousContainer();
        }
        return;
    case Actions::ActionId::GoNextArchiveAction:
        if (image != nullptr) {
            image->openNextContainer();
        }
        return;
    case Actions::ActionId::GoPreviousImageAction:
        requestPreviousActiveNavigationWithBoundary();
        return;
    case Actions::ActionId::GoNextImageAction:
        requestNextActiveNavigationWithBoundary();
        return;
    case Actions::ActionId::GoFirstImageAction:
        if (m_documentSession != nullptr) {
            m_documentSession->openFirstActiveNavigation();
        }
        return;
    case Actions::ActionId::GoLastImageAction:
        if (m_documentSession != nullptr) {
            m_documentSession->openLastActiveNavigation();
        }
        return;
    case Actions::ActionId::ViewZoomInAction:
        if (image != nullptr) {
            image->requestZoomByStepAtCenter(1.0);
        }
        return;
    case Actions::ActionId::ViewZoomOutAction:
        if (image != nullptr) {
            image->requestZoomByStepAtCenter(-1.0);
        }
        return;
    case Actions::ActionId::ViewZoom50PercentAction:
        if (image != nullptr) {
            image->requestManualZoomPercent(50.0);
        }
        return;
    case Actions::ActionId::ViewZoom100PercentAction:
        if (image != nullptr) {
            image->requestManualZoomPercent(100.0);
        }
        return;
    case Actions::ActionId::ViewZoom200PercentAction:
        if (image != nullptr) {
            image->requestManualZoomPercent(200.0);
        }
        return;
    case Actions::ActionId::ViewFitAction:
        if (image != nullptr) {
            image->requestFitMode(KiriImageDocument::ZoomMode::Fit);
        }
        return;
    case Actions::ActionId::ViewFitHeightAction:
        if (image != nullptr) {
            image->requestFitMode(KiriImageDocument::ZoomMode::FitHeight);
        }
        return;
    case Actions::ActionId::ViewFitWidthAction:
        if (image != nullptr) {
            image->requestFitMode(KiriImageDocument::ZoomMode::FitWidth);
        }
        return;
    case Actions::ActionId::ViewRotateClockwiseAction:
        if (image != nullptr) {
            image->rotateClockwise();
        }
        return;
    case Actions::ActionId::ViewRotateCounterclockwiseAction:
        if (image != nullptr) {
            image->rotateCounterclockwise();
        }
        return;
    case Actions::ActionId::ViewToggleTwoPageModeAction:
        if (image != nullptr) {
            image->requestToggleTwoPageMode();
        }
        return;
    case Actions::ActionId::ViewToggleRightToLeftReadingAction:
        if (image != nullptr) {
            image->requestToggleRightToLeftReading();
        }
        return;
    case Actions::ActionId::ViewToggleInfoPanelAction:
        Q_EMIT toggleInfoPanelRequested();
        return;
    case Actions::ActionId::ViewToggleThumbnailPanelAction:
        Q_EMIT toggleThumbnailPanelRequested();
        return;
    case Actions::ActionId::ViewPanTopLeftAction:
        if (image != nullptr) {
            image->requestViewportPanToInitialScanPosition();
        }
        return;
    case Actions::ActionId::ViewPanBottomRightAction:
        if (image != nullptr) {
            image->requestViewportPanToFinalScanPosition();
        }
        return;
    case Actions::ActionId::ViewScanForwardAction:
        handleScanForwardAction();
        return;
    case Actions::ActionId::ViewScanBackwardAction:
        handleScanBackwardAction();
        return;
    case Actions::ActionId::WindowFullscreenAction:
        Q_EMIT toggleFullScreenRequested();
        return;
    case Actions::ActionId::HelpShortcutsAction:
        Q_EMIT shortcutHelpRequested();
        return;
    case Actions::ActionId::OpenApplicationMenuAction:
        Q_EMIT openApplicationMenuRequested();
        return;
    default:
        return;
    }
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
    KiriImageDocument *image = imageDocument();
    const bool viewportMoved = image != nullptr && image->requestViewportScanForward();
    const KiriView::ImageShortcutNavigationPolicy::ScanAction action
        = m_navigationPolicy.scanForwardAction(m_imagePannable, viewportMoved);
    switch (action) {
    case KiriView::ImageShortcutNavigationPolicy::ScanAction::RequestNextActiveNavigationFromScan:
        requestNextActiveNavigationWithBoundary();
        return;
    default:
        return;
    }
}

void KiriViewApplication::handleScanBackwardAction()
{
    KiriImageDocument *image = imageDocument();
    const bool viewportMoved = image != nullptr && image->requestViewportScanBackward();
    const bool imageDocumentPageNavigationActive = m_documentSession != nullptr
        && m_documentSession->activeNavigationBoundaryScope()
            == KiriDocumentSession::ActiveNavigationBoundaryScope::
                ImageDocumentPageNavigationBoundary;
    const KiriView::ImageShortcutNavigationPolicy::ScanAction action
        = m_navigationPolicy.scanBackwardAction(m_imagePannable, viewportMoved,
            imageDocumentPageNavigationActive,
            m_documentSession != nullptr && m_documentSession->atKnownFirstActiveNavigation(),
            m_documentSession != nullptr && m_documentSession->canOpenPreviousActiveNavigation());
    switch (action) {
    case KiriView::ImageShortcutNavigationPolicy::ScanAction::
        RequestPreviousActiveNavigationFromScan:
        requestPreviousActiveNavigationWithBoundary();
        return;
    case KiriView::ImageShortcutNavigationPolicy::ScanAction::OpenPreviousPageFromFinalScanStart:
        if (image != nullptr) {
            image->requestNextDisplayedImageStartToFinalScanPosition();
        }
        requestPreviousActiveNavigationWithBoundary();
        return;
    case KiriView::ImageShortcutNavigationPolicy::ScanAction::ShowFirstImageBoundary:
        Q_EMIT imageBoundaryReached(i18nc("@info:status", "First image"));
        return;
    default:
        return;
    }
}

bool KiriViewApplication::executeHorizontalArrowShortcut(bool leftArrow)
{
    if (videoMode()) {
        if (leftArrow) {
            requestPreviousActiveNavigationWithBoundary();
        } else {
            requestNextActiveNavigationWithBoundary();
        }
        return true;
    }

    KiriImageDocument *image = imageDocument();
    if (image == nullptr) {
        return false;
    }

    const KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction action
        = m_navigationPolicy.horizontalArrowAction(leftArrow, image->viewportHorizontallyPannable(),
            m_imageActionProjection.rightToLeftReadingActive);
    switch (action) {
    case KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction::PanLeft:
        image->requestViewportPanBy(-keyboardPanDistance, 0.0);
        return true;
    case KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction::PanRight:
        image->requestViewportPanBy(keyboardPanDistance, 0.0);
        return true;
    case KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction::
        RequestPreviousActiveNavigation:
        requestPreviousActiveNavigationWithBoundary();
        return true;
    case KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction::
        RequestNextActiveNavigation:
        requestNextActiveNavigationWithBoundary();
        return true;
    }

    return false;
}

bool KiriViewApplication::executeSinglePageArrowShortcut(bool leftArrow)
{
    KiriImageDocument *image = imageDocument();
    if (image == nullptr) {
        return false;
    }

    const KiriView::ImageShortcutNavigationPolicy::SinglePageArrowAction action
        = m_navigationPolicy.singlePageArrowAction(
            leftArrow, m_imageActionProjection.rightToLeftReadingActive);
    switch (action) {
    case KiriView::ImageShortcutNavigationPolicy::SinglePageArrowAction::OpenPreviousSinglePage:
        image->openPreviousSinglePage();
        return true;
    case KiriView::ImageShortcutNavigationPolicy::SinglePageArrowAction::OpenNextSinglePage:
        image->openNextSinglePage();
        return true;
    }

    return false;
}

bool KiriViewApplication::executeVerticalPanShortcut(bool up)
{
    KiriImageDocument *image = imageDocument();
    if (image == nullptr) {
        return false;
    }

    image->requestViewportPanBy(0.0, up ? -keyboardPanDistance : keyboardPanDistance);
    return true;
}

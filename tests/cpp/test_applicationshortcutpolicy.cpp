// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationshortcutpolicy.h"
#include "application/kiriviewapplicationactions.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <QVariantList>
#include <cstddef>
#include <optional>

namespace {
using ActionId = kiriview::ApplicationActions::ActionId;
using Category = kiriview::ApplicationActions::ShortcutHelpCategory;
using ActivationScope = kiriview::ApplicationActions::ApplicationShortcutActivationScope;
using Scope = kiriview::ApplicationActions::ImageShortcutScope;

QKeySequence shortcut(const QString& sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
}

QString nativeText(const QKeySequence& sequence)
{
    return sequence.toString(QKeySequence::NativeText);
}

QVariantList actionIdVariants(const QList<ActionId>& actionIds)
{
    QVariantList variants;
    variants.reserve(actionIds.size());

    for (ActionId actionId : actionIds) {
        variants.push_back(static_cast<int>(actionId));
    }

    return variants;
}

QList<kiriview::ApplicationActions::ShortcutRouteSpec> routeSpecsFor(ActionId actionId)
{
    QList<kiriview::ApplicationActions::ShortcutRouteSpec> specs;
    const kiriview::ApplicationActions::ActionDefinition* definition
        = kiriview::ApplicationActions::definitionForId(actionId);
    if (definition == nullptr) {
        return specs;
    }

    for (std::size_t index = 0; index < definition->shortcutRoutes.count; ++index) {
        specs.push_back(definition->shortcutRoutes.specs[index]);
    }

    return specs;
}

bool hasRouteSpec(ActionId actionId, ActivationScope activationScope, Scope scope)
{
    for (const kiriview::ApplicationActions::ShortcutRouteSpec& spec : routeSpecsFor(actionId)) {
        if (spec.activationScope == activationScope && spec.shortcutScope == scope) {
            return true;
        }
    }

    return false;
}

const kiriview::ApplicationActions::ApplicationShortcutRoute* routeFor(
    ActivationScope activationScope, Scope scope)
{
    for (const kiriview::ApplicationActions::ApplicationShortcutRoute& route :
        kiriview::ApplicationActions::shortcutRoutes()) {
        if (route.activationScope == activationScope && route.shortcutScope == scope) {
            return &route;
        }
    }

    return nullptr;
}
}

class TestApplicationShortcutPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void programWideSanitizationKeepsTextInputSafeShortcuts();
    void menuShortcutSkipsViewerLocalShortcuts();
    void shortcutListTextJoinsAssignedShortcuts();
    void shortcutProjectionSeparatesActivationScopes();
    void sanitizeProgramWideShortcutsRemovesUnmodifiedTextInputShortcuts();
    void actionDefinitionsOwnApplicationShortcutRoutes();
    void actionDefinitionsOwnShortcutHelpCategories();
    void shortcutRoutesGroupDefinitionOwnedSpecs();
    void shortcutScopeValuesMapOnlyKnownScopes();
    void unknownShortcutScopesDisableAvailabilityPolicies();
    void videoShortcutScopesUseViewerDeletionAndNavigationGates();
    void videoUnsupportedActionPolicyRejectsImageOnlyCommands();
    void imageUnsupportedActionPolicyRejectsVideoOnlyCommands();
    void horizontalArrowShortcutPolicyUsesActiveMediaMode();
    void fixedShortcutDispatchPlansVideoSeek();
    void fixedShortcutDispatchPlansImageNavigationAndPan();
    void focusInapplicableBlocksFixedShortcutDispatch();
    void genericShortcutDispatchUsesFirstEnabledBinding();
    void genericShortcutDispatchReportsUnsupportedMediaActions();
    void focusInapplicableBlocksGenericShortcutDispatch();
};

void TestApplicationShortcutPolicy::programWideSanitizationKeepsTextInputSafeShortcuts()
{
    const QList<QKeySequence> shortcuts {
        shortcut(QStringLiteral("Q")),
        shortcut(QStringLiteral("Shift+Q")),
        shortcut(QStringLiteral("Ctrl+Q")),
        shortcut(QStringLiteral("Delete")),
    };

    QCOMPARE(kiriview::ApplicationActions::sanitizeProgramWideShortcuts(shortcuts),
        QList<QKeySequence>({ shortcut(QStringLiteral("Shift+Q")),
            shortcut(QStringLiteral("Ctrl+Q")), shortcut(QStringLiteral("Delete")) }));
}

void TestApplicationShortcutPolicy::menuShortcutSkipsViewerLocalShortcuts()
{
    const QList<QKeySequence> shortcuts {
        QKeySequence(),
        shortcut(QStringLiteral("Delete")),
        shortcut(QStringLiteral("Q")),
        shortcut(QStringLiteral("Alt+O")),
        shortcut(QStringLiteral("Ctrl+Shift+O")),
    };

    QCOMPARE(
        kiriview::ApplicationActions::menuShortcut(shortcuts), shortcut(QStringLiteral("Alt+O")));
    QVERIFY(
        kiriview::ApplicationActions::menuShortcut({ shortcut(QStringLiteral("Home")) }).isEmpty());
    QVERIFY(kiriview::ApplicationActions::menuShortcut({ shortcut(QStringLiteral("Ctrl+X, Q")) })
            .isEmpty());
}

void TestApplicationShortcutPolicy::shortcutListTextJoinsAssignedShortcuts()
{
    QCOMPARE(kiriview::ApplicationActions::shortcutListText({ QKeySequence() }), QString());
    QCOMPARE(kiriview::ApplicationActions::shortcutListText({ QKeySequence(),
                 shortcut(QStringLiteral("Alt+O")), shortcut(QStringLiteral("Ctrl+Shift+O")) }),
        QStringLiteral("%1 / %2").arg(nativeText(shortcut(QStringLiteral("Alt+O"))),
            nativeText(shortcut(QStringLiteral("Ctrl+Shift+O")))));
}

void TestApplicationShortcutPolicy::shortcutProjectionSeparatesActivationScopes()
{
    const QList<QKeySequence> programWideShortcuts { shortcut(QStringLiteral("Ctrl+R")) };
    const QList<QKeySequence> viewerLocalShortcuts {
        shortcut(QStringLiteral("R")),
        shortcut(QStringLiteral("Shift+R")),
    };

    const kiriview::ApplicationActions::ApplicationShortcutProjection projection
        = kiriview::ApplicationActions::shortcutProjection(
            programWideShortcuts, viewerLocalShortcuts);

    QCOMPARE(projection.shortcuts,
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+R")), shortcut(QStringLiteral("R")),
            shortcut(QStringLiteral("Shift+R")) }));
    QCOMPARE(projection.programWideShortcuts, programWideShortcuts);
    QCOMPARE(projection.viewerLocalShortcuts, viewerLocalShortcuts);
    QCOMPARE(projection.menuShortcut, shortcut(QStringLiteral("Ctrl+R")));
    QCOMPARE(projection.shortcutText,
        QStringLiteral("%1 / %2 / %3")
            .arg(nativeText(shortcut(QStringLiteral("Ctrl+R"))),
                nativeText(shortcut(QStringLiteral("R"))),
                nativeText(shortcut(QStringLiteral("Shift+R")))));
    QCOMPARE(projection.menuShortcutText, nativeText(shortcut(QStringLiteral("Ctrl+R"))));
}

void TestApplicationShortcutPolicy::
    sanitizeProgramWideShortcutsRemovesUnmodifiedTextInputShortcuts()
{
    const QList<QKeySequence> shortcuts {
        shortcut(QStringLiteral("Q")),
        shortcut(QStringLiteral("Shift+Q")),
        shortcut(QStringLiteral("Ctrl+Q")),
        shortcut(QStringLiteral("Delete")),
    };

    QCOMPARE(kiriview::ApplicationActions::sanitizeProgramWideShortcuts(shortcuts),
        QList<QKeySequence>({ shortcut(QStringLiteral("Shift+Q")),
            shortcut(QStringLiteral("Ctrl+Q")), shortcut(QStringLiteral("Delete")) }));
}

void TestApplicationShortcutPolicy::actionDefinitionsOwnApplicationShortcutRoutes()
{
    QVERIFY(hasRouteSpec(
        ActionId::FileOpenAction, ActivationScope::ProgramWide, Scope::HelpShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::FileMoveToTrashAction, ActivationScope::ViewerLocal,
        Scope::ReadyViewerShortcutScope));
    QVERIFY(hasRouteSpec(
        ActionId::ViewZoomInAction, ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::ViewToggleTwoPageModeAction, ActivationScope::ViewerLocal,
        Scope::ReadyViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::ViewToggleRightToLeftReadingAction, ActivationScope::ViewerLocal,
        Scope::RightToLeftReadingViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::ViewToggleInfoPanelAction, ActivationScope::ViewerLocal,
        Scope::ViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::ViewToggleThumbnailPanelAction, ActivationScope::ViewerLocal,
        Scope::ViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::ViewToggleVideoPlaybackAction, ActivationScope::ViewerLocal,
        Scope::ReadyViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::ViewGoToContentStartAction, ActivationScope::ViewerLocal,
        Scope::MediaStartEndViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::ViewGoToContentEndAction, ActivationScope::ViewerLocal,
        Scope::MediaStartEndViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::GoPreviousImageAction, ActivationScope::ViewerLocal,
        Scope::ImageSelectionViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::GoFirstImageAction, ActivationScope::ViewerLocal,
        Scope::PageViewerShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::GoPreviousArchiveAction, ActivationScope::ViewerLocal,
        Scope::ContainerViewerShortcutScope));
    QVERIFY(hasRouteSpec(
        ActionId::HelpShortcutsAction, ActivationScope::ViewerLocal, Scope::ViewerShortcutScope));
    QVERIFY(hasRouteSpec(
        ActionId::HelpShortcutsAction, ActivationScope::ProgramWide, Scope::HelpShortcutScope));
    QVERIFY(hasRouteSpec(ActionId::OptionsConfigureKeybindingAction, ActivationScope::ProgramWide,
        Scope::HelpShortcutScope));
    QVERIFY(hasRouteSpec(
        ActionId::FileQuitAction, ActivationScope::ProgramWide, Scope::HelpShortcutScope));
    QVERIFY(hasRouteSpec(
        ActionId::FileQuitAction, ActivationScope::ViewerLocal, Scope::ViewerShortcutScope));
}

void TestApplicationShortcutPolicy::actionDefinitionsOwnShortcutHelpCategories()
{
    const auto categoryFor = [](ActionId actionId) {
        const kiriview::ApplicationActions::ActionDefinition* definition
            = kiriview::ApplicationActions::definitionForId(actionId);
        Q_ASSERT(definition != nullptr);
        return definition->shortcutHelpCategory;
    };

    QCOMPARE(categoryFor(ActionId::FileOpenAction), Category::File);
    QCOMPARE(categoryFor(ActionId::FileQuitAction), Category::File);
    QCOMPARE(categoryFor(ActionId::GoPreviousImageAction), Category::Navigation);
    QCOMPARE(categoryFor(ActionId::ViewZoomInAction), Category::View);
    QCOMPARE(categoryFor(ActionId::ViewToggleInfoPanelAction), Category::Panels);
    QCOMPARE(categoryFor(ActionId::WindowFullscreenAction), Category::Window);
    QCOMPARE(categoryFor(ActionId::OptionsConfigureKeybindingAction), Category::Settings);
    QCOMPARE(categoryFor(ActionId::OptionsShowMenubarAction), Category::Settings);
    QCOMPARE(categoryFor(ActionId::HelpShortcutsAction), Category::Help);
    QCOMPARE(categoryFor(ActionId::OpenApplicationMenuAction), Category::Help);

    const QList<Category> orderedCategories {
        Category::File,
        Category::Navigation,
        Category::View,
        Category::Panels,
        Category::Window,
        Category::Settings,
        Category::Help,
    };
    const QStringList orderedKeys {
        QStringLiteral("file"),
        QStringLiteral("navigation"),
        QStringLiteral("view"),
        QStringLiteral("panels"),
        QStringLiteral("window"),
        QStringLiteral("settings"),
        QStringLiteral("help"),
    };

    for (int index = 0; index < orderedCategories.size(); ++index) {
        QCOMPARE(
            kiriview::ApplicationActions::shortcutHelpCategoryOrder(orderedCategories.at(index)),
            index);
        QCOMPARE(kiriview::ApplicationActions::shortcutHelpCategoryKey(orderedCategories.at(index)),
            orderedKeys.at(index));
    }
}

void TestApplicationShortcutPolicy::shortcutRoutesGroupDefinitionOwnedSpecs()
{
    const kiriview::ApplicationActions::ApplicationShortcutRoute* readyRoute
        = routeFor(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope);
    QVERIFY(readyRoute != nullptr);
    QCOMPARE(actionIdVariants(readyRoute->actionIds),
        actionIdVariants({ ActionId::FileMoveToTrashAction, ActionId::FileDeleteAction,
            ActionId::ViewZoomInAction, ActionId::ViewZoomOutAction,
            ActionId::ViewZoom50PercentAction, ActionId::ViewZoom100PercentAction,
            ActionId::ViewZoom200PercentAction, ActionId::ViewFitAction,
            ActionId::ViewFitHeightAction, ActionId::ViewFitWidthAction,
            ActionId::ViewToggleTwoPageModeAction, ActionId::ViewScanForwardAction,
            ActionId::ViewScanBackwardAction, ActionId::ViewToggleVideoPlaybackAction }));

    const kiriview::ApplicationActions::ApplicationShortcutRoute* containerRoute
        = routeFor(ActivationScope::ViewerLocal, Scope::ContainerViewerShortcutScope);
    QVERIFY(containerRoute != nullptr);
    QCOMPARE(actionIdVariants(containerRoute->actionIds),
        actionIdVariants({ ActionId::GoPreviousArchiveAction, ActionId::GoNextArchiveAction }));

    const kiriview::ApplicationActions::ApplicationShortcutRoute* viewerLocalRoute
        = routeFor(ActivationScope::ViewerLocal, Scope::ViewerShortcutScope);
    QVERIFY(viewerLocalRoute != nullptr);
    QVERIFY(viewerLocalRoute->actionIds.contains(ActionId::ViewToggleInfoPanelAction));
    QVERIFY(viewerLocalRoute->actionIds.contains(ActionId::ViewToggleThumbnailPanelAction));
    QVERIFY(viewerLocalRoute->actionIds.contains(ActionId::FileQuitAction));

    for (const kiriview::ApplicationActions::ApplicationShortcutRoute& route :
        kiriview::ApplicationActions::shortcutRoutes()) {
        QVERIFY(!route.actionIds.isEmpty());
        for (ActionId actionId : route.actionIds) {
            QVERIFY(kiriview::ApplicationActions::definitionForId(actionId) != nullptr);
            QVERIFY(hasRouteSpec(actionId, route.activationScope, route.shortcutScope));
        }
    }

    for (const kiriview::ApplicationActions::ActionDefinition& definition :
        kiriview::ApplicationActions::definitions()) {
        for (const kiriview::ApplicationActions::ShortcutRouteSpec& spec :
            routeSpecsFor(definition.actionId)) {
            const kiriview::ApplicationActions::ApplicationShortcutRoute* route
                = routeFor(spec.activationScope, spec.shortcutScope);
            QVERIFY(route != nullptr);
            QVERIFY(route->actionIds.contains(definition.actionId));
        }
    }
}

void TestApplicationShortcutPolicy::shortcutScopeValuesMapOnlyKnownScopes()
{
    const std::optional<Scope> readyViewerScope
        = kiriview::ApplicationActions::imageShortcutScopeFromValue(
            static_cast<int>(Scope::ReadyViewerShortcutScope));
    QVERIFY(readyViewerScope.has_value());
    QCOMPARE(*readyViewerScope, Scope::ReadyViewerShortcutScope);

    const std::optional<Scope> containerViewerScope
        = kiriview::ApplicationActions::imageShortcutScopeFromValue(
            static_cast<int>(Scope::ContainerViewerShortcutScope));
    QVERIFY(containerViewerScope.has_value());
    QCOMPARE(*containerViewerScope, Scope::ContainerViewerShortcutScope);

    QVERIFY(!kiriview::ApplicationActions::imageShortcutScopeFromValue(-1).has_value());
    QVERIFY(!kiriview::ApplicationActions::imageShortcutScopeFromValue(999).has_value());
}

void TestApplicationShortcutPolicy::unknownShortcutScopesDisableAvailabilityPolicies()
{
    const Scope unknownScope = static_cast<Scope>(999);

    ImageActionAvailabilityProjection imageProjection;
    imageProjection.helpShortcutsEnabled = true;
    imageProjection.viewerShortcutsEnabled = true;
    imageProjection.readyShortcutsEnabled = true;
    imageProjection.readyViewerShortcutsEnabled = true;
    imageProjection.containerShortcutsEnabled = true;
    imageProjection.containerViewerShortcutsEnabled = true;

    QVERIFY(!imageActionAvailabilityShortcutsEnabledForScope(imageProjection, unknownScope));

    ActiveMediaShortcutAvailabilityInput activeMediaInput;
    activeMediaInput.imageProjection = imageProjection;
    activeMediaInput.videoMode = true;
    activeMediaInput.activeNavigationActionsAvailable = true;

    QVERIFY(!activeMediaShortcutsEnabledForScope(activeMediaInput, unknownScope));

    kiriview::ApplicationActions::VideoShortcutAvailabilityInput videoInput;
    videoInput.helpShortcutsEnabled = true;
    videoInput.viewerShortcutsEnabled = true;
    videoInput.directMediaNavigationActive = true;

    QVERIFY(!kiriview::ApplicationActions::videoShortcutsEnabledForScope(videoInput, unknownScope));
}

void TestApplicationShortcutPolicy::videoShortcutScopesUseViewerDeletionAndNavigationGates()
{
    kiriview::ApplicationActions::VideoShortcutAvailabilityInput input;
    input.helpShortcutsEnabled = true;
    input.viewerShortcutsEnabled = true;
    input.directMediaNavigationActive = true;

    QVERIFY(kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::HelpShortcutScope));
    QVERIFY(kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ViewerShortcutScope));
    QVERIFY(kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyShortcutScope));
    QVERIFY(kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyViewerShortcutScope));
    QVERIFY(kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ImageSelectionShortcutScope));
    QVERIFY(kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ImageSelectionViewerShortcutScope));
    QVERIFY(kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::MediaStartEndViewerShortcutScope));

    input.viewerShortcutsEnabled = false;
    QVERIFY(kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyShortcutScope));
    QVERIFY(!kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyViewerShortcutScope));
    QVERIFY(!kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ImageSelectionViewerShortcutScope));
    QVERIFY(!kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::MediaStartEndViewerShortcutScope));

    input.viewerShortcutsEnabled = true;
    input.directMediaNavigationActive = false;
    QVERIFY(!kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ImageSelectionShortcutScope));
    QVERIFY(!kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::PageViewerShortcutScope));

    input.fileDeletionInProgress = true;
    QVERIFY(kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::HelpShortcutScope));
    QVERIFY(!kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyShortcutScope));
    QVERIFY(!kiriview::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ContainerViewerShortcutScope));
}

void TestApplicationShortcutPolicy::videoUnsupportedActionPolicyRejectsImageOnlyCommands()
{
    QVERIFY(
        kiriview::ApplicationActions::videoActionUnsupported(ActionId::ViewRotateClockwiseAction));
    QVERIFY(kiriview::ApplicationActions::videoActionUnsupported(ActionId::ViewZoomInAction));
    QVERIFY(
        kiriview::ApplicationActions::videoActionUnsupported(ActionId::ViewZoom50PercentAction));
    QVERIFY(
        kiriview::ApplicationActions::videoActionUnsupported(ActionId::ViewZoom100PercentAction));
    QVERIFY(
        kiriview::ApplicationActions::videoActionUnsupported(ActionId::ViewZoom200PercentAction));
    QVERIFY(kiriview::ApplicationActions::videoActionUnsupported(
        ActionId::ViewToggleTwoPageModeAction));
    QVERIFY(
        kiriview::ApplicationActions::videoActionUnsupported(ActionId::GoPreviousArchiveAction));
    QVERIFY(!kiriview::ApplicationActions::videoActionUnsupported(ActionId::ViewScanForwardAction));
    QVERIFY(
        !kiriview::ApplicationActions::videoActionUnsupported(ActionId::ViewScanBackwardAction));
    QVERIFY(!kiriview::ApplicationActions::videoActionUnsupported(
        ActionId::ViewGoToContentStartAction));
    QVERIFY(
        !kiriview::ApplicationActions::videoActionUnsupported(ActionId::ViewGoToContentEndAction));

    QVERIFY(
        !kiriview::ApplicationActions::videoActionUnsupported(ActionId::WindowFullscreenAction));
    QVERIFY(!kiriview::ApplicationActions::videoActionUnsupported(ActionId::HelpShortcutsAction));
    QVERIFY(!kiriview::ApplicationActions::videoActionUnsupported(ActionId::FileOpenAction));
    QVERIFY(!kiriview::ApplicationActions::videoActionUnsupported(ActionId::GoPreviousImageAction));
    QVERIFY(!kiriview::ApplicationActions::videoActionUnsupported(ActionId::FileDeleteAction));
    QVERIFY(!kiriview::ApplicationActions::videoActionUnsupported(
        ActionId::ViewToggleVideoPlaybackAction));
}

void TestApplicationShortcutPolicy::imageUnsupportedActionPolicyRejectsVideoOnlyCommands()
{
    QVERIFY(kiriview::ApplicationActions::imageActionUnsupported(
        ActionId::ViewToggleVideoPlaybackAction));
    QVERIFY(!kiriview::ApplicationActions::imageActionUnsupported(ActionId::ViewZoomInAction));
    QVERIFY(
        !kiriview::ApplicationActions::imageActionUnsupported(ActionId::ViewRotateClockwiseAction));
    QVERIFY(
        !kiriview::ApplicationActions::imageActionUnsupported(ActionId::WindowFullscreenAction));
    QVERIFY(!kiriview::ApplicationActions::imageActionUnsupported(ActionId::FileOpenAction));
}

void TestApplicationShortcutPolicy::horizontalArrowShortcutPolicyUsesActiveMediaMode()
{
    kiriview::ApplicationActions::VideoShortcutAvailabilityInput input;
    input.viewerShortcutsEnabled = true;
    input.directMediaNavigationActive = true;

    QVERIFY(kiriview::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(false, true, input));
    QVERIFY(
        !kiriview::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(false, false, input));
    QVERIFY(kiriview::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(true, false, input));

    input.viewerShortcutsEnabled = false;
    QVERIFY(!kiriview::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(true, true, input));

    input.viewerShortcutsEnabled = true;
    input.fileDeletionInProgress = true;
    QVERIFY(!kiriview::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(true, true, input));

    input.fileDeletionInProgress = false;
    input.directMediaNavigationActive = false;
    QVERIFY(!kiriview::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(true, true, input));
}

void TestApplicationShortcutPolicy::fixedShortcutDispatchPlansVideoSeek()
{
    kiriview::ApplicationActions::FixedShortcutDispatchInput input;
    input.videoMode = true;
    input.helpActionsEnabled = true;
    input.viewerShortcutsEnabled = true;
    input.readyViewerShortcutsEnabled = true;
    input.activeNavigationActionsAvailable = true;

    const kiriview::ApplicationActions::FixedShortcutDispatchOutcome leftSeek
        = kiriview::ApplicationActions::fixedShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Alt+Left")));
    QCOMPARE(leftSeek.kind, kiriview::ApplicationActions::FixedShortcutDispatchKind::VideoSeek);
    QCOMPARE(leftSeek.videoSeekDeltaMilliseconds, qint64(-5000));

    const kiriview::ApplicationActions::FixedShortcutDispatchOutcome downSeek
        = kiriview::ApplicationActions::fixedShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Alt+Down")));
    QCOMPARE(downSeek.kind, kiriview::ApplicationActions::FixedShortcutDispatchKind::VideoSeek);
    QCOMPARE(downSeek.videoSeekDeltaMilliseconds, qint64(-45000));

    input.videoFileDeletionInProgress = true;
    const kiriview::ApplicationActions::FixedShortcutDispatchOutcome blocked
        = kiriview::ApplicationActions::fixedShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Alt+Right")));
    QCOMPARE(blocked.kind, kiriview::ApplicationActions::FixedShortcutDispatchKind::None);
}

void TestApplicationShortcutPolicy::fixedShortcutDispatchPlansImageNavigationAndPan()
{
    kiriview::ApplicationActions::FixedShortcutDispatchInput input;
    input.helpActionsEnabled = true;
    input.viewerShortcutsEnabled = true;
    input.readyViewerShortcutsEnabled = true;
    input.twoPageViewerShortcutsEnabled = true;
    input.pannableViewerShortcutsEnabled = true;

    const kiriview::ApplicationActions::FixedShortcutDispatchOutcome left
        = kiriview::ApplicationActions::fixedShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Left")));
    QCOMPARE(left.kind, kiriview::ApplicationActions::FixedShortcutDispatchKind::HorizontalArrow);
    QVERIFY(left.previousOrUp);

    const kiriview::ApplicationActions::FixedShortcutDispatchOutcome nextPage
        = kiriview::ApplicationActions::fixedShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Shift+Right")));
    QCOMPARE(
        nextPage.kind, kiriview::ApplicationActions::FixedShortcutDispatchKind::SinglePageArrow);
    QVERIFY(!nextPage.previousOrUp);

    const kiriview::ApplicationActions::FixedShortcutDispatchOutcome panUp
        = kiriview::ApplicationActions::fixedShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Up")));
    QCOMPARE(panUp.kind, kiriview::ApplicationActions::FixedShortcutDispatchKind::VerticalPan);
    QVERIFY(panUp.previousOrUp);

    input.readyViewerShortcutsEnabled = false;
    const kiriview::ApplicationActions::FixedShortcutDispatchOutcome blockedHorizontal
        = kiriview::ApplicationActions::fixedShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Left")));
    QCOMPARE(blockedHorizontal.kind, kiriview::ApplicationActions::FixedShortcutDispatchKind::None);
}

void TestApplicationShortcutPolicy::focusInapplicableBlocksFixedShortcutDispatch()
{
    kiriview::ApplicationActions::FixedShortcutDispatchInput input;
    input.focusApplicable = false;
    input.helpActionsEnabled = true;
    input.viewerShortcutsEnabled = true;
    input.readyViewerShortcutsEnabled = true;
    input.activeNavigationActionsAvailable = true;

    const kiriview::ApplicationActions::FixedShortcutDispatchOutcome horizontal
        = kiriview::ApplicationActions::fixedShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Left")));
    QCOMPARE(horizontal.kind, kiriview::ApplicationActions::FixedShortcutDispatchKind::None);

    input.videoMode = true;
    const kiriview::ApplicationActions::FixedShortcutDispatchOutcome seek
        = kiriview::ApplicationActions::fixedShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Alt+Right")));
    QCOMPARE(seek.kind, kiriview::ApplicationActions::FixedShortcutDispatchKind::None);
}

void TestApplicationShortcutPolicy::genericShortcutDispatchUsesFirstEnabledBinding()
{
    kiriview::ApplicationActions::GenericShortcutDispatchInput input;
    input.actionState.helpActionsEnabled = true;
    input.actionState.readyViewerShortcutsEnabled = true;
    input.actionState.applicationMenuShortcutEnabled = true;
    input.actionState.showMenubarActionEnabled = true;
    input.bindings = {
        kiriview::ApplicationActions::GenericShortcutBinding {
            ActionId::GoNextImageAction,
            Scope::ReadyViewerShortcutScope,
            { shortcut(QStringLiteral("N")) },
            false,
        },
        kiriview::ApplicationActions::GenericShortcutBinding {
            ActionId::FileOpenAction,
            std::nullopt,
            { shortcut(QStringLiteral("N")) },
            true,
        },
    };

    kiriview::ApplicationActions::GenericShortcutDispatchOutcome outcome
        = kiriview::ApplicationActions::genericShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("N")));
    QCOMPARE(
        outcome.kind, kiriview::ApplicationActions::GenericShortcutDispatchKind::TriggerAction);
    QCOMPARE(outcome.actionId, ActionId::FileOpenAction);

    input.bindings.front().actionEnabled = true;
    outcome = kiriview::ApplicationActions::genericShortcutDispatchOutcome(
        input, shortcut(QStringLiteral("N")));
    QCOMPARE(
        outcome.kind, kiriview::ApplicationActions::GenericShortcutDispatchKind::TriggerAction);
    QCOMPARE(outcome.actionId, ActionId::GoNextImageAction);

    input.actionState.readyViewerShortcutsEnabled = false;
    outcome = kiriview::ApplicationActions::genericShortcutDispatchOutcome(
        input, shortcut(QStringLiteral("N")));
    QCOMPARE(
        outcome.kind, kiriview::ApplicationActions::GenericShortcutDispatchKind::TriggerAction);
    QCOMPARE(outcome.actionId, ActionId::FileOpenAction);
}

void TestApplicationShortcutPolicy::genericShortcutDispatchReportsUnsupportedMediaActions()
{
    kiriview::ApplicationActions::GenericShortcutDispatchInput input;
    input.actionState.videoMode = true;
    input.actionState.helpActionsEnabled = true;
    input.actionState.viewerShortcutsEnabled = true;
    input.actionState.readyViewerShortcutsEnabled = true;
    input.actionState.activeNavigationActionsAvailable = true;
    input.bindings = {
        kiriview::ApplicationActions::GenericShortcutBinding {
            ActionId::ViewZoomInAction,
            Scope::ReadyViewerShortcutScope,
            { shortcut(QStringLiteral("Z")) },
            false,
        },
    };

    kiriview::ApplicationActions::GenericShortcutDispatchOutcome outcome
        = kiriview::ApplicationActions::genericShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("Z")));
    QCOMPARE(outcome.kind,
        kiriview::ApplicationActions::GenericShortcutDispatchKind::UnsupportedVideoAction);
    QCOMPARE(outcome.actionId, ActionId::ViewZoomInAction);

    input.actionState.videoMode = false;
    input.bindings.front().actionId = ActionId::ViewToggleVideoPlaybackAction;
    outcome = kiriview::ApplicationActions::genericShortcutDispatchOutcome(
        input, shortcut(QStringLiteral("Z")));
    QCOMPARE(outcome.kind,
        kiriview::ApplicationActions::GenericShortcutDispatchKind::UnsupportedImageAction);
    QCOMPARE(outcome.actionId, ActionId::ViewToggleVideoPlaybackAction);
}

void TestApplicationShortcutPolicy::focusInapplicableBlocksGenericShortcutDispatch()
{
    kiriview::ApplicationActions::GenericShortcutDispatchInput input;
    input.focusApplicable = false;
    input.actionState.helpActionsEnabled = true;
    input.actionState.readyViewerShortcutsEnabled = true;
    input.bindings = {
        kiriview::ApplicationActions::GenericShortcutBinding {
            ActionId::GoNextImageAction,
            Scope::ReadyViewerShortcutScope,
            { shortcut(QStringLiteral("N")) },
            true,
        },
    };

    const kiriview::ApplicationActions::GenericShortcutDispatchOutcome outcome
        = kiriview::ApplicationActions::genericShortcutDispatchOutcome(
            input, shortcut(QStringLiteral("N")));
    QCOMPARE(outcome.kind, kiriview::ApplicationActions::GenericShortcutDispatchKind::None);
}

QTEST_GUILESS_MAIN(TestApplicationShortcutPolicy)

#include "test_applicationshortcutpolicy.moc"

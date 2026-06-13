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
using ActionId = KiriView::ApplicationActions::ActionId;
using Category = KiriView::ApplicationActions::ShortcutHelpCategory;
using ActivationScope = KiriView::ApplicationActions::ApplicationShortcutActivationScope;
using Scope = KiriView::ApplicationActions::ImageShortcutScope;

QKeySequence shortcut(const QString &sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
}

QString nativeText(const QKeySequence &sequence)
{
    return sequence.toString(QKeySequence::NativeText);
}

QVariantList actionIdVariants(const QList<ActionId> &actionIds)
{
    QVariantList variants;
    variants.reserve(actionIds.size());

    for (ActionId actionId : actionIds) {
        variants.push_back(static_cast<int>(actionId));
    }

    return variants;
}

QList<KiriView::ApplicationActions::ShortcutRouteSpec> routeSpecsFor(ActionId actionId)
{
    QList<KiriView::ApplicationActions::ShortcutRouteSpec> specs;
    const KiriView::ApplicationActions::ActionDefinition *definition
        = KiriView::ApplicationActions::definitionForId(actionId);
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
    for (const KiriView::ApplicationActions::ShortcutRouteSpec &spec : routeSpecsFor(actionId)) {
        if (spec.activationScope == activationScope && spec.shortcutScope == scope) {
            return true;
        }
    }

    return false;
}

const KiriView::ApplicationActions::ApplicationShortcutRoute *routeFor(
    ActivationScope activationScope, Scope scope)
{
    for (const KiriView::ApplicationActions::ApplicationShortcutRoute &route :
        KiriView::ApplicationActions::shortcutRoutes()) {
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
    void videoShortcutScopesUseViewerDeletionAndNavigationGates();
    void videoUnsupportedActionPolicyRejectsImageOnlyCommands();
    void imageUnsupportedActionPolicyRejectsVideoOnlyCommands();
    void horizontalArrowShortcutPolicyUsesActiveMediaMode();
};

void TestApplicationShortcutPolicy::programWideSanitizationKeepsTextInputSafeShortcuts()
{
    const QList<QKeySequence> shortcuts {
        shortcut(QStringLiteral("Q")),
        shortcut(QStringLiteral("Shift+Q")),
        shortcut(QStringLiteral("Ctrl+Q")),
        shortcut(QStringLiteral("Delete")),
    };

    QCOMPARE(KiriView::ApplicationActions::sanitizeProgramWideShortcuts(shortcuts),
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
        KiriView::ApplicationActions::menuShortcut(shortcuts), shortcut(QStringLiteral("Alt+O")));
    QVERIFY(
        KiriView::ApplicationActions::menuShortcut({ shortcut(QStringLiteral("Home")) }).isEmpty());
    QVERIFY(KiriView::ApplicationActions::menuShortcut({ shortcut(QStringLiteral("Ctrl+X, Q")) })
            .isEmpty());
}

void TestApplicationShortcutPolicy::shortcutListTextJoinsAssignedShortcuts()
{
    QCOMPARE(KiriView::ApplicationActions::shortcutListText({ QKeySequence() }), QString());
    QCOMPARE(KiriView::ApplicationActions::shortcutListText({ QKeySequence(),
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

    const KiriView::ApplicationActions::ApplicationShortcutProjection projection
        = KiriView::ApplicationActions::shortcutProjection(
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

    QCOMPARE(KiriView::ApplicationActions::sanitizeProgramWideShortcuts(shortcuts),
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
}

void TestApplicationShortcutPolicy::actionDefinitionsOwnShortcutHelpCategories()
{
    const auto categoryFor = [](ActionId actionId) {
        const KiriView::ApplicationActions::ActionDefinition *definition
            = KiriView::ApplicationActions::definitionForId(actionId);
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
            KiriView::ApplicationActions::shortcutHelpCategoryOrder(orderedCategories.at(index)),
            index);
        QCOMPARE(KiriView::ApplicationActions::shortcutHelpCategoryKey(orderedCategories.at(index)),
            orderedKeys.at(index));
    }
}

void TestApplicationShortcutPolicy::shortcutRoutesGroupDefinitionOwnedSpecs()
{
    const KiriView::ApplicationActions::ApplicationShortcutRoute *readyRoute
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

    const KiriView::ApplicationActions::ApplicationShortcutRoute *containerRoute
        = routeFor(ActivationScope::ViewerLocal, Scope::ContainerViewerShortcutScope);
    QVERIFY(containerRoute != nullptr);
    QCOMPARE(actionIdVariants(containerRoute->actionIds),
        actionIdVariants({ ActionId::GoPreviousArchiveAction, ActionId::GoNextArchiveAction }));

    const KiriView::ApplicationActions::ApplicationShortcutRoute *viewerLocalRoute
        = routeFor(ActivationScope::ViewerLocal, Scope::ViewerShortcutScope);
    QVERIFY(viewerLocalRoute != nullptr);
    QVERIFY(viewerLocalRoute->actionIds.contains(ActionId::ViewToggleInfoPanelAction));
    QVERIFY(viewerLocalRoute->actionIds.contains(ActionId::ViewToggleThumbnailPanelAction));

    for (const KiriView::ApplicationActions::ApplicationShortcutRoute &route :
        KiriView::ApplicationActions::shortcutRoutes()) {
        QVERIFY(!route.actionIds.isEmpty());
        for (ActionId actionId : route.actionIds) {
            QVERIFY(KiriView::ApplicationActions::definitionForId(actionId) != nullptr);
            QVERIFY(hasRouteSpec(actionId, route.activationScope, route.shortcutScope));
        }
    }

    for (const KiriView::ApplicationActions::ActionDefinition &definition :
        KiriView::ApplicationActions::definitions()) {
        for (const KiriView::ApplicationActions::ShortcutRouteSpec &spec :
            routeSpecsFor(definition.actionId)) {
            const KiriView::ApplicationActions::ApplicationShortcutRoute *route
                = routeFor(spec.activationScope, spec.shortcutScope);
            QVERIFY(route != nullptr);
            QVERIFY(route->actionIds.contains(definition.actionId));
        }
    }
}

void TestApplicationShortcutPolicy::shortcutScopeValuesMapOnlyKnownScopes()
{
    const std::optional<Scope> readyViewerScope
        = KiriView::ApplicationActions::imageShortcutScopeFromValue(
            static_cast<int>(Scope::ReadyViewerShortcutScope));
    QVERIFY(readyViewerScope.has_value());
    QCOMPARE(*readyViewerScope, Scope::ReadyViewerShortcutScope);

    const std::optional<Scope> containerViewerScope
        = KiriView::ApplicationActions::imageShortcutScopeFromValue(
            static_cast<int>(Scope::ContainerViewerShortcutScope));
    QVERIFY(containerViewerScope.has_value());
    QCOMPARE(*containerViewerScope, Scope::ContainerViewerShortcutScope);

    QVERIFY(!KiriView::ApplicationActions::imageShortcutScopeFromValue(-1).has_value());
    QVERIFY(!KiriView::ApplicationActions::imageShortcutScopeFromValue(999).has_value());
}

void TestApplicationShortcutPolicy::videoShortcutScopesUseViewerDeletionAndNavigationGates()
{
    KiriView::ApplicationActions::VideoShortcutAvailabilityInput input;
    input.helpShortcutsEnabled = true;
    input.viewerShortcutsEnabled = true;
    input.directMediaNavigationActive = true;

    QVERIFY(KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::HelpShortcutScope));
    QVERIFY(KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ViewerShortcutScope));
    QVERIFY(KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyShortcutScope));
    QVERIFY(KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyViewerShortcutScope));
    QVERIFY(KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ImageSelectionShortcutScope));
    QVERIFY(KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ImageSelectionViewerShortcutScope));
    QVERIFY(KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::MediaStartEndViewerShortcutScope));

    input.viewerShortcutsEnabled = false;
    QVERIFY(KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyShortcutScope));
    QVERIFY(!KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyViewerShortcutScope));
    QVERIFY(!KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ImageSelectionViewerShortcutScope));
    QVERIFY(!KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::MediaStartEndViewerShortcutScope));

    input.viewerShortcutsEnabled = true;
    input.directMediaNavigationActive = false;
    QVERIFY(!KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ImageSelectionShortcutScope));
    QVERIFY(!KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::PageViewerShortcutScope));

    input.fileDeletionInProgress = true;
    QVERIFY(KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::HelpShortcutScope));
    QVERIFY(!KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ReadyShortcutScope));
    QVERIFY(!KiriView::ApplicationActions::videoShortcutsEnabledForScope(
        input, Scope::ContainerViewerShortcutScope));
}

void TestApplicationShortcutPolicy::videoUnsupportedActionPolicyRejectsImageOnlyCommands()
{
    QVERIFY(
        KiriView::ApplicationActions::videoActionUnsupported(ActionId::ViewRotateClockwiseAction));
    QVERIFY(KiriView::ApplicationActions::videoActionUnsupported(ActionId::ViewZoomInAction));
    QVERIFY(
        KiriView::ApplicationActions::videoActionUnsupported(ActionId::ViewZoom50PercentAction));
    QVERIFY(
        KiriView::ApplicationActions::videoActionUnsupported(ActionId::ViewZoom100PercentAction));
    QVERIFY(
        KiriView::ApplicationActions::videoActionUnsupported(ActionId::ViewZoom200PercentAction));
    QVERIFY(KiriView::ApplicationActions::videoActionUnsupported(
        ActionId::ViewToggleTwoPageModeAction));
    QVERIFY(
        KiriView::ApplicationActions::videoActionUnsupported(ActionId::GoPreviousArchiveAction));
    QVERIFY(!KiriView::ApplicationActions::videoActionUnsupported(ActionId::ViewScanForwardAction));
    QVERIFY(
        !KiriView::ApplicationActions::videoActionUnsupported(ActionId::ViewScanBackwardAction));
    QVERIFY(!KiriView::ApplicationActions::videoActionUnsupported(
        ActionId::ViewGoToContentStartAction));
    QVERIFY(
        !KiriView::ApplicationActions::videoActionUnsupported(ActionId::ViewGoToContentEndAction));

    QVERIFY(
        !KiriView::ApplicationActions::videoActionUnsupported(ActionId::WindowFullscreenAction));
    QVERIFY(!KiriView::ApplicationActions::videoActionUnsupported(ActionId::HelpShortcutsAction));
    QVERIFY(!KiriView::ApplicationActions::videoActionUnsupported(ActionId::FileOpenAction));
    QVERIFY(!KiriView::ApplicationActions::videoActionUnsupported(ActionId::GoPreviousImageAction));
    QVERIFY(!KiriView::ApplicationActions::videoActionUnsupported(ActionId::FileDeleteAction));
    QVERIFY(!KiriView::ApplicationActions::videoActionUnsupported(
        ActionId::ViewToggleVideoPlaybackAction));
}

void TestApplicationShortcutPolicy::imageUnsupportedActionPolicyRejectsVideoOnlyCommands()
{
    QVERIFY(KiriView::ApplicationActions::imageActionUnsupported(
        ActionId::ViewToggleVideoPlaybackAction));
    QVERIFY(!KiriView::ApplicationActions::imageActionUnsupported(ActionId::ViewZoomInAction));
    QVERIFY(
        !KiriView::ApplicationActions::imageActionUnsupported(ActionId::ViewRotateClockwiseAction));
    QVERIFY(
        !KiriView::ApplicationActions::imageActionUnsupported(ActionId::WindowFullscreenAction));
    QVERIFY(!KiriView::ApplicationActions::imageActionUnsupported(ActionId::FileOpenAction));
}

void TestApplicationShortcutPolicy::horizontalArrowShortcutPolicyUsesActiveMediaMode()
{
    KiriView::ApplicationActions::VideoShortcutAvailabilityInput input;
    input.viewerShortcutsEnabled = true;
    input.directMediaNavigationActive = true;

    QVERIFY(KiriView::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(false, true, input));
    QVERIFY(
        !KiriView::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(false, false, input));
    QVERIFY(KiriView::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(true, false, input));

    input.viewerShortcutsEnabled = false;
    QVERIFY(!KiriView::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(true, true, input));

    input.viewerShortcutsEnabled = true;
    input.fileDeletionInProgress = true;
    QVERIFY(!KiriView::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(true, true, input));

    input.fileDeletionInProgress = false;
    input.directMediaNavigationActive = false;
    QVERIFY(!KiriView::ApplicationActions::mediaHorizontalArrowShortcutsEnabled(true, true, input));
}

QTEST_GUILESS_MAIN(TestApplicationShortcutPolicy)

#include "test_applicationshortcutpolicy.moc"

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationshortcutpolicy.h"

#include "kiriviewapplicationactions.h"

#include <QStringList>

namespace {
using Route = kiriview::ApplicationActions::ApplicationShortcutRoute;
using ActivationScope = kiriview::ApplicationActions::ApplicationShortcutActivationScope;
using Scope = kiriview::ApplicationActions::ImageShortcutScope;
using RouteSpec = kiriview::ApplicationActions::ShortcutRouteSpec;

bool isAsciiPrintableKey(Qt::Key key) { return key >= Qt::Key_Space && key <= Qt::Key_AsciiTilde; }

bool isDeletionOrNavigationKey(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        return true;
    default:
        return false;
    }
}

bool shortcutCombinationIsMenuDisplaySafe(QKeyCombination combination)
{
    const Qt::Key key = combination.key();
    if (isDeletionOrNavigationKey(key)) {
        return false;
    }

    const Qt::KeyboardModifiers commandModifiers
        = Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    if ((combination.keyboardModifiers() & commandModifiers) == Qt::NoModifier
        && isAsciiPrintableKey(key)) {
        return false;
    }

    return key != Qt::Key_unknown;
}

bool shortcutIsMenuDisplaySafe(const QKeySequence &shortcut)
{
    if (shortcut.isEmpty()) {
        return false;
    }

    for (int index = 0; index < shortcut.count(); ++index) {
        if (!shortcutCombinationIsMenuDisplaySafe(shortcut[static_cast<uint>(index)])) {
            return false;
        }
    }

    return true;
}

bool shortcutCombinationIsUnmodifiedAsciiPrintable(QKeyCombination combination)
{
    return combination.keyboardModifiers() == Qt::NoModifier
        && isAsciiPrintableKey(combination.key());
}

bool shortcutIsUnmodifiedAsciiPrintable(const QKeySequence &shortcut)
{
    return shortcut.count() == 1 && shortcutCombinationIsUnmodifiedAsciiPrintable(shortcut[0]);
}

bool exactShortcut(const QKeySequence &shortcut, const char *portableText)
{
    return shortcut.matches(QKeySequence::fromString(
               QString::fromLatin1(portableText), QKeySequence::PortableText))
        == QKeySequence::ExactMatch;
}

std::optional<qint64> fixedVideoSeekShortcutDeltaMilliseconds(const QKeySequence &shortcut)
{
    if (exactShortcut(shortcut, "Alt+Left")) {
        return -5000;
    }
    if (exactShortcut(shortcut, "Alt+Right")) {
        return 5000;
    }
    if (exactShortcut(shortcut, "Alt+Up")) {
        return 45000;
    }
    if (exactShortcut(shortcut, "Alt+Down")) {
        return -45000;
    }

    return std::nullopt;
}

bool routeMatchesSpec(const Route &route, const RouteSpec &spec)
{
    return route.activationScope == spec.activationScope
        && route.shortcutScope == spec.shortcutScope;
}

void appendActionToRoute(
    QList<Route> &routes, kiriview::ApplicationActions::ActionId actionId, const RouteSpec &spec)
{
    for (Route &route : routes) {
        if (!routeMatchesSpec(route, spec)) {
            continue;
        }

        if (!route.actionIds.contains(actionId)) {
            route.actionIds.push_back(actionId);
        }
        return;
    }

    routes.push_back(Route { { actionId }, spec.activationScope, spec.shortcutScope });
}

QList<Route> buildShortcutRoutes()
{
    QList<Route> routes;
    for (const kiriview::ApplicationActions::ActionDefinition &definition :
        kiriview::ApplicationActions::definitions()) {
        for (std::size_t index = 0; index < definition.shortcutRoutes.count; ++index) {
            appendActionToRoute(
                routes, definition.actionId, definition.shortcutRoutes.specs[index]);
        }
    }

    return routes;
}

}

namespace kiriview::ApplicationActions {
QKeySequence menuShortcut(const QList<QKeySequence> &shortcuts)
{
    for (const QKeySequence &shortcut : shortcuts) {
        if (shortcutIsMenuDisplaySafe(shortcut)) {
            return shortcut;
        }
    }

    return {};
}

QString shortcutListText(const QList<QKeySequence> &shortcuts)
{
    QStringList texts;
    texts.reserve(shortcuts.size());
    for (const QKeySequence &shortcut : shortcuts) {
        if (!shortcut.isEmpty()) {
            texts.push_back(shortcut.toString(QKeySequence::NativeText));
        }
    }

    return texts.join(QStringLiteral(" / "));
}

QList<QKeySequence> sanitizeProgramWideShortcuts(const QList<QKeySequence> &shortcuts)
{
    QList<QKeySequence> sanitizedShortcuts;
    sanitizedShortcuts.reserve(shortcuts.size());

    for (const QKeySequence &shortcut : shortcuts) {
        if (!shortcutIsUnmodifiedAsciiPrintable(shortcut)) {
            sanitizedShortcuts.push_back(shortcut);
        }
    }

    return sanitizedShortcuts;
}

ApplicationShortcutProjection shortcutProjection(const QList<QKeySequence> &programWideShortcuts,
    const QList<QKeySequence> &viewerLocalShortcuts)
{
    QList<QKeySequence> shortcuts = programWideShortcuts;
    shortcuts.append(viewerLocalShortcuts);
    const QKeySequence menu = menuShortcut(programWideShortcuts);
    return ApplicationShortcutProjection {
        shortcuts,
        programWideShortcuts,
        viewerLocalShortcuts,
        menu,
        shortcutListText(shortcuts),
        menu.toString(QKeySequence::NativeText),
    };
}

const QList<ApplicationShortcutRoute> &shortcutRoutes()
{
    static const QList<ApplicationShortcutRoute> routes = buildShortcutRoutes();
    return routes;
}

std::optional<ImageShortcutScope> imageShortcutScopeFromValue(int value)
{
    const ImageShortcutScope scope = static_cast<ImageShortcutScope>(value);

    if (!imageShortcutScopeKnown(scope)) {
        return std::nullopt;
    }

    return scope;
}

FixedShortcutDispatchOutcome fixedShortcutDispatchOutcome(
    const FixedShortcutDispatchInput &input, const QKeySequence &shortcut)
{
    if (!input.focusApplicable) {
        return {};
    }

    const VideoShortcutAvailabilityInput videoShortcutInput {
        input.helpActionsEnabled,
        input.viewerShortcutsEnabled,
        input.videoFileDeletionInProgress,
        input.activeNavigationActionsAvailable,
    };
    const bool horizontalArrowEnabled = mediaHorizontalArrowShortcutsEnabled(
        input.videoMode, input.readyViewerShortcutsEnabled, videoShortcutInput);
    const bool videoSeekEnabled = input.videoMode
        && videoShortcutsEnabledForScope(
            videoShortcutInput, ImageShortcutScope::ReadyViewerShortcutScope);
    const std::optional<qint64> videoSeekDelta = fixedVideoSeekShortcutDeltaMilliseconds(shortcut);

    if (videoSeekEnabled && videoSeekDelta.has_value()) {
        return FixedShortcutDispatchOutcome {
            FixedShortcutDispatchKind::VideoSeek,
            false,
            *videoSeekDelta,
        };
    }
    if (horizontalArrowEnabled && exactShortcut(shortcut, "Left")) {
        return FixedShortcutDispatchOutcome { FixedShortcutDispatchKind::HorizontalArrow, true, 0 };
    }
    if (horizontalArrowEnabled && exactShortcut(shortcut, "Right")) {
        return FixedShortcutDispatchOutcome { FixedShortcutDispatchKind::HorizontalArrow, false,
            0 };
    }
    if (input.twoPageViewerShortcutsEnabled && exactShortcut(shortcut, "Shift+Left")) {
        return FixedShortcutDispatchOutcome { FixedShortcutDispatchKind::SinglePageArrow, true, 0 };
    }
    if (input.twoPageViewerShortcutsEnabled && exactShortcut(shortcut, "Shift+Right")) {
        return FixedShortcutDispatchOutcome { FixedShortcutDispatchKind::SinglePageArrow, false,
            0 };
    }
    if (input.pannableViewerShortcutsEnabled && exactShortcut(shortcut, "Up")) {
        return FixedShortcutDispatchOutcome { FixedShortcutDispatchKind::VerticalPan, true, 0 };
    }
    if (input.pannableViewerShortcutsEnabled && exactShortcut(shortcut, "Down")) {
        return FixedShortcutDispatchOutcome { FixedShortcutDispatchKind::VerticalPan, false, 0 };
    }

    return {};
}

bool genericShortcutBindingEnabled(
    const ApplicationActionStateInput &actionState, const GenericShortcutBinding &binding)
{
    const bool unsupportedVideoIntercept
        = actionState.videoMode && videoActionUnsupported(binding.actionId);
    const bool unsupportedImageIntercept
        = !actionState.videoMode && imageActionUnsupported(binding.actionId);

    if (binding.shortcutScope.has_value()) {
        return applicationShortcutsEnabledForScope(actionState, *binding.shortcutScope)
            && (binding.actionEnabled || unsupportedVideoIntercept || unsupportedImageIntercept);
    }

    switch (binding.actionId) {
    case ActionId::OpenApplicationMenuAction:
        return actionState.applicationMenuShortcutEnabled && binding.actionEnabled;
    case ActionId::OptionsShowMenubarAction:
        return actionState.showMenubarActionEnabled && binding.actionEnabled;
    default:
        return binding.actionEnabled;
    }
}

GenericShortcutDispatchOutcome genericShortcutDispatchOutcome(
    const GenericShortcutDispatchInput &input, const QKeySequence &shortcut)
{
    if (!input.focusApplicable || shortcut.isEmpty()) {
        return {};
    }

    for (const GenericShortcutBinding &binding : input.bindings) {
        if (!binding.shortcuts.contains(shortcut)
            || !genericShortcutBindingEnabled(input.actionState, binding)) {
            continue;
        }
        if (input.actionState.videoMode && videoActionUnsupported(binding.actionId)) {
            return { GenericShortcutDispatchKind::UnsupportedVideoAction, binding.actionId };
        }
        if (!input.actionState.videoMode && imageActionUnsupported(binding.actionId)) {
            return { GenericShortcutDispatchKind::UnsupportedImageAction, binding.actionId };
        }
        if (binding.actionEnabled) {
            return { GenericShortcutDispatchKind::TriggerAction, binding.actionId };
        }
    }

    return {};
}

bool videoActionUnsupported(ActionId actionId)
{
    switch (actionId) {
    case ActionId::GoPreviousArchiveAction:
    case ActionId::GoNextArchiveAction:
    case ActionId::ViewZoomInAction:
    case ActionId::ViewZoomOutAction:
    case ActionId::ViewZoom50PercentAction:
    case ActionId::ViewZoom100PercentAction:
    case ActionId::ViewZoom200PercentAction:
    case ActionId::ViewFitAction:
    case ActionId::ViewFitHeightAction:
    case ActionId::ViewFitWidthAction:
    case ActionId::ViewRotateClockwiseAction:
    case ActionId::ViewRotateCounterclockwiseAction:
    case ActionId::ViewToggleTwoPageModeAction:
    case ActionId::ViewToggleRightToLeftReadingAction:
        return true;
    default:
        return false;
    }
}

bool imageActionUnsupported(ActionId actionId)
{
    switch (actionId) {
    case ActionId::ViewToggleVideoPlaybackAction:
        return true;
    default:
        return false;
    }
}

}

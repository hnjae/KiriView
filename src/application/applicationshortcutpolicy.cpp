// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationshortcutpolicy.h"

#include "kiriviewapplicationactions.h"

#include <QStringList>

namespace {
using Route = KiriView::ApplicationActions::ApplicationShortcutRoute;
using ActivationScope = KiriView::ApplicationActions::ApplicationShortcutActivationScope;
using Scope = KiriView::ApplicationActions::ImageShortcutScope;
using RouteSpec = KiriView::ApplicationActions::ShortcutRouteSpec;

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

bool routeMatchesSpec(const Route &route, const RouteSpec &spec)
{
    return route.activationScope == spec.activationScope
        && route.shortcutScope == spec.shortcutScope;
}

void appendActionToRoute(
    QList<Route> &routes, KiriView::ApplicationActions::ActionId actionId, const RouteSpec &spec)
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
    for (const KiriView::ApplicationActions::ActionDefinition &definition :
        KiriView::ApplicationActions::definitions()) {
        for (std::size_t index = 0; index < definition.shortcutRoutes.count; ++index) {
            appendActionToRoute(
                routes, definition.actionId, definition.shortcutRoutes.specs[index]);
        }
    }

    return routes;
}

}

namespace KiriView::ApplicationActions {
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
    switch (static_cast<ImageShortcutScope>(value)) {
    case ImageShortcutScope::HelpShortcutScope:
    case ImageShortcutScope::ViewerShortcutScope:
    case ImageShortcutScope::ReadyShortcutScope:
    case ImageShortcutScope::ReadyViewerShortcutScope:
    case ImageShortcutScope::ImageSelectionShortcutScope:
    case ImageShortcutScope::ImageSelectionViewerShortcutScope:
    case ImageShortcutScope::PageShortcutScope:
    case ImageShortcutScope::PageViewerShortcutScope:
    case ImageShortcutScope::RightToLeftReadingShortcutScope:
    case ImageShortcutScope::RightToLeftReadingViewerShortcutScope:
    case ImageShortcutScope::RotateShortcutScope:
    case ImageShortcutScope::RotateViewerShortcutScope:
    case ImageShortcutScope::PannableShortcutScope:
    case ImageShortcutScope::PannableViewerShortcutScope:
    case ImageShortcutScope::ContainerShortcutScope:
    case ImageShortcutScope::ContainerViewerShortcutScope:
        return static_cast<ImageShortcutScope>(value);
    }

    return std::nullopt;
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
    case ActionId::ViewPanTopLeftAction:
    case ActionId::ViewPanBottomRightAction:
    case ActionId::ViewScanForwardAction:
    case ActionId::ViewScanBackwardAction:
        return true;
    default:
        return false;
    }
}

}

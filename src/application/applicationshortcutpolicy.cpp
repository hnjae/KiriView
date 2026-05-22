// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationshortcutpolicy.h"

#include <QStringList>
#include <QVariantMap>

namespace {
using Route = KiriView::ApplicationActions::ApplicationShortcutRoute;
using Filter = KiriView::ApplicationActions::ApplicationShortcutFilter;
using ActionId = KiriViewApplication::ActionId;
using Scope = KiriView::ApplicationActions::ImageShortcutScope;

bool shortcutHasCommandModifier(const QKeySequence &shortcut)
{
    const Qt::KeyboardModifiers commandModifiers
        = Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;

    for (int index = 0; index < shortcut.count(); ++index) {
        const Qt::KeyboardModifiers shortcutModifiers
            = shortcut[static_cast<uint>(index)].keyboardModifiers();
        if ((shortcutModifiers & commandModifiers) != Qt::NoModifier) {
            return true;
        }
    }

    return false;
}

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

QKeySequence shortcutAlias(const QKeySequence &shortcut)
{
    if (shortcut.count() != 1) {
        return {};
    }

    const QKeyCombination combination = shortcut[0];
    const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
    const Qt::KeyboardModifiers allowedModifiers = Qt::ControlModifier | Qt::ShiftModifier;
    if ((modifiers & Qt::ControlModifier) == Qt::NoModifier
        || (modifiers & ~allowedModifiers) != Qt::NoModifier
        || !isAsciiPrintableKey(combination.key())) {
        return {};
    }

    return QKeySequence(QKeyCombination(modifiers & ~Qt::ControlModifier, combination.key()));
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
}

namespace KiriView::ApplicationActions {
QList<QKeySequence> filterShortcutsByCommandModifier(
    const QList<QKeySequence> &shortcuts, bool requireCommandModifier)
{
    QList<QKeySequence> filteredShortcuts;
    filteredShortcuts.reserve(shortcuts.size());

    for (const QKeySequence &shortcut : shortcuts) {
        if (!shortcut.isEmpty() && shortcutHasCommandModifier(shortcut) == requireCommandModifier) {
            filteredShortcuts.push_back(shortcut);
        }
    }

    return filteredShortcuts;
}

QKeySequence menuShortcut(const QList<QKeySequence> &shortcuts)
{
    for (const QKeySequence &shortcut : shortcuts) {
        if (shortcutIsMenuDisplaySafe(shortcut)) {
            return shortcut;
        }
    }

    return {};
}

QList<QKeySequence> shortcutAliases(const QList<QKeySequence> &shortcuts)
{
    QList<QKeySequence> aliases;
    aliases.reserve(shortcuts.size());

    for (const QKeySequence &shortcut : shortcuts) {
        const QKeySequence alias = shortcutAlias(shortcut);
        if (!alias.isEmpty() && !aliases.contains(alias)) {
            aliases.push_back(alias);
        }
    }

    return aliases;
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

QList<QKeySequence> sanitizeShortcuts(const QList<QKeySequence> &shortcuts)
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

ApplicationShortcutProjection shortcutProjection(const QList<QKeySequence> &shortcuts)
{
    const QKeySequence menu = menuShortcut(shortcuts);
    return ApplicationShortcutProjection {
        shortcuts,
        filterShortcutsByCommandModifier(shortcuts, true),
        filterShortcutsByCommandModifier(shortcuts, false),
        shortcutAliases(shortcuts),
        menu,
        shortcutListText(shortcuts),
        menu.toString(QKeySequence::NativeText),
    };
}

const QList<ApplicationShortcutRoute> &shortcutRoutes()
{
    static const QList<ApplicationShortcutRoute> routes {
        Route {
            { ActionId::FileOpenAction }, Filter::WithCommandModifier, Scope::HelpShortcutScope },
        Route { { ActionId::FileOpenAction }, Filter::WithoutCommandModifier,
            Scope::ViewerShortcutScope },
        Route {
            { ActionId::FileQuitAction }, Filter::WithCommandModifier, Scope::HelpShortcutScope },
        Route { { ActionId::FileQuitAction }, Filter::WithoutCommandModifier,
            Scope::ViewerShortcutScope },
        Route { { ActionId::FileQuitAction }, Filter::ShortcutAliases, Scope::ViewerShortcutScope },
        Route { { ActionId::FileMoveToTrashAction, ActionId::FileDeleteAction },
            Filter::AllShortcuts, Scope::ReadyViewerShortcutScope },
        Route { { ActionId::ViewZoomInAction, ActionId::ViewZoomOutAction, ActionId::ViewFitAction,
                    ActionId::ViewFitHeightAction, ActionId::ViewFitWidthAction,
                    ActionId::ViewActualSizeAction, ActionId::ViewToggleTwoPageModeAction },
            Filter::WithCommandModifier, Scope::ReadyShortcutScope },
        Route { { ActionId::ViewZoomInAction, ActionId::ViewZoomOutAction, ActionId::ViewFitAction,
                    ActionId::ViewFitHeightAction, ActionId::ViewFitWidthAction,
                    ActionId::ViewActualSizeAction, ActionId::ViewToggleTwoPageModeAction },
            Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope },
        Route { { ActionId::ViewZoomInAction, ActionId::ViewZoomOutAction, ActionId::ViewFitAction,
                    ActionId::ViewFitHeightAction, ActionId::ViewFitWidthAction,
                    ActionId::ViewActualSizeAction, ActionId::ViewToggleTwoPageModeAction },
            Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope },
        Route { { ActionId::ViewRotateClockwiseAction, ActionId::ViewRotateCounterclockwiseAction },
            Filter::WithCommandModifier, Scope::RotateShortcutScope },
        Route { { ActionId::ViewRotateClockwiseAction, ActionId::ViewRotateCounterclockwiseAction },
            Filter::WithoutCommandModifier, Scope::RotateViewerShortcutScope },
        Route { { ActionId::ViewRotateClockwiseAction, ActionId::ViewRotateCounterclockwiseAction },
            Filter::ShortcutAliases, Scope::RotateViewerShortcutScope },
        Route { { ActionId::ViewToggleRightToLeftReadingAction }, Filter::WithCommandModifier,
            Scope::RightToLeftReadingShortcutScope },
        Route { { ActionId::ViewToggleRightToLeftReadingAction }, Filter::WithoutCommandModifier,
            Scope::RightToLeftReadingViewerShortcutScope },
        Route { { ActionId::ViewToggleRightToLeftReadingAction }, Filter::ShortcutAliases,
            Scope::RightToLeftReadingViewerShortcutScope },
        Route { { ActionId::ViewPanTopLeftAction, ActionId::ViewPanBottomRightAction },
            Filter::WithCommandModifier, Scope::PannableShortcutScope },
        Route { { ActionId::ViewPanTopLeftAction, ActionId::ViewPanBottomRightAction },
            Filter::WithoutCommandModifier, Scope::PannableViewerShortcutScope },
        Route { { ActionId::ViewPanTopLeftAction, ActionId::ViewPanBottomRightAction },
            Filter::ShortcutAliases, Scope::PannableViewerShortcutScope },
        Route { { ActionId::ViewScanForwardAction, ActionId::ViewScanBackwardAction },
            Filter::WithCommandModifier, Scope::ReadyShortcutScope },
        Route { { ActionId::ViewScanForwardAction, ActionId::ViewScanBackwardAction },
            Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope },
        Route { { ActionId::ViewScanForwardAction, ActionId::ViewScanBackwardAction },
            Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope },
        Route { { ActionId::GoPreviousImageAction, ActionId::GoNextImageAction },
            Filter::WithCommandModifier, Scope::ImageSelectionShortcutScope },
        Route { { ActionId::GoPreviousImageAction, ActionId::GoNextImageAction },
            Filter::WithoutCommandModifier, Scope::ImageSelectionViewerShortcutScope },
        Route { { ActionId::GoPreviousImageAction, ActionId::GoNextImageAction },
            Filter::ShortcutAliases, Scope::ImageSelectionViewerShortcutScope },
        Route { { ActionId::GoFirstImageAction, ActionId::GoLastImageAction },
            Filter::WithCommandModifier, Scope::PageShortcutScope },
        Route { { ActionId::GoFirstImageAction, ActionId::GoLastImageAction },
            Filter::WithoutCommandModifier, Scope::PageViewerShortcutScope },
        Route { { ActionId::GoFirstImageAction, ActionId::GoLastImageAction },
            Filter::ShortcutAliases, Scope::PageViewerShortcutScope },
        Route { { ActionId::GoPreviousArchiveAction, ActionId::GoNextArchiveAction },
            Filter::WithCommandModifier, Scope::ContainerShortcutScope },
        Route { { ActionId::GoPreviousArchiveAction, ActionId::GoNextArchiveAction },
            Filter::WithoutCommandModifier, Scope::ContainerViewerShortcutScope },
        Route { { ActionId::GoPreviousArchiveAction, ActionId::GoNextArchiveAction },
            Filter::ShortcutAliases, Scope::ContainerViewerShortcutScope },
        Route { { ActionId::WindowFullscreenAction, ActionId::HelpShortcutsAction },
            Filter::WithCommandModifier, Scope::HelpShortcutScope },
        Route { { ActionId::WindowFullscreenAction, ActionId::HelpShortcutsAction },
            Filter::WithoutCommandModifier, Scope::HelpShortcutScope },
        Route { { ActionId::WindowFullscreenAction, ActionId::HelpShortcutsAction },
            Filter::ShortcutAliases, Scope::ViewerShortcutScope },
        Route { { ActionId::OptionsConfigureKeybindingAction, ActionId::OptionsShowMenubarAction },
            Filter::AllShortcuts, Scope::HelpShortcutScope },
    };
    return routes;
}

QVariantList shortcutRouteVariants()
{
    const QList<ApplicationShortcutRoute> &routes = shortcutRoutes();
    QVariantList variants;
    variants.reserve(routes.size());

    for (const ApplicationShortcutRoute &route : routes) {
        variants.push_back(QVariantMap {
            { QStringLiteral("actionIds"), actionIdVariants(route.actionIds) },
            { QStringLiteral("shortcutFilter"), static_cast<int>(route.shortcutFilter) },
            { QStringLiteral("shortcutScope"), static_cast<int>(route.shortcutScope) },
        });
    }

    return variants;
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

bool videoShortcutsEnabledForScope(
    const VideoShortcutAvailabilityInput &input, ImageShortcutScope scope)
{
    const bool videoReadyShortcutsEnabled
        = input.helpShortcutsEnabled && !input.fileDeletionInProgress;
    const bool videoReadyViewerShortcutsEnabled
        = input.viewerShortcutsEnabled && !input.fileDeletionInProgress;
    const bool videoMediaShortcutsEnabled
        = input.mediaNavigationActive && videoReadyShortcutsEnabled;
    const bool videoMediaViewerShortcutsEnabled
        = input.mediaNavigationActive && videoReadyViewerShortcutsEnabled;

    switch (scope) {
    case ImageShortcutScope::HelpShortcutScope:
        return input.helpShortcutsEnabled;
    case ImageShortcutScope::ViewerShortcutScope:
        return input.viewerShortcutsEnabled;
    case ImageShortcutScope::ReadyShortcutScope:
    case ImageShortcutScope::RotateShortcutScope:
    case ImageShortcutScope::PannableShortcutScope:
    case ImageShortcutScope::ContainerShortcutScope:
    case ImageShortcutScope::RightToLeftReadingShortcutScope:
        return videoReadyShortcutsEnabled;
    case ImageShortcutScope::ReadyViewerShortcutScope:
    case ImageShortcutScope::RotateViewerShortcutScope:
    case ImageShortcutScope::PannableViewerShortcutScope:
    case ImageShortcutScope::ContainerViewerShortcutScope:
    case ImageShortcutScope::RightToLeftReadingViewerShortcutScope:
        return videoReadyViewerShortcutsEnabled;
    case ImageShortcutScope::ImageSelectionShortcutScope:
    case ImageShortcutScope::PageShortcutScope:
        return videoMediaShortcutsEnabled;
    case ImageShortcutScope::ImageSelectionViewerShortcutScope:
    case ImageShortcutScope::PageViewerShortcutScope:
        return videoMediaViewerShortcutsEnabled;
    }

    return false;
}

bool videoActionUnsupported(KiriViewApplication::ActionId actionId)
{
    switch (actionId) {
    case KiriViewApplication::GoPreviousArchiveAction:
    case KiriViewApplication::GoNextArchiveAction:
    case KiriViewApplication::ViewZoomInAction:
    case KiriViewApplication::ViewZoomOutAction:
    case KiriViewApplication::ViewFitAction:
    case KiriViewApplication::ViewFitHeightAction:
    case KiriViewApplication::ViewFitWidthAction:
    case KiriViewApplication::ViewActualSizeAction:
    case KiriViewApplication::ViewRotateClockwiseAction:
    case KiriViewApplication::ViewRotateCounterclockwiseAction:
    case KiriViewApplication::ViewToggleTwoPageModeAction:
    case KiriViewApplication::ViewToggleRightToLeftReadingAction:
    case KiriViewApplication::ViewPanTopLeftAction:
    case KiriViewApplication::ViewPanBottomRightAction:
    case KiriViewApplication::ViewScanForwardAction:
    case KiriViewApplication::ViewScanBackwardAction:
        return true;
    default:
        return false;
    }
}

bool mediaHorizontalArrowShortcutsEnabled(bool videoMode, bool imageReadyViewerShortcutsEnabled,
    const VideoShortcutAvailabilityInput &videoInput)
{
    if (!videoMode) {
        return imageReadyViewerShortcutsEnabled;
    }

    return videoInput.mediaNavigationActive && !videoInput.fileDeletionInProgress
        && videoInput.viewerShortcutsEnabled;
}
}

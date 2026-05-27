// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplicationactions.h"

namespace {
namespace Actions = KiriView::ApplicationActions;
using Filter = KiriView::ApplicationActions::ApplicationShortcutFilter;
using Scope = KiriView::ApplicationActions::ImageShortcutScope;

constexpr Actions::DefaultShortcutSpec noDefaultShortcuts() { return {}; }

constexpr Actions::DefaultShortcutRouteSpec noShortcutRoutes() { return {}; }

constexpr Actions::DefaultShortcutSpec standardShortcutSpec(QKeySequence::StandardKey key)
{
    return Actions::DefaultShortcutSpec {
        Actions::ShortcutSpecKind::StandardKey,
        key,
        {},
        0,
    };
}

template <typename... Sequences>
constexpr Actions::DefaultShortcutSpec portableShortcutSpec(Sequences... sequences)
{
    static_assert(sizeof...(Sequences) <= Actions::maxPortableShortcutCount);
    return Actions::DefaultShortcutSpec {
        Actions::ShortcutSpecKind::PortableText,
        QKeySequence::UnknownKey,
        { sequences... },
        sizeof...(Sequences),
    };
}

constexpr Actions::ShortcutRouteSpec route(Filter filter, Scope scope)
{
    return Actions::ShortcutRouteSpec { filter, scope };
}

template <typename... Routes>
constexpr Actions::DefaultShortcutRouteSpec shortcutRouteSpecs(Routes... routes)
{
    static_assert(sizeof...(Routes) <= Actions::maxShortcutRouteSpecCount);
    return Actions::DefaultShortcutRouteSpec { { routes... }, sizeof...(Routes) };
}

constexpr Actions::ActionDefinition existingAction(Actions::ActionId actionId, const char *name,
    Actions::DefaultShortcutSpec defaultShortcuts, Actions::DefaultShortcutRouteSpec shortcutRoutes)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Existing,
        KStandardActions::Open, {}, nullptr, defaultShortcuts, shortcutRoutes };
}

constexpr Actions::ActionDefinition inheritedAction(Actions::ActionId actionId, const char *name,
    Actions::DefaultShortcutRouteSpec shortcutRoutes = noShortcutRoutes())
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Inherited,
        KStandardActions::Open, {}, nullptr, noDefaultShortcuts(), shortcutRoutes };
}

constexpr Actions::ActionDefinition registeredAction(Actions::ActionId actionId, const char *name,
    KLazyLocalizedString text, const char *iconName, Actions::DefaultShortcutSpec defaultShortcuts,
    Actions::DefaultShortcutRouteSpec shortcutRoutes)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Registered,
        KStandardActions::Open, text, iconName, defaultShortcuts, shortcutRoutes };
}

constexpr Actions::ActionDefinition standardAction(Actions::ActionId actionId, const char *name,
    KStandardActions::StandardAction actionType, KLazyLocalizedString text,
    Actions::DefaultShortcutSpec defaultShortcuts, Actions::DefaultShortcutRouteSpec shortcutRoutes)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Standard,
        actionType, text, nullptr, defaultShortcuts, shortcutRoutes };
}

constexpr Actions::ActionDefinition showMenubarAction(Actions::ActionId actionId, const char *name,
    KStandardActions::StandardAction actionType, KLazyLocalizedString text,
    Actions::DefaultShortcutSpec defaultShortcuts, Actions::DefaultShortcutRouteSpec shortcutRoutes)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::ShowMenubar,
        actionType, text, nullptr, defaultShortcuts, shortcutRoutes,
        Actions::ShortcutConfigurability::NonConfigurable, Actions::ShortcutAliasPolicy::NoAlias };
}

constexpr Actions::ActionDefinition fixedCommandAction(Actions::ActionId actionId, const char *name,
    KLazyLocalizedString text, const char *iconName, Actions::DefaultShortcutSpec defaultShortcuts)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Registered,
        KStandardActions::Open, text, iconName, defaultShortcuts, noShortcutRoutes(),
        Actions::ShortcutConfigurability::NonConfigurable, Actions::ShortcutAliasPolicy::NoAlias };
}

constexpr std::array actionDefinitions {
    standardAction(Actions::ActionId::FileOpenAction, "file_open", KStandardActions::Open,
        kli18n("Open"), standardShortcutSpec(QKeySequence::Open),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::HelpShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ViewerShortcutScope))),
    registeredAction(Actions::ActionId::FileOpenWithAction, "file_open_with",
        kli18nc("@action", "Open With..."), "document-open-symbolic", noDefaultShortcuts(),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::HelpShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ViewerShortcutScope))),
    standardAction(Actions::ActionId::FileMoveToTrashAction, "movetotrash",
        KStandardActions::MoveToTrash, kli18nc("@action", "Move to Trash"),
        portableShortcutSpec("Delete"),
        shortcutRouteSpecs(route(Filter::AllShortcuts, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::FileDeleteAction, "deletefile", KStandardActions::DeleteFile,
        kli18nc("@action", "Delete Permanently"), portableShortcutSpec("Shift+Delete"),
        shortcutRouteSpecs(route(Filter::AllShortcuts, Scope::ReadyViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoPreviousArchiveAction, "go_previous_archive",
        kli18nc("@action", "Previous Archive"), "go-previous-use", portableShortcutSpec("Ctrl+["),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ContainerShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ContainerViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ContainerViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoNextArchiveAction, "go_next_archive",
        kli18nc("@action", "Next Archive"), "go-next-use", portableShortcutSpec("Ctrl+]"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ContainerShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ContainerViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ContainerViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoPreviousImageAction, "go_previous_image",
        kli18nc("@action", "Previous"), "go-previous",
        standardShortcutSpec(QKeySequence::MoveToPreviousPage),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ImageSelectionShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ImageSelectionViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ImageSelectionViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoNextImageAction, "go_next_image",
        kli18nc("@action", "Next"), "go-next", standardShortcutSpec(QKeySequence::MoveToNextPage),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ImageSelectionShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ImageSelectionViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ImageSelectionViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoFirstImageAction, "go_first_image",
        kli18nc("@action", "First Image"), "go-first-symbolic",
        portableShortcutSpec("Ctrl+Home", "Home"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::PageShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::PageViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::PageViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoLastImageAction, "go_last_image",
        kli18nc("@action", "Last Image"), "go-last-symbolic",
        portableShortcutSpec("Ctrl+End", "End"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::PageShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::PageViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::PageViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewZoomInAction, "view_zoom_in", KStandardActions::ZoomIn,
        kli18nc("@action", "Zoom In"), portableShortcutSpec("Ctrl+=", "Ctrl++"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ReadyShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewZoomOutAction, "view_zoom_out", KStandardActions::ZoomOut,
        kli18nc("@action", "Zoom Out"), portableShortcutSpec("Ctrl+-"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ReadyShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewFitAction, "view_fit", KStandardActions::FitToPage,
        kli18nc("@action", "Fit"), portableShortcutSpec("Ctrl+1"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ReadyShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewFitHeightAction, "view_fit_height",
        KStandardActions::FitToHeight, kli18nc("@action", "Fit Height"),
        portableShortcutSpec("Ctrl+2"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ReadyShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewFitWidthAction, "view_fit_width",
        KStandardActions::FitToWidth, kli18nc("@action", "Fit Width"),
        portableShortcutSpec("Ctrl+3"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ReadyShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewActualSizeAction, "view_actual_size",
        KStandardActions::ActualSize, kli18nc("@action", "Actual Size"),
        portableShortcutSpec("Ctrl+0"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ReadyShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewRotateClockwiseAction, "view_rotate_clockwise",
        kli18nc("@action", "Rotate Clockwise"), "object-rotate-right-symbolic",
        portableShortcutSpec("Ctrl+R"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::RotateShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::RotateViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::RotateViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewRotateCounterclockwiseAction,
        "view_rotate_counterclockwise", kli18nc("@action", "Rotate Counterclockwise"),
        "object-rotate-left-symbolic", portableShortcutSpec("Ctrl+Shift+R"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::RotateShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::RotateViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::RotateViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewToggleTwoPageModeAction, "view_toggle_two_page_mode",
        kli18nc("@action", "Two-Page Spread"), "view-split-left-right-symbolic",
        portableShortcutSpec("Ctrl+S"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ReadyShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewToggleRightToLeftReadingAction,
        "view_toggle_right_to_left_reading", kli18nc("@action", "Right-to-Left Reading"),
        "format-text-direction-rtl-symbolic", portableShortcutSpec("Ctrl+B"),
        shortcutRouteSpecs(
            route(Filter::WithCommandModifier, Scope::RightToLeftReadingShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::RightToLeftReadingViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::RightToLeftReadingViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewToggleInfoPanelAction, "view_toggle_info_panel",
        kli18nc("@action", "Show Info Panel"), "documentinfo-symbolic",
        portableShortcutSpec("Ctrl+I"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::HelpShortcutScope))),
    registeredAction(Actions::ActionId::ViewToggleThumbnailPanelAction,
        "view_toggle_thumbnail_panel", kli18nc("@action", "Show Thumbnail Panel"),
        "view-list-icons-symbolic", portableShortcutSpec("Ctrl+T"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::HelpShortcutScope))),
    registeredAction(Actions::ActionId::ViewPanTopLeftAction, "view_pan_top_left",
        kli18nc("@action", "Top Left"), nullptr, portableShortcutSpec("Ctrl+<"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::PannableShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::PannableViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::PannableViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewPanBottomRightAction, "view_pan_bottom_right",
        kli18nc("@action", "Bottom Right"), nullptr, portableShortcutSpec("Ctrl+>"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::PannableShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::PannableViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::PannableViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewScanForwardAction, "view_scan_forward",
        kli18nc("@action", "Scan Forward"), nullptr, portableShortcutSpec("Ctrl+."),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ReadyShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewScanBackwardAction, "view_scan_backward",
        kli18nc("@action", "Scan Backward"), nullptr, portableShortcutSpec("Ctrl+,"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::ReadyShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ReadyViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::WindowFullscreenAction, "window_fullscreen",
        KStandardActions::FullScreen, kli18nc("@action", "Fullscreen"),
        portableShortcutSpec("Ctrl+F", "F11"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::HelpShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::HelpShortcutScope),
            route(Filter::ShortcutAliases, Scope::ViewerShortcutScope))),
    registeredAction(Actions::ActionId::HelpShortcutsAction, "help_shortcuts",
        kli18nc("@action", "Keyboard Shortcuts"), "help-keyboard-shortcuts-symbolic",
        portableShortcutSpec("Ctrl+?", "F1"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::HelpShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::HelpShortcutScope),
            route(Filter::ShortcutAliases, Scope::ViewerShortcutScope))),
    inheritedAction(Actions::ActionId::OptionsConfigureKeybindingAction,
        "options_configure_keybinding",
        shortcutRouteSpecs(route(Filter::AllShortcuts, Scope::HelpShortcutScope))),
    showMenubarAction(Actions::ActionId::OptionsShowMenubarAction, "options_show_menubar",
        KStandardActions::ShowMenubar, kli18nc("@action", "Show Menubar"),
        portableShortcutSpec("Ctrl+M"), noShortcutRoutes()),
    fixedCommandAction(Actions::ActionId::OpenApplicationMenuAction, "open_application_menu",
        kli18nc("@action", "Open Application Menu"), "application-menu-symbolic",
        portableShortcutSpec("F10")),
    existingAction(Actions::ActionId::FileQuitAction, "file_quit", portableShortcutSpec("Ctrl+Q"),
        shortcutRouteSpecs(route(Filter::WithCommandModifier, Scope::HelpShortcutScope),
            route(Filter::WithoutCommandModifier, Scope::ViewerShortcutScope),
            route(Filter::ShortcutAliases, Scope::ViewerShortcutScope))),
};

static_assert(actionDefinitions.size() == Actions::actionDefinitionCount);

constexpr bool actionDefinitionsFollowActionIdOrder()
{
    for (std::size_t index = 0; index < actionDefinitions.size(); ++index) {
        if (actionDefinitions[index].actionId != static_cast<Actions::ActionId>(index)) {
            return false;
        }
    }

    return true;
}

static_assert(actionDefinitionsFollowActionIdOrder());

bool actionIdInRange(Actions::ActionId actionId)
{
    const int index = static_cast<int>(actionId);
    return index >= 0 && index < static_cast<int>(Actions::actionDefinitionCount);
}

QKeySequence shortcut(const char *sequence)
{
    return QKeySequence::fromString(QString::fromLatin1(sequence), QKeySequence::PortableText);
}

QList<QKeySequence> portableShortcutList(
    const std::array<const char *, Actions::maxPortableShortcutCount> &sequences, std::size_t count)
{
    QList<QKeySequence> shortcutList;
    shortcutList.reserve(static_cast<qsizetype>(count));
    for (std::size_t index = 0; index < count; ++index) {
        shortcutList.push_back(shortcut(sequences[index]));
    }

    return shortcutList;
}

QList<QKeySequence> standardShortcuts(QKeySequence::StandardKey key)
{
    return QKeySequence::keyBindings(key);
}
}

namespace KiriView::ApplicationActions {
const std::array<ActionDefinition, actionDefinitionCount> &definitions()
{
    return actionDefinitions;
}

const ActionDefinition *definitionForId(Actions::ActionId actionId)
{
    if (!actionIdInRange(actionId)) {
        return nullptr;
    }

    return &actionDefinitions[static_cast<std::size_t>(actionId)];
}

const ActionDefinition *definitionForName(const QString &actionName)
{
    for (const ActionDefinition &definition : actionDefinitions) {
        if (actionName == QString::fromLatin1(definition.name)) {
            return &definition;
        }
    }

    return nullptr;
}

QString actionName(Actions::ActionId actionId)
{
    const Actions::ActionDefinition *definition = definitionForId(actionId);
    return definition == nullptr ? QString() : QString::fromLatin1(definition->name);
}

QString latin1String(const char *text)
{
    return text == nullptr ? QString() : QString::fromLatin1(text);
}

QString localizedString(const KLazyLocalizedString &text)
{
    return text.isEmpty() ? QString() : text.toString();
}

QList<QKeySequence> defaultShortcuts(const DefaultShortcutSpec &spec)
{
    switch (spec.kind) {
    case ShortcutSpecKind::None:
        return {};
    case ShortcutSpecKind::PortableText:
        return portableShortcutList(spec.portableText, spec.portableTextCount);
    case ShortcutSpecKind::StandardKey:
        return standardShortcuts(spec.standardKey);
    }

    return {};
}
}

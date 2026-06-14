// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplicationactions.h"

#include "applicationzoompresets.h"

namespace {
namespace Actions = kiriview::ApplicationActions;
using ActivationScope = kiriview::ApplicationActions::ApplicationShortcutActivationScope;
using Category = kiriview::ApplicationActions::ShortcutHelpCategory;
using Scope = kiriview::ApplicationActions::ImageShortcutScope;

struct ShortcutHelpCategoryDefinition {
    Category category;
    const char *key;
    KLazyLocalizedString text;
    int order = 0;
};

constexpr Actions::DefaultShortcutSpec noDefaultShortcuts() { return {}; }

constexpr Actions::DefaultShortcutRouteSpec noShortcutRoutes() { return {}; }

constexpr std::array shortcutHelpCategories {
    ShortcutHelpCategoryDefinition { Category::File, "file", kli18nc("@title:group", "File"), 0 },
    ShortcutHelpCategoryDefinition {
        Category::Navigation, "navigation", kli18nc("@title:group", "Navigation"), 1 },
    ShortcutHelpCategoryDefinition { Category::View, "view", kli18nc("@title:group", "View"), 2 },
    ShortcutHelpCategoryDefinition {
        Category::Panels, "panels", kli18nc("@title:group", "Panels"), 3 },
    ShortcutHelpCategoryDefinition {
        Category::Window, "window", kli18nc("@title:group", "Window"), 4 },
    ShortcutHelpCategoryDefinition {
        Category::Settings, "settings", kli18nc("@title:group", "Settings"), 5 },
    ShortcutHelpCategoryDefinition { Category::Help, "help", kli18nc("@title:group", "Help"), 6 },
};

static_assert(shortcutHelpCategories.size() == 7);

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

constexpr Actions::ShortcutRouteSpec route(ActivationScope activationScope, Scope scope)
{
    return Actions::ShortcutRouteSpec { activationScope, scope };
}

template <typename... Routes>
constexpr Actions::DefaultShortcutRouteSpec shortcutRouteSpecs(Routes... routes)
{
    static_assert(sizeof...(Routes) <= Actions::maxShortcutRouteSpecCount);
    return Actions::DefaultShortcutRouteSpec { { routes... }, sizeof...(Routes) };
}

constexpr Actions::ActionDefinition existingAction(Actions::ActionId actionId, const char *name,
    Category shortcutHelpCategory, Actions::DefaultShortcutSpec programWideShortcuts,
    Actions::DefaultShortcutSpec viewerLocalShortcuts,
    Actions::DefaultShortcutRouteSpec shortcutRoutes)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Existing,
        KStandardActions::Open, {}, nullptr, programWideShortcuts, viewerLocalShortcuts,
        shortcutRoutes, shortcutHelpCategory };
}

constexpr Actions::ActionDefinition inheritedAction(Actions::ActionId actionId, const char *name,
    Category shortcutHelpCategory,
    Actions::DefaultShortcutRouteSpec shortcutRoutes = noShortcutRoutes())
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Inherited,
        KStandardActions::Open, {}, nullptr, noDefaultShortcuts(), noDefaultShortcuts(),
        shortcutRoutes, shortcutHelpCategory };
}

constexpr Actions::ActionDefinition registeredAction(Actions::ActionId actionId, const char *name,
    Category shortcutHelpCategory, KLazyLocalizedString text, const char *iconName,
    Actions::DefaultShortcutSpec programWideShortcuts,
    Actions::DefaultShortcutSpec viewerLocalShortcuts,
    Actions::DefaultShortcutRouteSpec shortcutRoutes)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Registered,
        KStandardActions::Open, text, iconName, programWideShortcuts, viewerLocalShortcuts,
        shortcutRoutes, shortcutHelpCategory };
}

constexpr Actions::ActionDefinition standardAction(Actions::ActionId actionId, const char *name,
    Category shortcutHelpCategory, KStandardActions::StandardAction actionType,
    KLazyLocalizedString text, Actions::DefaultShortcutSpec programWideShortcuts,
    Actions::DefaultShortcutSpec viewerLocalShortcuts,
    Actions::DefaultShortcutRouteSpec shortcutRoutes)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Standard,
        actionType, text, nullptr, programWideShortcuts, viewerLocalShortcuts, shortcutRoutes,
        shortcutHelpCategory };
}

constexpr Actions::ActionDefinition showMenubarAction(Actions::ActionId actionId, const char *name,
    Category shortcutHelpCategory, KStandardActions::StandardAction actionType,
    KLazyLocalizedString text, Actions::DefaultShortcutSpec defaultShortcuts,
    Actions::DefaultShortcutRouteSpec shortcutRoutes)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::ShowMenubar,
        actionType, text, nullptr, defaultShortcuts, noDefaultShortcuts(), shortcutRoutes,
        shortcutHelpCategory, Actions::ShortcutConfigurability::NonConfigurable };
}

constexpr Actions::ActionDefinition fixedCommandAction(Actions::ActionId actionId, const char *name,
    Category shortcutHelpCategory, KLazyLocalizedString text, const char *iconName,
    Actions::DefaultShortcutSpec defaultShortcuts)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Registered,
        KStandardActions::Open, text, iconName, defaultShortcuts, noDefaultShortcuts(),
        noShortcutRoutes(), shortcutHelpCategory,
        Actions::ShortcutConfigurability::NonConfigurable };
}

constexpr Actions::ActionDefinition zoomPresetAction(
    Actions::ActionId actionId, Actions::DefaultShortcutSpec viewerLocalShortcuts)
{
    const Actions::ZoomPresetDescriptor *preset = Actions::zoomPresetDescriptorForAction(actionId);
    return registeredAction(actionId, preset == nullptr ? "" : preset->actionName, Category::View,
        preset == nullptr ? KLazyLocalizedString() : preset->actionText, nullptr,
        noDefaultShortcuts(), viewerLocalShortcuts,
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope)));
}

constexpr std::array actionDefinitions {
    standardAction(Actions::ActionId::FileOpenAction, "file_open", Category::File,
        KStandardActions::Open, kli18n("Open"), standardShortcutSpec(QKeySequence::Open),
        noDefaultShortcuts(),
        shortcutRouteSpecs(route(ActivationScope::ProgramWide, Scope::HelpShortcutScope))),
    registeredAction(Actions::ActionId::FileOpenWithAction, "file_open_with", Category::File,
        kli18nc("@action", "Open With..."), "document-open-symbolic", noDefaultShortcuts(),
        noDefaultShortcuts(), noShortcutRoutes()),
    standardAction(Actions::ActionId::FileMoveToTrashAction, "movetotrash", Category::File,
        KStandardActions::MoveToTrash, kli18nc("@action", "Move to Trash"), noDefaultShortcuts(),
        portableShortcutSpec("Delete"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::FileDeleteAction, "deletefile", Category::File,
        KStandardActions::DeleteFile, kli18nc("@action", "Delete Permanently"),
        noDefaultShortcuts(), portableShortcutSpec("Shift+Delete"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoPreviousArchiveAction, "go_previous_archive",
        Category::Navigation, kli18nc("@action", "Previous Archive"), "go-previous-use",
        noDefaultShortcuts(), portableShortcutSpec("["),
        shortcutRouteSpecs(
            route(ActivationScope::ViewerLocal, Scope::ContainerViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoNextArchiveAction, "go_next_archive",
        Category::Navigation, kli18nc("@action", "Next Archive"), "go-next-use",
        noDefaultShortcuts(), portableShortcutSpec("]"),
        shortcutRouteSpecs(
            route(ActivationScope::ViewerLocal, Scope::ContainerViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoPreviousImageAction, "go_previous_image",
        Category::Navigation, kli18nc("@action", "Previous"), "go-previous", noDefaultShortcuts(),
        standardShortcutSpec(QKeySequence::MoveToPreviousPage),
        shortcutRouteSpecs(
            route(ActivationScope::ViewerLocal, Scope::ImageSelectionViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoNextImageAction, "go_next_image", Category::Navigation,
        kli18nc("@action", "Next"), "go-next", noDefaultShortcuts(),
        standardShortcutSpec(QKeySequence::MoveToNextPage),
        shortcutRouteSpecs(
            route(ActivationScope::ViewerLocal, Scope::ImageSelectionViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoFirstImageAction, "go_first_image", Category::Navigation,
        kli18nc("@action", "First Image"), "go-first-symbolic", noDefaultShortcuts(),
        portableShortcutSpec("Home"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::PageViewerShortcutScope))),
    registeredAction(Actions::ActionId::GoLastImageAction, "go_last_image", Category::Navigation,
        kli18nc("@action", "Last Image"), "go-last-symbolic", noDefaultShortcuts(),
        portableShortcutSpec("End"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::PageViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewZoomInAction, "view_zoom_in", Category::View,
        KStandardActions::ZoomIn, kli18nc("@action", "Zoom In"), noDefaultShortcuts(),
        portableShortcutSpec("=", "+"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewZoomOutAction, "view_zoom_out", Category::View,
        KStandardActions::ZoomOut, kli18nc("@action", "Zoom Out"), noDefaultShortcuts(),
        portableShortcutSpec("-"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    zoomPresetAction(Actions::ActionId::ViewZoom50PercentAction, portableShortcutSpec("`")),
    zoomPresetAction(Actions::ActionId::ViewZoom100PercentAction, portableShortcutSpec("1")),
    zoomPresetAction(Actions::ActionId::ViewZoom200PercentAction, portableShortcutSpec("2")),
    standardAction(Actions::ActionId::ViewFitAction, "view_fit", Category::View,
        KStandardActions::FitToPage, kli18nc("@action", "Fit to Window"), noDefaultShortcuts(),
        portableShortcutSpec("0"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewFitHeightAction, "view_fit_height", Category::View,
        KStandardActions::FitToHeight, kli18nc("@action", "Fit Height"), noDefaultShortcuts(),
        portableShortcutSpec("8"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::ViewFitWidthAction, "view_fit_width", Category::View,
        KStandardActions::FitToWidth, kli18nc("@action", "Fit Width"), noDefaultShortcuts(),
        portableShortcutSpec("9"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewRotateClockwiseAction, "view_rotate_clockwise",
        Category::View, kli18nc("@action", "Rotate Clockwise"), "object-rotate-right-symbolic",
        noDefaultShortcuts(), portableShortcutSpec("R"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::RotateViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewRotateCounterclockwiseAction,
        "view_rotate_counterclockwise", Category::View,
        kli18nc("@action", "Rotate Counterclockwise"), "object-rotate-left-symbolic",
        noDefaultShortcuts(), portableShortcutSpec("Shift+R"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::RotateViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewToggleTwoPageModeAction, "view_toggle_two_page_mode",
        Category::View, kli18nc("@action", "Two-Page Spread"), "view-split-left-right-symbolic",
        noDefaultShortcuts(), portableShortcutSpec("S"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewToggleRightToLeftReadingAction,
        "view_toggle_right_to_left_reading", Category::View,
        kli18nc("@action", "Right-to-Left Reading"), "format-text-direction-rtl-symbolic",
        noDefaultShortcuts(), portableShortcutSpec("B"),
        shortcutRouteSpecs(
            route(ActivationScope::ViewerLocal, Scope::RightToLeftReadingViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewToggleInfoPanelAction, "view_toggle_info_panel",
        Category::Panels, kli18nc("@action", "Show Info Panel"), "documentinfo-symbolic",
        noDefaultShortcuts(), portableShortcutSpec("I"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewToggleThumbnailPanelAction,
        "view_toggle_thumbnail_panel", Category::Panels, kli18nc("@action", "Show Thumbnail Panel"),
        "view-list-icons-symbolic", noDefaultShortcuts(), portableShortcutSpec("T"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewGoToContentStartAction, "view_go_to_content_start",
        Category::View, kli18nc("@action", "Go to Content Start"), nullptr, noDefaultShortcuts(),
        portableShortcutSpec("Shift+,", "Alt+Home"),
        shortcutRouteSpecs(
            route(ActivationScope::ViewerLocal, Scope::MediaStartEndViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewGoToContentEndAction, "view_go_to_content_end",
        Category::View, kli18nc("@action", "Go to Content End"), nullptr, noDefaultShortcuts(),
        portableShortcutSpec("Shift+.", "Alt+End"),
        shortcutRouteSpecs(
            route(ActivationScope::ViewerLocal, Scope::MediaStartEndViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewScanForwardAction, "view_scan_forward", Category::View,
        kli18nc("@action", "Scan Forward"), nullptr, noDefaultShortcuts(),
        portableShortcutSpec(".", "Space"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewScanBackwardAction, "view_scan_backward",
        Category::View, kli18nc("@action", "Scan Backward"), nullptr, noDefaultShortcuts(),
        portableShortcutSpec(",", "Shift+Space"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    registeredAction(Actions::ActionId::ViewToggleVideoPlaybackAction, "view_toggle_video_playback",
        Category::View, kli18nc("@action", "Play/Pause"), "media-playback-start-symbolic",
        noDefaultShortcuts(), portableShortcutSpec("P"),
        shortcutRouteSpecs(route(ActivationScope::ViewerLocal, Scope::ReadyViewerShortcutScope))),
    standardAction(Actions::ActionId::WindowFullscreenAction, "window_fullscreen", Category::Window,
        KStandardActions::FullScreen, kli18nc("@action", "Fullscreen"),
        portableShortcutSpec("Ctrl+F", "F11"), portableShortcutSpec("F"),
        shortcutRouteSpecs(route(ActivationScope::ProgramWide, Scope::HelpShortcutScope),
            route(ActivationScope::ViewerLocal, Scope::ViewerShortcutScope))),
    registeredAction(Actions::ActionId::HelpShortcutsAction, "help_shortcuts", Category::Help,
        kli18nc("@action", "Keyboard Shortcuts"), "help-keyboard-shortcuts-symbolic",
        portableShortcutSpec("Ctrl+?", "F1"), portableShortcutSpec("?"),
        shortcutRouteSpecs(route(ActivationScope::ProgramWide, Scope::HelpShortcutScope),
            route(ActivationScope::ViewerLocal, Scope::ViewerShortcutScope))),
    inheritedAction(Actions::ActionId::OptionsConfigureKeybindingAction,
        "options_configure_keybinding", Category::Settings,
        shortcutRouteSpecs(route(ActivationScope::ProgramWide, Scope::HelpShortcutScope))),
    showMenubarAction(Actions::ActionId::OptionsShowMenubarAction, "options_show_menubar",
        Category::Settings, KStandardActions::ShowMenubar, kli18nc("@action", "Show Menubar"),
        portableShortcutSpec("Ctrl+M"), noShortcutRoutes()),
    fixedCommandAction(Actions::ActionId::OpenApplicationMenuAction, "open_application_menu",
        Category::Help, kli18nc("@action", "Open Application Menu"), "application-menu-symbolic",
        portableShortcutSpec("F10")),
    existingAction(Actions::ActionId::FileQuitAction, "file_quit", Category::File,
        portableShortcutSpec("Ctrl+Q"), portableShortcutSpec("Q"),
        shortcutRouteSpecs(route(ActivationScope::ProgramWide, Scope::HelpShortcutScope),
            route(ActivationScope::ViewerLocal, Scope::ViewerShortcutScope))),
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

const ShortcutHelpCategoryDefinition &shortcutHelpCategoryDefinition(Category category)
{
    for (const ShortcutHelpCategoryDefinition &definition : shortcutHelpCategories) {
        if (definition.category == category) {
            return definition;
        }
    }

    return shortcutHelpCategories.back();
}
}

namespace kiriview::ApplicationActions {
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

QString shortcutHelpCategoryKey(ShortcutHelpCategory category)
{
    return latin1String(shortcutHelpCategoryDefinition(category).key);
}

QString shortcutHelpCategoryText(ShortcutHelpCategory category)
{
    return localizedString(shortcutHelpCategoryDefinition(category).text);
}

int shortcutHelpCategoryOrder(ShortcutHelpCategory category)
{
    return shortcutHelpCategoryDefinition(category).order;
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

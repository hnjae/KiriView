// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplicationactions.h"

namespace {
namespace Actions = KiriView::ApplicationActions;

constexpr Actions::DefaultShortcutSpec noDefaultShortcuts() { return {}; }

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

constexpr Actions::ActionDefinition existingAction(KiriViewApplication::ActionId actionId,
    const char *name, Actions::DefaultShortcutSpec defaultShortcuts)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Existing,
        KStandardActions::Open, {}, nullptr, defaultShortcuts };
}

constexpr Actions::ActionDefinition inheritedAction(
    KiriViewApplication::ActionId actionId, const char *name)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Inherited,
        KStandardActions::Open, {}, nullptr, noDefaultShortcuts() };
}

constexpr Actions::ActionDefinition registeredAction(KiriViewApplication::ActionId actionId,
    const char *name, KLazyLocalizedString text, const char *iconName,
    Actions::DefaultShortcutSpec defaultShortcuts)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Registered,
        KStandardActions::Open, text, iconName, defaultShortcuts };
}

constexpr Actions::ActionDefinition standardAction(KiriViewApplication::ActionId actionId,
    const char *name, KStandardActions::StandardAction actionType, KLazyLocalizedString text,
    Actions::DefaultShortcutSpec defaultShortcuts)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::Standard,
        actionType, text, nullptr, defaultShortcuts };
}

constexpr Actions::ActionDefinition showMenubarAction(KiriViewApplication::ActionId actionId,
    const char *name, KStandardActions::StandardAction actionType, KLazyLocalizedString text,
    Actions::DefaultShortcutSpec defaultShortcuts)
{
    return Actions::ActionDefinition { actionId, name, Actions::RegistrationKind::ShowMenubar,
        actionType, text, nullptr, defaultShortcuts };
}

constexpr std::array actionDefinitions {
    standardAction(KiriViewApplication::FileOpenAction, "file_open", KStandardActions::Open,
        kli18n("Open"), standardShortcutSpec(QKeySequence::Open)),
    standardAction(KiriViewApplication::FileMoveToTrashAction, "movetotrash",
        KStandardActions::MoveToTrash, kli18nc("@action", "Move to Trash"),
        portableShortcutSpec("Delete")),
    standardAction(KiriViewApplication::FileDeleteAction, "deletefile",
        KStandardActions::DeleteFile, kli18nc("@action", "Delete Permanently"),
        portableShortcutSpec("Shift+Delete")),
    registeredAction(KiriViewApplication::GoPreviousArchiveAction, "go_previous_archive",
        kli18nc("@action", "Previous Archive"), "go-previous-use", portableShortcutSpec("Ctrl+[")),
    registeredAction(KiriViewApplication::GoNextArchiveAction, "go_next_archive",
        kli18nc("@action", "Next Archive"), "go-next-use", portableShortcutSpec("Ctrl+]")),
    registeredAction(KiriViewApplication::GoPreviousImageAction, "go_previous_image",
        kli18nc("@action", "Previous"), "go-previous",
        standardShortcutSpec(QKeySequence::MoveToPreviousPage)),
    registeredAction(KiriViewApplication::GoNextImageAction, "go_next_image",
        kli18nc("@action", "Next"), "go-next", standardShortcutSpec(QKeySequence::MoveToNextPage)),
    registeredAction(KiriViewApplication::GoFirstImageAction, "go_first_image",
        kli18nc("@action", "First Image"), "go-first-symbolic",
        portableShortcutSpec("Ctrl+Home", "Home")),
    registeredAction(KiriViewApplication::GoLastImageAction, "go_last_image",
        kli18nc("@action", "Last Image"), "go-last-symbolic",
        portableShortcutSpec("Ctrl+End", "End")),
    standardAction(KiriViewApplication::ViewZoomInAction, "view_zoom_in", KStandardActions::ZoomIn,
        kli18nc("@action", "Zoom In"), portableShortcutSpec("Ctrl+=", "Ctrl++")),
    standardAction(KiriViewApplication::ViewZoomOutAction, "view_zoom_out",
        KStandardActions::ZoomOut, kli18nc("@action", "Zoom Out"), portableShortcutSpec("Ctrl+-")),
    standardAction(KiriViewApplication::ViewFitAction, "view_fit", KStandardActions::FitToPage,
        kli18nc("@action", "Fit"), portableShortcutSpec("Ctrl+1")),
    standardAction(KiriViewApplication::ViewFitHeightAction, "view_fit_height",
        KStandardActions::FitToHeight, kli18nc("@action", "Fit Height"),
        portableShortcutSpec("Ctrl+2")),
    standardAction(KiriViewApplication::ViewFitWidthAction, "view_fit_width",
        KStandardActions::FitToWidth, kli18nc("@action", "Fit Width"),
        portableShortcutSpec("Ctrl+3")),
    standardAction(KiriViewApplication::ViewActualSizeAction, "view_actual_size",
        KStandardActions::ActualSize, kli18nc("@action", "Actual Size"),
        portableShortcutSpec("Ctrl+0")),
    registeredAction(KiriViewApplication::ViewRotateClockwiseAction, "view_rotate_clockwise",
        kli18nc("@action", "Rotate Clockwise"), "object-rotate-right-symbolic",
        portableShortcutSpec("Ctrl+R")),
    registeredAction(KiriViewApplication::ViewRotateCounterclockwiseAction,
        "view_rotate_counterclockwise", kli18nc("@action", "Rotate Counterclockwise"),
        "object-rotate-left-symbolic", portableShortcutSpec("Ctrl+Shift+R")),
    registeredAction(KiriViewApplication::ViewToggleTwoPageModeAction, "view_toggle_two_page_mode",
        kli18nc("@action", "Two-Page Spread"), "view-split-left-right-symbolic",
        portableShortcutSpec("Ctrl+S")),
    registeredAction(KiriViewApplication::ViewToggleRightToLeftReadingAction,
        "view_toggle_right_to_left_reading", kli18nc("@action", "Right-to-Left Reading"),
        "format-text-direction-rtl-symbolic", portableShortcutSpec("Ctrl+B")),
    registeredAction(KiriViewApplication::ViewPanTopLeftAction, "view_pan_top_left",
        kli18nc("@action", "Top Left"), nullptr, portableShortcutSpec("Ctrl+<")),
    registeredAction(KiriViewApplication::ViewPanBottomRightAction, "view_pan_bottom_right",
        kli18nc("@action", "Bottom Right"), nullptr, portableShortcutSpec("Ctrl+>")),
    registeredAction(KiriViewApplication::ViewScanForwardAction, "view_scan_forward",
        kli18nc("@action", "Scan Forward"), nullptr, portableShortcutSpec("Ctrl+.")),
    registeredAction(KiriViewApplication::ViewScanBackwardAction, "view_scan_backward",
        kli18nc("@action", "Scan Backward"), nullptr, portableShortcutSpec("Ctrl+,")),
    standardAction(KiriViewApplication::WindowFullscreenAction, "window_fullscreen",
        KStandardActions::FullScreen, kli18nc("@action", "Fullscreen"),
        portableShortcutSpec("Ctrl+F", "F11")),
    registeredAction(KiriViewApplication::HelpShortcutsAction, "help_shortcuts",
        kli18nc("@action", "Keyboard Shortcuts"), "help-keyboard-shortcuts-symbolic",
        portableShortcutSpec("Ctrl+?", "F1")),
    inheritedAction(
        KiriViewApplication::OptionsConfigureKeybindingAction, "options_configure_keybinding"),
    showMenubarAction(KiriViewApplication::OptionsShowMenubarAction, "options_show_menubar",
        KStandardActions::ShowMenubar, kli18nc("@action", "Show Menubar"), noDefaultShortcuts()),
    existingAction(
        KiriViewApplication::FileQuitAction, "file_quit", portableShortcutSpec("Ctrl+Q")),
};

static_assert(actionDefinitions.size() == Actions::actionDefinitionCount);

constexpr bool actionDefinitionsFollowActionIdOrder()
{
    for (std::size_t index = 0; index < actionDefinitions.size(); ++index) {
        if (actionDefinitions[index].actionId
            != static_cast<KiriViewApplication::ActionId>(index)) {
            return false;
        }
    }

    return true;
}

static_assert(actionDefinitionsFollowActionIdOrder());

bool actionIdInRange(KiriViewApplication::ActionId actionId)
{
    const int index = static_cast<int>(actionId);
    return index >= 0 && index < static_cast<int>(Actions::actionDefinitionCount);
}

const Actions::ActionDefinition *definitionForId(KiriViewApplication::ActionId actionId)
{
    if (!actionIdInRange(actionId)) {
        return nullptr;
    }

    return &actionDefinitions[static_cast<std::size_t>(actionId)];
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

QString actionName(KiriViewApplication::ActionId actionId)
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

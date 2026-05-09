// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplicationactions.h"

#include <QStringList>

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
        kli18nc("@action", "Previous Archive"), "go-previous-symbolic", portableShortcutSpec("[")),
    registeredAction(KiriViewApplication::GoNextArchiveAction, "go_next_archive",
        kli18nc("@action", "Next Archive"), "go-next-symbolic", portableShortcutSpec("]")),
    registeredAction(KiriViewApplication::GoPreviousImageAction, "go_previous_image",
        kli18nc("@action", "Previous"), "go-up-symbolic",
        standardShortcutSpec(QKeySequence::MoveToPreviousPage)),
    registeredAction(KiriViewApplication::GoNextImageAction, "go_next_image",
        kli18nc("@action", "Next"), "go-down-symbolic",
        standardShortcutSpec(QKeySequence::MoveToNextPage)),
    registeredAction(KiriViewApplication::GoFirstImageAction, "go_first_image",
        kli18nc("@action", "First Image"), "go-first-symbolic",
        portableShortcutSpec("Ctrl+Home", "Home")),
    registeredAction(KiriViewApplication::GoLastImageAction, "go_last_image",
        kli18nc("@action", "Last Image"), "go-last-symbolic",
        portableShortcutSpec("Ctrl+End", "End")),
    standardAction(KiriViewApplication::ViewZoomInAction, "view_zoom_in", KStandardActions::ZoomIn,
        kli18nc("@action", "Zoom In"), portableShortcutSpec("Ctrl+=", "Ctrl++", "=", "+")),
    standardAction(KiriViewApplication::ViewZoomOutAction, "view_zoom_out",
        KStandardActions::ZoomOut, kli18nc("@action", "Zoom Out"),
        portableShortcutSpec("-", "Ctrl+-")),
    standardAction(KiriViewApplication::ViewFitAction, "view_fit", KStandardActions::FitToPage,
        kli18nc("@action", "Fit"), portableShortcutSpec("1")),
    standardAction(KiriViewApplication::ViewFitHeightAction, "view_fit_height",
        KStandardActions::FitToHeight, kli18nc("@action", "Fit Height"), portableShortcutSpec("2")),
    standardAction(KiriViewApplication::ViewFitWidthAction, "view_fit_width",
        KStandardActions::FitToWidth, kli18nc("@action", "Fit Width"), portableShortcutSpec("3")),
    standardAction(KiriViewApplication::ViewActualSizeAction, "view_actual_size",
        KStandardActions::ActualSize, kli18nc("@action", "Actual Size"), portableShortcutSpec("0")),
    registeredAction(KiriViewApplication::ViewToggleTwoPageModeAction, "view_toggle_two_page_mode",
        kli18nc("@action", "Two-Page Mode"), "view-split-left-right-symbolic",
        portableShortcutSpec("D")),
    registeredAction(KiriViewApplication::ViewToggleRightToLeftReadingAction,
        "view_toggle_right_to_left_reading", kli18nc("@action", "Right-to-Left Reading"),
        "format-text-direction-rtl-symbolic", portableShortcutSpec("B")),
    registeredAction(KiriViewApplication::ViewPanLeftAction, "view_pan_left",
        kli18nc("@action", "Pan Left"), nullptr, portableShortcutSpec("Left")),
    registeredAction(KiriViewApplication::ViewPanRightAction, "view_pan_right",
        kli18nc("@action", "Pan Right"), nullptr, portableShortcutSpec("Right")),
    registeredAction(KiriViewApplication::ViewPanUpAction, "view_pan_up",
        kli18nc("@action", "Pan Up"), nullptr, portableShortcutSpec("Up")),
    registeredAction(KiriViewApplication::ViewPanDownAction, "view_pan_down",
        kli18nc("@action", "Pan Down"), nullptr, portableShortcutSpec("Down")),
    registeredAction(KiriViewApplication::ViewPanTopLeftAction, "view_pan_top_left",
        kli18nc("@action", "Top Left"), nullptr, portableShortcutSpec("<")),
    registeredAction(KiriViewApplication::ViewPanBottomRightAction, "view_pan_bottom_right",
        kli18nc("@action", "Bottom Right"), nullptr, portableShortcutSpec(">")),
    registeredAction(KiriViewApplication::ViewScanForwardAction, "view_scan_forward",
        kli18nc("@action", "Scan Forward"), nullptr, portableShortcutSpec(".")),
    registeredAction(KiriViewApplication::ViewScanBackwardAction, "view_scan_backward",
        kli18nc("@action", "Scan Backward"), nullptr, portableShortcutSpec(",")),
    registeredAction(KiriViewApplication::GoPreviousSinglePageAction, "go_previous_single_page",
        kli18nc("@action", "Previous Page"), "go-previous-symbolic", portableShortcutSpec("P")),
    registeredAction(KiriViewApplication::GoNextSinglePageAction, "go_next_single_page",
        kli18nc("@action", "Next Page"), "go-next-symbolic", portableShortcutSpec("N")),
    standardAction(KiriViewApplication::WindowFullscreenAction, "window_fullscreen",
        KStandardActions::FullScreen, kli18nc("@action", "Fullscreen"),
        portableShortcutSpec("F", "F11")),
    registeredAction(KiriViewApplication::HelpShortcutsAction, "help_shortcuts",
        kli18nc("@action", "Keyboard Shortcuts"), "help-keyboard-shortcuts-symbolic",
        portableShortcutSpec("?", "F1")),
    inheritedAction(KiriViewApplication::OptionsConfigureAction, "options_configure"),
    inheritedAction(
        KiriViewApplication::OptionsConfigureKeybindingAction, "options_configure_keybinding"),
    showMenubarAction(KiriViewApplication::OptionsShowMenubarAction, "options_show_menubar",
        KStandardActions::ShowMenubar, kli18nc("@action", "Show Menubar"), noDefaultShortcuts()),
    existingAction(
        KiriViewApplication::FileQuitAction, "file_quit", portableShortcutSpec("Q", "Ctrl+Q")),
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
}

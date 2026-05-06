// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplication.h"

#include "kiriviewsettings.h"

#include <KirigamiActionCollection>
#include <QIcon>
#include <QSignalBlocker>
#include <array>
#include <cstddef>

namespace {
enum class ActionRegistrationKind {
    Existing,
    Inherited,
    Registered,
    ShowMenubar,
    Standard,
};

enum class ShortcutSpecKind {
    None,
    PortableText,
    StandardKey,
};

constexpr std::size_t maxPortableShortcutCount = 4;

struct DefaultShortcutSpec {
    ShortcutSpecKind kind = ShortcutSpecKind::None;
    QKeySequence::StandardKey standardKey = QKeySequence::UnknownKey;
    std::array<const char *, maxPortableShortcutCount> portableText {};
    std::size_t portableTextCount = 0;
};

struct ActionDefinition {
    KiriViewApplication::ActionId actionId;
    const char *name;
    ActionRegistrationKind kind;
    KStandardActions::StandardAction actionType;
    const char *text;
    const char *iconName;
    DefaultShortcutSpec defaultShortcuts;
};

constexpr DefaultShortcutSpec noDefaultShortcuts() { return {}; }

constexpr DefaultShortcutSpec standardShortcutSpec(QKeySequence::StandardKey key)
{
    return DefaultShortcutSpec {
        ShortcutSpecKind::StandardKey,
        key,
        {},
        0,
    };
}

template <typename... Sequences>
constexpr DefaultShortcutSpec portableShortcutSpec(Sequences... sequences)
{
    static_assert(sizeof...(Sequences) <= maxPortableShortcutCount);
    return DefaultShortcutSpec {
        ShortcutSpecKind::PortableText,
        QKeySequence::UnknownKey,
        { sequences... },
        sizeof...(Sequences),
    };
}

constexpr ActionDefinition existingAction(
    KiriViewApplication::ActionId actionId, const char *name, DefaultShortcutSpec defaultShortcuts)
{
    return ActionDefinition { actionId, name, ActionRegistrationKind::Existing,
        KStandardActions::Open, nullptr, nullptr, defaultShortcuts };
}

constexpr ActionDefinition inheritedAction(KiriViewApplication::ActionId actionId, const char *name)
{
    return ActionDefinition { actionId, name, ActionRegistrationKind::Inherited,
        KStandardActions::Open, nullptr, nullptr, noDefaultShortcuts() };
}

constexpr ActionDefinition registeredAction(KiriViewApplication::ActionId actionId,
    const char *name, const char *text, const char *iconName, DefaultShortcutSpec defaultShortcuts)
{
    return ActionDefinition { actionId, name, ActionRegistrationKind::Registered,
        KStandardActions::Open, text, iconName, defaultShortcuts };
}

constexpr ActionDefinition standardAction(KiriViewApplication::ActionId actionId, const char *name,
    KStandardActions::StandardAction actionType, const char *text,
    DefaultShortcutSpec defaultShortcuts)
{
    return ActionDefinition { actionId, name, ActionRegistrationKind::Standard, actionType, text,
        nullptr, defaultShortcuts };
}

constexpr ActionDefinition showMenubarAction(KiriViewApplication::ActionId actionId,
    const char *name, KStandardActions::StandardAction actionType, const char *text,
    DefaultShortcutSpec defaultShortcuts)
{
    return ActionDefinition { actionId, name, ActionRegistrationKind::ShowMenubar, actionType, text,
        nullptr, defaultShortcuts };
}

constexpr std::array actionDefinitions {
    standardAction(KiriViewApplication::FileOpenAction, "file_open", KStandardActions::Open, "Open",
        standardShortcutSpec(QKeySequence::Open)),
    existingAction(
        KiriViewApplication::FileQuitAction, "file_quit", portableShortcutSpec("Q", "Ctrl+Q")),
    registeredAction(KiriViewApplication::GoPreviousArchiveAction, "go_previous_archive",
        "Previous Archive", "go-previous-symbolic", portableShortcutSpec("[")),
    registeredAction(KiriViewApplication::GoNextArchiveAction, "go_next_archive", "Next Archive",
        "go-next-symbolic", portableShortcutSpec("]")),
    registeredAction(KiriViewApplication::GoPreviousImageAction, "go_previous_image", "Previous",
        "go-up-symbolic", standardShortcutSpec(QKeySequence::MoveToPreviousPage)),
    registeredAction(KiriViewApplication::GoNextImageAction, "go_next_image", "Next",
        "go-down-symbolic", standardShortcutSpec(QKeySequence::MoveToNextPage)),
    registeredAction(KiriViewApplication::GoFirstImageAction, "go_first_image", "First Image",
        "go-first-symbolic", portableShortcutSpec("Ctrl+Home", "Home")),
    registeredAction(KiriViewApplication::GoLastImageAction, "go_last_image", "Last Image",
        "go-last-symbolic", portableShortcutSpec("Ctrl+End", "End")),
    standardAction(KiriViewApplication::ViewZoomInAction, "view_zoom_in", KStandardActions::ZoomIn,
        "Zoom In", portableShortcutSpec("Ctrl+=", "Ctrl++", "=", "+")),
    standardAction(KiriViewApplication::ViewZoomOutAction, "view_zoom_out",
        KStandardActions::ZoomOut, "Zoom Out", portableShortcutSpec("-", "Ctrl+-")),
    standardAction(KiriViewApplication::ViewFitAction, "view_fit", KStandardActions::FitToPage,
        "Fit", portableShortcutSpec("1")),
    standardAction(KiriViewApplication::ViewFitHeightAction, "view_fit_height",
        KStandardActions::FitToHeight, "Fit Height", portableShortcutSpec("2")),
    standardAction(KiriViewApplication::ViewFitWidthAction, "view_fit_width",
        KStandardActions::FitToWidth, "Fit Width", portableShortcutSpec("3")),
    standardAction(KiriViewApplication::ViewActualSizeAction, "view_actual_size",
        KStandardActions::ActualSize, "Actual Size", portableShortcutSpec("0")),
    registeredAction(KiriViewApplication::ViewPanLeftAction, "view_pan_left", "Pan Left", nullptr,
        portableShortcutSpec("Left")),
    registeredAction(KiriViewApplication::ViewPanRightAction, "view_pan_right", "Pan Right",
        nullptr, portableShortcutSpec("Right")),
    registeredAction(KiriViewApplication::ViewPanUpAction, "view_pan_up", "Pan Up", nullptr,
        portableShortcutSpec("Up")),
    registeredAction(KiriViewApplication::ViewPanDownAction, "view_pan_down", "Pan Down", nullptr,
        portableShortcutSpec("Down")),
    registeredAction(KiriViewApplication::ViewPanTopLeftAction, "view_pan_top_left", "Top Left",
        nullptr, portableShortcutSpec("<")),
    registeredAction(KiriViewApplication::ViewPanBottomRightAction, "view_pan_bottom_right",
        "Bottom Right", nullptr, portableShortcutSpec(">")),
    registeredAction(KiriViewApplication::ViewScanForwardAction, "view_scan_forward",
        "Scan Forward", nullptr, portableShortcutSpec(".")),
    registeredAction(KiriViewApplication::ViewScanBackwardAction, "view_scan_backward",
        "Scan Backward", nullptr, portableShortcutSpec(",")),
    standardAction(KiriViewApplication::WindowFullscreenAction, "window_fullscreen",
        KStandardActions::FullScreen, "Fullscreen", portableShortcutSpec("F", "F11")),
    registeredAction(KiriViewApplication::HelpShortcutsAction, "help_shortcuts",
        "Keyboard Shortcuts", "help-keyboard-shortcuts-symbolic", portableShortcutSpec("?", "F1")),
    inheritedAction(KiriViewApplication::OptionsConfigureAction, "options_configure"),
    inheritedAction(
        KiriViewApplication::OptionsConfigureKeybindingAction, "options_configure_keybinding"),
    showMenubarAction(KiriViewApplication::OptionsShowMenubarAction, "options_show_menubar",
        KStandardActions::ShowMenubar, "Show Menubar", noDefaultShortcuts()),
};

static_assert(
    actionDefinitions.size() == static_cast<std::size_t>(KiriViewApplication::FileQuitAction) + 1);

constexpr bool actionDefinitionsContainActionId(KiriViewApplication::ActionId actionId)
{
    for (const ActionDefinition &definition : actionDefinitions) {
        if (definition.actionId == actionId) {
            return true;
        }
    }

    return false;
}

constexpr bool actionDefinitionsCoverActionIds()
{
    for (std::size_t index = 0; index < actionDefinitions.size(); ++index) {
        if (!actionDefinitionsContainActionId(static_cast<KiriViewApplication::ActionId>(index))) {
            return false;
        }
    }

    return true;
}

constexpr bool actionDefinitionsHaveUniqueActionIds()
{
    for (std::size_t left = 0; left < actionDefinitions.size(); ++left) {
        for (std::size_t right = left + 1; right < actionDefinitions.size(); ++right) {
            if (actionDefinitions[left].actionId == actionDefinitions[right].actionId) {
                return false;
            }
        }
    }

    return true;
}

static_assert(actionDefinitionsCoverActionIds());
static_assert(actionDefinitionsHaveUniqueActionIds());

const ActionDefinition *actionDefinitionForId(KiriViewApplication::ActionId actionId)
{
    for (const ActionDefinition &definition : actionDefinitions) {
        if (definition.actionId == actionId) {
            return &definition;
        }
    }

    return nullptr;
}

QString latin1String(const char *text)
{
    return text == nullptr ? QString() : QString::fromLatin1(text);
}

QString applicationActionName(KiriViewApplication::ActionId actionId)
{
    const ActionDefinition *definition = actionDefinitionForId(actionId);
    return definition == nullptr ? QString() : QString::fromLatin1(definition->name);
}

QKeySequence shortcut(const char *sequence)
{
    return QKeySequence::fromString(QString::fromLatin1(sequence), QKeySequence::PortableText);
}

QList<QKeySequence> portableShortcutList(
    const std::array<const char *, maxPortableShortcutCount> &sequences, std::size_t count)
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

KiriViewApplication::MenuPresentation toMenuPresentation(int value)
{
    switch (value) {
    case KiriViewApplication::MenuBar:
        return KiriViewApplication::MenuBar;
    case KiriViewApplication::HamburgerMenu:
    default:
        return KiriViewApplication::HamburgerMenu;
    }
}
}

KiriViewApplication::KiriViewApplication(QObject *parent)
    : AbstractKirigamiApplication(parent)
{
    KiriViewApplication::setupActions();
}

KiriViewApplication::~KiriViewApplication() = default;

KiriViewApplication::MenuPresentation KiriViewApplication::menuPresentation() const
{
    return toMenuPresentation(KiriViewSettings::menuPresentation());
}

void KiriViewApplication::setMenuPresentation(MenuPresentation presentation)
{
    if (menuPresentation() == presentation) {
        return;
    }

    KiriViewSettings::setMenuPresentation(static_cast<int>(presentation));
    KiriViewSettings::self()->save();
    updateShowMenuBarAction();
    Q_EMIT menuPresentationChanged();
}

int KiriViewApplication::shortcutRevision() const { return m_shortcutRevision; }

QAction *KiriViewApplication::action(const QString &actionName)
{
    return AbstractKirigamiApplication::action(actionName);
}

QAction *KiriViewApplication::actionForId(ActionId actionId)
{
    const QString name = actionName(actionId);
    if (name.isEmpty()) {
        return nullptr;
    }

    return action(name);
}

QString KiriViewApplication::actionName(ActionId actionId) const
{
    return applicationActionName(actionId);
}

QList<QKeySequence> KiriViewApplication::shortcuts(const QString &actionName) const
{
    const QAction *registeredAction = const_cast<KiriViewApplication *>(this)->action(actionName);
    if (!registeredAction) {
        return {};
    }

    return registeredAction->shortcuts();
}

QList<QKeySequence> KiriViewApplication::shortcutsForId(ActionId actionId) const
{
    const QString name = actionName(actionId);
    if (name.isEmpty()) {
        return {};
    }

    return shortcuts(name);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return filterShortcutsByCommandModifier(shortcuts(actionName), true);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifierForId(ActionId actionId) const
{
    return filterShortcutsByCommandModifier(shortcutsForId(actionId), true);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return filterShortcutsByCommandModifier(shortcuts(actionName), false);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifierForId(
    ActionId actionId) const
{
    return filterShortcutsByCommandModifier(shortcutsForId(actionId), false);
}

QString KiriViewApplication::shortcutText(const QString &actionName) const
{
    return shortcutListText(shortcuts(actionName));
}

QString KiriViewApplication::shortcutTextForId(ActionId actionId) const
{
    return shortcutListText(shortcutsForId(actionId));
}

void KiriViewApplication::setupActions()
{
    AbstractKirigamiApplication::setupActions();
    mainCollection()->setComponentDisplayName(QStringLiteral("KiriView"));

    const auto addAction = [this](const ActionDefinition &definition) {
        const QString name = QString::fromLatin1(definition.name);
        const QList<QKeySequence> shortcuts = defaultShortcuts(definition.defaultShortcuts);

        switch (definition.kind) {
        case ActionRegistrationKind::Existing:
            if (QAction *registeredAction = action(name)) {
                finishRegisteredAction(registeredAction, registeredAction->text(), shortcuts);
            }
            return;
        case ActionRegistrationKind::Inherited:
            return;
        case ActionRegistrationKind::Registered:
            addRegisteredAction(
                name, latin1String(definition.text), latin1String(definition.iconName), shortcuts);
            return;
        case ActionRegistrationKind::ShowMenubar:
            m_showMenuBarAction = addStandardAction(
                definition.actionType, name, latin1String(definition.text), shortcuts);
            m_showMenuBarAction->setCheckable(true);
            connect(m_showMenuBarAction, &QAction::triggered, this,
                [this](bool checked) { setMenuPresentation(checked ? MenuBar : HamburgerMenu); });
            return;
        case ActionRegistrationKind::Standard:
            addStandardAction(
                definition.actionType, name, latin1String(definition.text), shortcuts);
            return;
        }
    };

    for (const ActionDefinition &definition : actionDefinitions) {
        addAction(definition);
    }

    readSettings();
    updateShowMenuBarAction();
}

QAction *KiriViewApplication::addRegisteredAction(const QString &name, const QString &text,
    const QString &iconName, const QList<QKeySequence> &defaultShortcuts)
{
    auto *registeredAction = new QAction(this);
    registeredAction->setObjectName(name);
    registeredAction->setText(text);
    if (!iconName.isEmpty()) {
        registeredAction->setIcon(QIcon::fromTheme(iconName));
    }

    mainCollection()->addAction(name, registeredAction);
    KirigamiActionCollection::setDefaultShortcuts(registeredAction, defaultShortcuts);
    connect(registeredAction, &QAction::changed, this, &KiriViewApplication::handleActionChanged);
    return registeredAction;
}

QAction *KiriViewApplication::addStandardAction(KStandardActions::StandardAction actionType,
    const QString &name, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    QAction *registeredAction = mainCollection()->addAction(actionType, name, this, [](bool) { });
    return finishRegisteredAction(registeredAction, text, defaultShortcuts);
}

QAction *KiriViewApplication::finishRegisteredAction(
    QAction *registeredAction, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    registeredAction->setText(text);
    KirigamiActionCollection::setDefaultShortcuts(registeredAction, defaultShortcuts);
    connect(registeredAction, &QAction::changed, this, &KiriViewApplication::handleActionChanged);
    return registeredAction;
}

void KiriViewApplication::handleActionChanged()
{
    ++m_shortcutRevision;
    Q_EMIT shortcutRevisionChanged();
}

void KiriViewApplication::updateShowMenuBarAction()
{
    if (!m_showMenuBarAction) {
        return;
    }

    const QSignalBlocker blocker(m_showMenuBarAction);
    m_showMenuBarAction->setChecked(menuPresentation() == MenuBar);
}

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

constexpr std::array actionDefinitions {
    ActionDefinition { KiriViewApplication::FileOpenAction, "file_open",
        ActionRegistrationKind::Standard, KStandardActions::Open, "Open", nullptr,
        standardShortcutSpec(QKeySequence::Open) },
    ActionDefinition { KiriViewApplication::FileQuitAction, "file_quit",
        ActionRegistrationKind::Existing, KStandardActions::Open, nullptr, nullptr,
        portableShortcutSpec("Q", "Ctrl+Q") },
    ActionDefinition { KiriViewApplication::GoPreviousArchiveAction, "go_previous_archive",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Previous Archive",
        "go-previous-symbolic", portableShortcutSpec("[") },
    ActionDefinition { KiriViewApplication::GoNextArchiveAction, "go_next_archive",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Next Archive",
        "go-next-symbolic", portableShortcutSpec("]") },
    ActionDefinition { KiriViewApplication::GoPreviousImageAction, "go_previous_image",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Previous", "go-up-symbolic",
        standardShortcutSpec(QKeySequence::MoveToPreviousPage) },
    ActionDefinition { KiriViewApplication::GoNextImageAction, "go_next_image",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Next", "go-down-symbolic",
        standardShortcutSpec(QKeySequence::MoveToNextPage) },
    ActionDefinition { KiriViewApplication::GoFirstImageAction, "go_first_image",
        ActionRegistrationKind::Registered, KStandardActions::Open, "First Image",
        "go-first-symbolic", portableShortcutSpec("Ctrl+Home", "Home") },
    ActionDefinition { KiriViewApplication::GoLastImageAction, "go_last_image",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Last Image",
        "go-last-symbolic", portableShortcutSpec("Ctrl+End", "End") },
    ActionDefinition { KiriViewApplication::ViewZoomInAction, "view_zoom_in",
        ActionRegistrationKind::Standard, KStandardActions::ZoomIn, "Zoom In", nullptr,
        portableShortcutSpec("Ctrl+=", "Ctrl++", "=", "+") },
    ActionDefinition { KiriViewApplication::ViewZoomOutAction, "view_zoom_out",
        ActionRegistrationKind::Standard, KStandardActions::ZoomOut, "Zoom Out", nullptr,
        portableShortcutSpec("-", "Ctrl+-") },
    ActionDefinition { KiriViewApplication::ViewFitAction, "view_fit",
        ActionRegistrationKind::Standard, KStandardActions::FitToPage, "Fit", nullptr,
        portableShortcutSpec("1") },
    ActionDefinition { KiriViewApplication::ViewFitHeightAction, "view_fit_height",
        ActionRegistrationKind::Standard, KStandardActions::FitToHeight, "Fit Height", nullptr,
        portableShortcutSpec("2") },
    ActionDefinition { KiriViewApplication::ViewFitWidthAction, "view_fit_width",
        ActionRegistrationKind::Standard, KStandardActions::FitToWidth, "Fit Width", nullptr,
        portableShortcutSpec("3") },
    ActionDefinition { KiriViewApplication::ViewActualSizeAction, "view_actual_size",
        ActionRegistrationKind::Standard, KStandardActions::ActualSize, "Actual Size", nullptr,
        portableShortcutSpec("0") },
    ActionDefinition { KiriViewApplication::ViewPanLeftAction, "view_pan_left",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Pan Left", nullptr,
        portableShortcutSpec("Left") },
    ActionDefinition { KiriViewApplication::ViewPanRightAction, "view_pan_right",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Pan Right", nullptr,
        portableShortcutSpec("Right") },
    ActionDefinition { KiriViewApplication::ViewPanUpAction, "view_pan_up",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Pan Up", nullptr,
        portableShortcutSpec("Up") },
    ActionDefinition { KiriViewApplication::ViewPanDownAction, "view_pan_down",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Pan Down", nullptr,
        portableShortcutSpec("Down") },
    ActionDefinition { KiriViewApplication::ViewPanTopLeftAction, "view_pan_top_left",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Top Left", nullptr,
        portableShortcutSpec("<") },
    ActionDefinition { KiriViewApplication::ViewPanBottomRightAction, "view_pan_bottom_right",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Bottom Right", nullptr,
        portableShortcutSpec(">") },
    ActionDefinition { KiriViewApplication::ViewScanForwardAction, "view_scan_forward",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Scan Forward", nullptr,
        portableShortcutSpec(".") },
    ActionDefinition { KiriViewApplication::ViewScanBackwardAction, "view_scan_backward",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Scan Backward", nullptr,
        portableShortcutSpec(",") },
    ActionDefinition { KiriViewApplication::WindowFullscreenAction, "window_fullscreen",
        ActionRegistrationKind::Standard, KStandardActions::FullScreen, "Fullscreen", nullptr,
        portableShortcutSpec("F", "F11") },
    ActionDefinition { KiriViewApplication::HelpShortcutsAction, "help_shortcuts",
        ActionRegistrationKind::Registered, KStandardActions::Open, "Keyboard Shortcuts",
        "help-keyboard-shortcuts-symbolic", portableShortcutSpec("?", "F1") },
    ActionDefinition { KiriViewApplication::OptionsConfigureAction, "options_configure",
        ActionRegistrationKind::Inherited, KStandardActions::Open, nullptr, nullptr,
        noDefaultShortcuts() },
    ActionDefinition { KiriViewApplication::OptionsConfigureKeybindingAction,
        "options_configure_keybinding", ActionRegistrationKind::Inherited, KStandardActions::Open,
        nullptr, nullptr, noDefaultShortcuts() },
    ActionDefinition { KiriViewApplication::OptionsShowMenubarAction, "options_show_menubar",
        ActionRegistrationKind::ShowMenubar, KStandardActions::ShowMenubar, "Show Menubar", nullptr,
        noDefaultShortcuts() },
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

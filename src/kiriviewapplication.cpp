// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplication.h"

#include "kiriviewsettings.h"

#include <KirigamiActionCollection>
#include <QIcon>
#include <QSignalBlocker>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>

namespace {
enum class ActionSpecKind {
    Registered,
    Standard,
};

struct ActionNameSpec {
    KiriViewApplication::ActionId actionId;
    const char *name;
};

constexpr std::array actionNameSpecs {
    ActionNameSpec { KiriViewApplication::FileOpenAction, "file_open" },
    ActionNameSpec { KiriViewApplication::GoPreviousArchiveAction, "go_previous_archive" },
    ActionNameSpec { KiriViewApplication::GoNextArchiveAction, "go_next_archive" },
    ActionNameSpec { KiriViewApplication::GoPreviousImageAction, "go_previous_image" },
    ActionNameSpec { KiriViewApplication::GoNextImageAction, "go_next_image" },
    ActionNameSpec { KiriViewApplication::GoFirstImageAction, "go_first_image" },
    ActionNameSpec { KiriViewApplication::GoLastImageAction, "go_last_image" },
    ActionNameSpec { KiriViewApplication::ViewZoomInAction, "view_zoom_in" },
    ActionNameSpec { KiriViewApplication::ViewZoomOutAction, "view_zoom_out" },
    ActionNameSpec { KiriViewApplication::ViewFitAction, "view_fit" },
    ActionNameSpec { KiriViewApplication::ViewFitHeightAction, "view_fit_height" },
    ActionNameSpec { KiriViewApplication::ViewFitWidthAction, "view_fit_width" },
    ActionNameSpec { KiriViewApplication::ViewActualSizeAction, "view_actual_size" },
    ActionNameSpec { KiriViewApplication::ViewPanLeftAction, "view_pan_left" },
    ActionNameSpec { KiriViewApplication::ViewPanRightAction, "view_pan_right" },
    ActionNameSpec { KiriViewApplication::ViewPanUpAction, "view_pan_up" },
    ActionNameSpec { KiriViewApplication::ViewPanDownAction, "view_pan_down" },
    ActionNameSpec { KiriViewApplication::ViewPanTopLeftAction, "view_pan_top_left" },
    ActionNameSpec { KiriViewApplication::ViewPanBottomRightAction, "view_pan_bottom_right" },
    ActionNameSpec { KiriViewApplication::ViewScanForwardAction, "view_scan_forward" },
    ActionNameSpec { KiriViewApplication::ViewScanBackwardAction, "view_scan_backward" },
    ActionNameSpec { KiriViewApplication::WindowFullscreenAction, "window_fullscreen" },
    ActionNameSpec { KiriViewApplication::HelpShortcutsAction, "help_shortcuts" },
    ActionNameSpec { KiriViewApplication::OptionsConfigureAction, "options_configure" },
    ActionNameSpec {
        KiriViewApplication::OptionsConfigureKeybindingAction, "options_configure_keybinding" },
    ActionNameSpec { KiriViewApplication::OptionsShowMenubarAction, "options_show_menubar" },
    ActionNameSpec { KiriViewApplication::FileQuitAction, "file_quit" },
};

static_assert(
    actionNameSpecs.size() == static_cast<std::size_t>(KiriViewApplication::FileQuitAction) + 1);

constexpr bool actionNameSpecsMatchActionIds()
{
    for (std::size_t index = 0; index < actionNameSpecs.size(); ++index) {
        if (static_cast<std::size_t>(actionNameSpecs[index].actionId) != index) {
            return false;
        }
    }

    return true;
}

static_assert(actionNameSpecsMatchActionIds());

QString applicationActionName(KiriViewApplication::ActionId actionId)
{
    const int actionIndex = static_cast<int>(actionId);
    if (actionIndex < 0 || actionIndex >= static_cast<int>(actionNameSpecs.size())) {
        return {};
    }

    const ActionNameSpec &spec = actionNameSpecs[static_cast<std::size_t>(actionIndex)];
    if (spec.actionId != actionId) {
        return {};
    }

    return QString::fromLatin1(spec.name);
}

struct ActionSpec {
    static ActionSpec registered(KiriViewApplication::ActionId actionId, QString text,
        QString iconName, QList<QKeySequence> defaultShortcuts)
    {
        return ActionSpec { ActionSpecKind::Registered, KStandardActions::Open,
            applicationActionName(actionId), std::move(text), std::move(iconName),
            std::move(defaultShortcuts) };
    }

    static ActionSpec standard(KiriViewApplication::ActionId actionId,
        KStandardActions::StandardAction actionType, QString text,
        QList<QKeySequence> defaultShortcuts)
    {
        return ActionSpec { ActionSpecKind::Standard, actionType, applicationActionName(actionId),
            std::move(text), QString(), std::move(defaultShortcuts) };
    }

    ActionSpecKind kind;
    KStandardActions::StandardAction actionType;
    QString name;
    QString text;
    QString iconName;
    QList<QKeySequence> defaultShortcuts;
};

QKeySequence shortcut(const QString &sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
}

QList<QKeySequence> shortcutList(std::initializer_list<const char *> sequences)
{
    QList<QKeySequence> shortcutList;
    shortcutList.reserve(static_cast<qsizetype>(sequences.size()));
    for (const char *sequence : sequences) {
        shortcutList.push_back(shortcut(QString::fromLatin1(sequence)));
    }

    return shortcutList;
}

QList<QKeySequence> standardShortcuts(QKeySequence::StandardKey key)
{
    return QKeySequence::keyBindings(key);
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

ActionSpec openActionSpec()
{
    return ActionSpec::standard(KiriViewApplication::FileOpenAction, KStandardActions::Open,
        QStringLiteral("Open"), standardShortcuts(QKeySequence::Open));
}

std::vector<ActionSpec> actionSpecs()
{
    return {
        ActionSpec::registered(KiriViewApplication::GoPreviousArchiveAction,
            QStringLiteral("Previous Archive"), QStringLiteral("go-previous-symbolic"),
            shortcutList({ "[" })),
        ActionSpec::registered(KiriViewApplication::GoNextArchiveAction,
            QStringLiteral("Next Archive"), QStringLiteral("go-next-symbolic"),
            shortcutList({ "]" })),
        ActionSpec::registered(KiriViewApplication::GoPreviousImageAction,
            QStringLiteral("Previous"), QStringLiteral("go-up-symbolic"),
            standardShortcuts(QKeySequence::MoveToPreviousPage)),
        ActionSpec::registered(KiriViewApplication::GoNextImageAction, QStringLiteral("Next"),
            QStringLiteral("go-down-symbolic"), standardShortcuts(QKeySequence::MoveToNextPage)),
        ActionSpec::registered(KiriViewApplication::GoFirstImageAction,
            QStringLiteral("First Image"), QStringLiteral("go-first-symbolic"),
            shortcutList({ "Ctrl+Home", "Home" })),
        ActionSpec::registered(KiriViewApplication::GoLastImageAction, QStringLiteral("Last Image"),
            QStringLiteral("go-last-symbolic"), shortcutList({ "Ctrl+End", "End" })),
        ActionSpec::standard(KiriViewApplication::ViewZoomInAction, KStandardActions::ZoomIn,
            QStringLiteral("Zoom In"), shortcutList({ "Ctrl+=", "Ctrl++", "=", "+" })),
        ActionSpec::standard(KiriViewApplication::ViewZoomOutAction, KStandardActions::ZoomOut,
            QStringLiteral("Zoom Out"), shortcutList({ "-", "Ctrl+-" })),
        ActionSpec::standard(KiriViewApplication::ViewFitAction, KStandardActions::FitToPage,
            QStringLiteral("Fit"), shortcutList({ "1" })),
        ActionSpec::standard(KiriViewApplication::ViewFitHeightAction,
            KStandardActions::FitToHeight, QStringLiteral("Fit Height"), shortcutList({ "2" })),
        ActionSpec::standard(KiriViewApplication::ViewFitWidthAction, KStandardActions::FitToWidth,
            QStringLiteral("Fit Width"), shortcutList({ "3" })),
        ActionSpec::standard(KiriViewApplication::ViewActualSizeAction,
            KStandardActions::ActualSize, QStringLiteral("Actual Size"), shortcutList({ "0" })),
        ActionSpec::registered(KiriViewApplication::ViewPanLeftAction, QStringLiteral("Pan Left"),
            QString(), shortcutList({ "Left" })),
        ActionSpec::registered(KiriViewApplication::ViewPanRightAction, QStringLiteral("Pan Right"),
            QString(), shortcutList({ "Right" })),
        ActionSpec::registered(KiriViewApplication::ViewPanUpAction, QStringLiteral("Pan Up"),
            QString(), shortcutList({ "Up" })),
        ActionSpec::registered(KiriViewApplication::ViewPanDownAction, QStringLiteral("Pan Down"),
            QString(), shortcutList({ "Down" })),
        ActionSpec::registered(KiriViewApplication::ViewPanTopLeftAction,
            QStringLiteral("Top Left"), QString(), shortcutList({ "<" })),
        ActionSpec::registered(KiriViewApplication::ViewPanBottomRightAction,
            QStringLiteral("Bottom Right"), QString(), shortcutList({ ">" })),
        ActionSpec::registered(KiriViewApplication::ViewScanForwardAction,
            QStringLiteral("Scan Forward"), QString(), shortcutList({ "." })),
        ActionSpec::registered(KiriViewApplication::ViewScanBackwardAction,
            QStringLiteral("Scan Backward"), QString(), shortcutList({ "," })),
        ActionSpec::standard(KiriViewApplication::WindowFullscreenAction,
            KStandardActions::FullScreen, QStringLiteral("Fullscreen"),
            shortcutList({ "F", "F11" })),
        ActionSpec::registered(KiriViewApplication::HelpShortcutsAction,
            QStringLiteral("Keyboard Shortcuts"),
            QStringLiteral("help-keyboard-shortcuts-symbolic"), shortcutList({ "?", "F1" })),
    };
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

    const auto addAction = [this](const ActionSpec &spec) {
        if (spec.kind == ActionSpecKind::Standard) {
            addStandardAction(spec.actionType, spec.name, spec.text, spec.defaultShortcuts);
        } else {
            addRegisteredAction(spec.name, spec.text, spec.iconName, spec.defaultShortcuts);
        }
    };

    addAction(openActionSpec());

    if (QAction *quitAction = actionForId(FileQuitAction)) {
        finishRegisteredAction(quitAction, quitAction->text(), shortcutList({ "Q", "Ctrl+Q" }));
    }

    for (const ActionSpec &spec : actionSpecs()) {
        addAction(spec);
    }

    m_showMenuBarAction
        = addStandardAction(KStandardActions::ShowMenubar, actionName(OptionsShowMenubarAction),
            QStringLiteral("Show Menubar"), QList<QKeySequence> {});
    m_showMenuBarAction->setCheckable(true);
    connect(m_showMenuBarAction, &QAction::triggered, this,
        [this](bool checked) { setMenuPresentation(checked ? MenuBar : HamburgerMenu); });

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

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplication.h"

#include "kiriviewsettings.h"

#include <KirigamiActionCollection>
#include <QIcon>
#include <QSignalBlocker>
#include <initializer_list>
#include <utility>
#include <vector>

namespace {
enum class ActionSpecKind {
    Registered,
    Standard,
};

struct ActionSpec {
    static ActionSpec registered(
        QString name, QString text, QString iconName, QList<QKeySequence> defaultShortcuts)
    {
        return ActionSpec { ActionSpecKind::Registered, KStandardActions::Open, std::move(name),
            std::move(text), std::move(iconName), std::move(defaultShortcuts) };
    }

    static ActionSpec standard(KStandardActions::StandardAction actionType, QString name,
        QString text, QList<QKeySequence> defaultShortcuts)
    {
        return ActionSpec { ActionSpecKind::Standard, actionType, std::move(name), std::move(text),
            QString(), std::move(defaultShortcuts) };
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
    return ActionSpec::standard(KStandardActions::Open, QStringLiteral("file_open"),
        QStringLiteral("Open"), standardShortcuts(QKeySequence::Open));
}

std::vector<ActionSpec> actionSpecs()
{
    return {
        ActionSpec::registered(QStringLiteral("go_previous_archive"),
            QStringLiteral("Previous Archive"), QStringLiteral("go-previous-symbolic"),
            shortcutList({ "[" })),
        ActionSpec::registered(QStringLiteral("go_next_archive"), QStringLiteral("Next Archive"),
            QStringLiteral("go-next-symbolic"), shortcutList({ "]" })),
        ActionSpec::registered(QStringLiteral("go_previous_image"), QStringLiteral("Previous"),
            QStringLiteral("go-up-symbolic"), standardShortcuts(QKeySequence::MoveToPreviousPage)),
        ActionSpec::registered(QStringLiteral("go_next_image"), QStringLiteral("Next"),
            QStringLiteral("go-down-symbolic"), standardShortcuts(QKeySequence::MoveToNextPage)),
        ActionSpec::registered(QStringLiteral("go_first_image"), QStringLiteral("First Image"),
            QStringLiteral("go-first-symbolic"), shortcutList({ "Ctrl+Home", "Home" })),
        ActionSpec::registered(QStringLiteral("go_last_image"), QStringLiteral("Last Image"),
            QStringLiteral("go-last-symbolic"), shortcutList({ "Ctrl+End", "End" })),
        ActionSpec::standard(KStandardActions::ZoomIn, QStringLiteral("view_zoom_in"),
            QStringLiteral("Zoom In"), shortcutList({ "Ctrl+=", "Ctrl++", "=", "+" })),
        ActionSpec::standard(KStandardActions::ZoomOut, QStringLiteral("view_zoom_out"),
            QStringLiteral("Zoom Out"), shortcutList({ "-", "Ctrl+-" })),
        ActionSpec::standard(KStandardActions::FitToPage, QStringLiteral("view_fit"),
            QStringLiteral("Fit"), shortcutList({ "1" })),
        ActionSpec::standard(KStandardActions::FitToHeight, QStringLiteral("view_fit_height"),
            QStringLiteral("Fit Height"), shortcutList({ "2" })),
        ActionSpec::standard(KStandardActions::FitToWidth, QStringLiteral("view_fit_width"),
            QStringLiteral("Fit Width"), shortcutList({ "3" })),
        ActionSpec::standard(KStandardActions::ActualSize, QStringLiteral("view_actual_size"),
            QStringLiteral("Actual Size"), shortcutList({ "0" })),
        ActionSpec::registered(QStringLiteral("view_pan_left"), QStringLiteral("Pan Left"),
            QString(), shortcutList({ "Left" })),
        ActionSpec::registered(QStringLiteral("view_pan_right"), QStringLiteral("Pan Right"),
            QString(), shortcutList({ "Right" })),
        ActionSpec::registered(QStringLiteral("view_pan_up"), QStringLiteral("Pan Up"), QString(),
            shortcutList({ "Up" })),
        ActionSpec::registered(QStringLiteral("view_pan_down"), QStringLiteral("Pan Down"),
            QString(), shortcutList({ "Down" })),
        ActionSpec::registered(QStringLiteral("view_pan_top_left"), QStringLiteral("Top Left"),
            QString(), shortcutList({ "<" })),
        ActionSpec::registered(QStringLiteral("view_pan_bottom_right"),
            QStringLiteral("Bottom Right"), QString(), shortcutList({ ">" })),
        ActionSpec::registered(QStringLiteral("view_scan_forward"), QStringLiteral("Scan Forward"),
            QString(), shortcutList({ "." })),
        ActionSpec::registered(QStringLiteral("view_scan_backward"),
            QStringLiteral("Scan Backward"), QString(), shortcutList({ "," })),
        ActionSpec::standard(KStandardActions::FullScreen, QStringLiteral("window_fullscreen"),
            QStringLiteral("Fullscreen"), shortcutList({ "F", "F11" })),
        ActionSpec::registered(QStringLiteral("help_shortcuts"),
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

QList<QKeySequence> KiriViewApplication::shortcuts(const QString &actionName) const
{
    const QAction *registeredAction = const_cast<KiriViewApplication *>(this)->action(actionName);
    if (!registeredAction) {
        return {};
    }

    return registeredAction->shortcuts();
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return filterShortcutsByCommandModifier(shortcuts(actionName), true);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return filterShortcutsByCommandModifier(shortcuts(actionName), false);
}

QString KiriViewApplication::shortcutText(const QString &actionName) const
{
    return shortcutListText(shortcuts(actionName));
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

    if (QAction *quitAction = action(QStringLiteral("file_quit"))) {
        finishRegisteredAction(quitAction, quitAction->text(), shortcutList({ "Q", "Ctrl+Q" }));
    }

    for (const ActionSpec &spec : actionSpecs()) {
        addAction(spec);
    }

    m_showMenuBarAction
        = addStandardAction(KStandardActions::ShowMenubar, QStringLiteral("options_show_menubar"),
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

QAction *KiriViewApplication::addStandardAction(
    KStandardActions::StandardAction actionType, const QString &name, const QString &text)
{
    QAction *registeredAction = mainCollection()->addAction(actionType, name, this, [](bool) { });
    return finishRegisteredAction(registeredAction, text, registeredAction->shortcuts());
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

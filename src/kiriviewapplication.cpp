// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplication.h"

#include "kiriviewsettings.h"

#include <KirigamiActionCollection>
#include <QIcon>
#include <QSignalBlocker>

namespace {
QKeySequence shortcut(const QString &sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
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

QList<QKeySequence> KiriViewApplication::shortcuts(const QString &actionName) const
{
    const QAction *registeredAction = const_cast<KiriViewApplication *>(this)->action(actionName);
    if (!registeredAction) {
        return {};
    }

    return registeredAction->shortcuts();
}

QString KiriViewApplication::shortcutText(const QString &actionName) const
{
    return shortcutListText(shortcuts(actionName));
}

void KiriViewApplication::setupActions()
{
    AbstractKirigamiApplication::setupActions();
    mainCollection()->setComponentDisplayName(QStringLiteral("KiriView"));

    addStandardAction(KStandardActions::Open, QStringLiteral("file_open"), QStringLiteral("Open"),
        standardShortcuts(QKeySequence::Open));

    addRegisteredAction(QStringLiteral("go_previous_archive"), QStringLiteral("Previous Archive"),
        QStringLiteral("go-previous-symbolic"), { shortcut(QStringLiteral("[")) });
    addRegisteredAction(QStringLiteral("go_next_archive"), QStringLiteral("Next Archive"),
        QStringLiteral("go-next-symbolic"), { shortcut(QStringLiteral("]")) });
    addRegisteredAction(QStringLiteral("go_previous_image"), QStringLiteral("Previous"),
        QStringLiteral("go-up-symbolic"), standardShortcuts(QKeySequence::MoveToPreviousPage));
    addRegisteredAction(QStringLiteral("go_next_image"), QStringLiteral("Next"),
        QStringLiteral("go-down-symbolic"), standardShortcuts(QKeySequence::MoveToNextPage));
    addRegisteredAction(QStringLiteral("go_first_image"), QStringLiteral("First Image"),
        QStringLiteral("go-first-symbolic"),
        { shortcut(QStringLiteral("Ctrl+Home")), shortcut(QStringLiteral("Home")) });
    addRegisteredAction(QStringLiteral("go_last_image"), QStringLiteral("Last Image"),
        QStringLiteral("go-last-symbolic"),
        { shortcut(QStringLiteral("Ctrl+End")), shortcut(QStringLiteral("End")) });

    addStandardAction(KStandardActions::ZoomIn, QStringLiteral("view_zoom_in"),
        QStringLiteral("Zoom In"),
        { shortcut(QStringLiteral("Ctrl+=")), shortcut(QStringLiteral("Ctrl++")),
            shortcut(QStringLiteral("=")), shortcut(QStringLiteral("+")) });
    addStandardAction(KStandardActions::ZoomOut, QStringLiteral("view_zoom_out"),
        QStringLiteral("Zoom Out"),
        { shortcut(QStringLiteral("-")), shortcut(QStringLiteral("Ctrl+-")) });
    addStandardAction(KStandardActions::FitToPage, QStringLiteral("view_fit"),
        QStringLiteral("Fit"), { shortcut(QStringLiteral("1")) });
    addStandardAction(KStandardActions::FitToHeight, QStringLiteral("view_fit_height"),
        QStringLiteral("Fit Height"), { shortcut(QStringLiteral("2")) });
    addStandardAction(KStandardActions::FitToWidth, QStringLiteral("view_fit_width"),
        QStringLiteral("Fit Width"), { shortcut(QStringLiteral("3")) });
    addStandardAction(KStandardActions::ActualSize, QStringLiteral("view_actual_size"),
        QStringLiteral("Actual Size"), { shortcut(QStringLiteral("0")) });
    addRegisteredAction(QStringLiteral("view_pan_left"), QStringLiteral("Pan Left"), QString(),
        { shortcut(QStringLiteral("Left")) });
    addRegisteredAction(QStringLiteral("view_pan_right"), QStringLiteral("Pan Right"), QString(),
        { shortcut(QStringLiteral("Right")) });
    addRegisteredAction(QStringLiteral("view_pan_up"), QStringLiteral("Pan Up"), QString(),
        { shortcut(QStringLiteral("Up")) });
    addRegisteredAction(QStringLiteral("view_pan_down"), QStringLiteral("Pan Down"), QString(),
        { shortcut(QStringLiteral("Down")) });
    addRegisteredAction(QStringLiteral("view_pan_top_left"), QStringLiteral("Top Left"), QString(),
        { shortcut(QStringLiteral("<")) });
    addRegisteredAction(QStringLiteral("view_pan_bottom_right"), QStringLiteral("Bottom Right"),
        QString(), { shortcut(QStringLiteral(">")) });
    addRegisteredAction(QStringLiteral("view_scan_forward"), QStringLiteral("Scan Forward"),
        QString(), { shortcut(QStringLiteral(".")) });
    addRegisteredAction(QStringLiteral("view_scan_backward"), QStringLiteral("Scan Backward"),
        QString(), { shortcut(QStringLiteral(",")) });

    addStandardAction(KStandardActions::FullScreen, QStringLiteral("window_fullscreen"),
        QStringLiteral("Fullscreen"),
        { shortcut(QStringLiteral("F")), shortcut(QStringLiteral("F11")) });
    addRegisteredAction(QStringLiteral("help_shortcuts"), QStringLiteral("Keyboard Shortcuts"),
        QStringLiteral("help-keyboard-shortcuts-symbolic"),
        { shortcut(QStringLiteral("?")), shortcut(QStringLiteral("F1")) });

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

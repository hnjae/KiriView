// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationmenupresentationruntime.h"

#include "kiriviewstate.h"

#include <KirigamiActionCollection>
#include <QAction>
#include <QObject>
#include <QSignalBlocker>

namespace KiriView::ApplicationActions {
ApplicationMenuPresentationRuntime::ApplicationMenuPresentationRuntime(
    KiriViewApplication &application)
    : m_application(application)
{
}

KiriViewApplication::MenuPresentation ApplicationMenuPresentationRuntime::menuPresentation() const
{
    return presentationForStoredValue(KiriViewState::menuPresentation());
}

void ApplicationMenuPresentationRuntime::setMenuPresentation(
    KiriViewApplication::MenuPresentation presentation)
{
    if (menuPresentation() == presentation) {
        return;
    }

    KiriViewState::setMenuPresentation(static_cast<int>(presentation));
    KiriViewState::self()->save();
    syncShowMenuBarAction();
    Q_EMIT m_application.menuPresentationChanged();
}

void ApplicationMenuPresentationRuntime::bindShowMenuBarAction(QAction *action)
{
    m_showMenuBarAction = action;
    if (m_showMenuBarAction == nullptr) {
        return;
    }

    KirigamiActionCollection::setShortcutsConfigurable(m_showMenuBarAction, false);
    m_showMenuBarAction->setCheckable(true);
    QObject::connect(
        m_showMenuBarAction, &QAction::triggered, &m_application, [this](bool checked) {
            setMenuPresentation(
                checked ? KiriViewApplication::MenuBar : KiriViewApplication::HamburgerMenu);
        });
    syncShowMenuBarAction();
}

void ApplicationMenuPresentationRuntime::syncShowMenuBarAction()
{
    if (m_showMenuBarAction == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_showMenuBarAction);
    m_showMenuBarAction->setChecked(menuPresentation() == KiriViewApplication::MenuBar);
}

KiriViewApplication::MenuPresentation
ApplicationMenuPresentationRuntime::presentationForStoredValue(int value)
{
    if (value == static_cast<int>(KiriViewApplication::MenuBar)) {
        return KiriViewApplication::MenuBar;
    }

    return KiriViewApplication::HamburgerMenu;
}
}

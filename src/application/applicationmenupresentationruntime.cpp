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
    , m_state(ApplicationMenuPresentationState::presentationForStoredValue(
          KiriViewState::menuPresentation()))
{
}

KiriViewApplication::MenuPresentation ApplicationMenuPresentationRuntime::menuPresentation() const
{
    return m_state.presentation();
}

void ApplicationMenuPresentationRuntime::setMenuPresentation(
    KiriViewApplication::MenuPresentation presentation)
{
    if (!m_state.setPresentation(presentation)) {
        return;
    }

    KiriViewState::setMenuPresentation(m_state.storedValue());
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

void ApplicationMenuPresentationRuntime::syncFromSettings()
{
    const bool changed = m_state.setStoredValue(KiriViewState::menuPresentation());
    syncShowMenuBarAction();
    if (changed) {
        Q_EMIT m_application.menuPresentationChanged();
    }
}

void ApplicationMenuPresentationRuntime::syncShowMenuBarAction()
{
    if (m_showMenuBarAction == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_showMenuBarAction);
    m_showMenuBarAction->setChecked(menuPresentation() == KiriViewApplication::MenuBar);
}
}

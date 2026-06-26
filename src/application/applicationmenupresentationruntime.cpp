// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationmenupresentationruntime.h"

#include "kiriviewstate.h"

#include <KirigamiActionCollection>
#include <QAction>
#include <QObject>
#include <QSignalBlocker>
#include <utility>

namespace kiriview::ApplicationActions {
ApplicationMenuPresentationRuntime::ApplicationMenuPresentationRuntime(
    ApplicationActionHost& host, ChangeCallback changeCallback)
    : m_host(host)
    , m_changeCallback(std::move(changeCallback))
    , m_state(ApplicationMenuPresentationState::presentationForStoredValue(
          KiriViewState::menuPresentation()))
{
}

MenuPresentation ApplicationMenuPresentationRuntime::menuPresentation() const
{
    return m_state.presentation();
}

void ApplicationMenuPresentationRuntime::setMenuPresentation(MenuPresentation presentation)
{
    if (!m_state.setPresentation(presentation)) {
        return;
    }

    KiriViewState::setMenuPresentation(m_state.storedValue());
    KiriViewState::self()->save();
    syncShowMenuBarAction();
    if (m_changeCallback) {
        m_changeCallback();
    }
}

void ApplicationMenuPresentationRuntime::bindShowMenuBarAction(QAction* action)
{
    m_showMenuBarAction = action;
    if (m_showMenuBarAction == nullptr) {
        return;
    }

    KirigamiActionCollection::setShortcutsConfigurable(m_showMenuBarAction, false);
    m_showMenuBarAction->setCheckable(true);
    QObject::connect(
        m_showMenuBarAction, &QAction::triggered, m_host.actionContext(), [this](bool checked) {
            setMenuPresentation(
                checked ? MenuPresentation::MenuBar : MenuPresentation::HamburgerMenu);
        });
    syncShowMenuBarAction();
}

void ApplicationMenuPresentationRuntime::syncFromSettings()
{
    const bool changed = m_state.setStoredValue(KiriViewState::menuPresentation());
    syncShowMenuBarAction();
    if (changed) {
        if (m_changeCallback) {
            m_changeCallback();
        }
    }
}

void ApplicationMenuPresentationRuntime::syncShowMenuBarAction()
{
    if (m_showMenuBarAction == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_showMenuBarAction);
    m_showMenuBarAction->setChecked(menuPresentation() == MenuPresentation::MenuBar);
}
}

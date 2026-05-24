// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONMENUPRESENTATIONRUNTIME_H
#define KIRIVIEW_APPLICATIONMENUPRESENTATIONRUNTIME_H

#include "application/applicationmenupresentationstate.h"

#include <functional>

class QAction;
class KiriViewApplication;

namespace KiriView::ApplicationActions {
class ApplicationMenuPresentationRuntime final
{
public:
    using ChangeCallback = std::function<void()>;

    explicit ApplicationMenuPresentationRuntime(
        KiriViewApplication &application, ChangeCallback changeCallback = {});

    MenuPresentation menuPresentation() const;
    void setMenuPresentation(MenuPresentation presentation);
    void bindShowMenuBarAction(QAction *action);
    void syncFromSettings();
    void syncShowMenuBarAction();

private:
    KiriViewApplication &m_application;
    ChangeCallback m_changeCallback;
    ApplicationMenuPresentationState m_state;
    QAction *m_showMenuBarAction = nullptr;
};
}

#endif

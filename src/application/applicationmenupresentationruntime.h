// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONMENUPRESENTATIONRUNTIME_H
#define KIRIVIEW_APPLICATIONMENUPRESENTATIONRUNTIME_H

#include "application/applicationmenupresentationstate.h"
#include "facade/kiriviewapplication.h"

class QAction;

namespace KiriView::ApplicationActions {
class ApplicationMenuPresentationRuntime final
{
public:
    explicit ApplicationMenuPresentationRuntime(KiriViewApplication &application);

    KiriViewApplication::MenuPresentation menuPresentation() const;
    void setMenuPresentation(KiriViewApplication::MenuPresentation presentation);
    void bindShowMenuBarAction(QAction *action);
    void syncFromSettings();
    void syncShowMenuBarAction();

private:
    KiriViewApplication &m_application;
    ApplicationMenuPresentationState m_state;
    QAction *m_showMenuBarAction = nullptr;
};
}

#endif

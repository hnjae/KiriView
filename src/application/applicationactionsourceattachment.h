// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONACTIONSOURCEATTACHMENT_H
#define KIRIVIEW_APPLICATIONACTIONSOURCEATTACHMENT_H

#include "applicationactionruntime.h"

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <functional>
#include <vector>

namespace kiriview::ApplicationActions {
struct ApplicationActionUiGateSnapshot {
    bool helpDialogOpen = false;
    bool textInputFocused = false;
    bool infoPanelVisible = false;
    bool thumbnailPanelVisible = false;
    bool fullscreen = false;
    bool applicationMenuShortcutEnabled = false;
    bool showMenubarActionEnabled = true;
};

class ApplicationActionStateSource
{
public:
    virtual ~ApplicationActionStateSource();

    virtual ApplicationActionStateSnapshot actionStateSnapshot() const = 0;
    virtual std::vector<QMetaObject::Connection> connectActionStateChanged(
        QObject *context, std::function<void()> refresh)
        = 0;
};

class ApplicationActionSourceAttachment final
{
public:
    ApplicationActionSourceAttachment(ApplicationActionRuntime &runtime, QObject &context);
    ~ApplicationActionSourceAttachment();

    void setSource(ApplicationActionStateSource *source);
    void reattach();
    void refresh();

private:
    void connectSource();
    void disconnectSource();

    ApplicationActionRuntime &m_runtime;
    QPointer<QObject> m_context;
    ApplicationActionStateSource *m_source = nullptr;
    std::vector<QMetaObject::Connection> m_connections;
};
}

#endif

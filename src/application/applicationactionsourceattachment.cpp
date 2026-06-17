// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionsourceattachment.h"

namespace kiriview::ApplicationActions {
ApplicationActionStateSource::~ApplicationActionStateSource() = default;

ApplicationActionSourceAttachment::ApplicationActionSourceAttachment(
    ApplicationActionRuntime &runtime, QObject &context)
    : m_runtime(runtime)
    , m_context(&context)
{
}

ApplicationActionSourceAttachment::~ApplicationActionSourceAttachment() { disconnectSource(); }

void ApplicationActionSourceAttachment::setSource(ApplicationActionStateSource *source)
{
    if (m_source == source) {
        return;
    }

    disconnectSource();
    m_source = source;
    connectSource();
    refresh();
}

void ApplicationActionSourceAttachment::reattach()
{
    disconnectSource();
    connectSource();
    refresh();
}

void ApplicationActionSourceAttachment::refresh()
{
    if (m_source == nullptr) {
        m_runtime.setActionStateSnapshot(ApplicationActionStateSnapshot {});
        return;
    }

    m_runtime.setActionStateSnapshot(m_source->actionStateSnapshot());
}

void ApplicationActionSourceAttachment::connectSource()
{
    if (m_source == nullptr || m_context == nullptr) {
        return;
    }

    m_connections = m_source->connectActionStateChanged(m_context, [this]() { refresh(); });
}

void ApplicationActionSourceAttachment::disconnectSource()
{
    for (const QMetaObject::Connection &connection : m_connections) {
        QObject::disconnect(connection);
    }
    m_connections.clear();
}
}

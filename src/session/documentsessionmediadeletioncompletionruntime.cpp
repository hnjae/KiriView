// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediadeletioncompletionruntime.h"

#include "localization/imageerrortext.h"

#include <utility>

namespace {
QString genericFileDeletionErrorMessage()
{
    return kiriview::imageErrorText(kiriview::ImageErrorTextId::DeleteFile);
}
}

namespace kiriview {
DocumentSessionMediaDeletionCompletionRuntime::DocumentSessionMediaDeletionCompletionRuntime(
    DocumentSessionMediaDeletionCompletionRuntimePorts ports)
    : m_ports(std::move(ports))
{
}

void DocumentSessionMediaDeletionCompletionRuntime::apply(
    const DocumentSessionMediaDeletionCompletion& completion)
{
    if (m_ports.setFileDeletionInProgress) {
        m_ports.setFileDeletionInProgress(false);
    }

    if (completion.plan.reportFailure) {
        if (m_ports.setSessionErrorString) {
            m_ports.setSessionErrorString(completion.failure.userMessage.isEmpty()
                    ? genericFileDeletionErrorMessage()
                    : completion.failure.userMessage);
        }
        if (m_ports.recomputePublicProjection) {
            m_ports.recomputePublicProjection();
        }
        return;
    }

    if (m_ports.recomputePublicProjection) {
        m_ports.recomputePublicProjection();
    }
    if (completion.plan.hasRoutePlan() && m_ports.executeRoutePlan) {
        m_ports.executeRoutePlan(completion.plan.routePlan);
    }
}
}

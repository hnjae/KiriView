// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionvideodocumentsyncruntime.h"

#include <QUrl>
#include <utility>

namespace kiriview {
DocumentSessionVideoDocumentSyncRuntime::DocumentSessionVideoDocumentSyncRuntime(
    DocumentSessionVideoDocumentSyncRuntimePorts ports)
    : m_ports(std::move(ports))
{
}

void DocumentSessionVideoDocumentSyncRuntime::sync(
    DocumentSessionKind documentKind, const DocumentSessionPublicVideoLeafSnapshot& video)
{
    apply(documentSessionVideoDocumentSyncPlan(
        DocumentSessionVideoDocumentSyncInput { documentKind, video }));
}

void DocumentSessionVideoDocumentSyncRuntime::apply(
    const DocumentSessionVideoDocumentSyncPlan& plan)
{
    switch (plan.operation) {
    case DocumentSessionVideoDocumentSyncOperation::None:
        return;
    case DocumentSessionVideoDocumentSyncOperation::ClearSessionDirectMedia:
        if (m_ports.clearDirectMediaCursor) {
            m_ports.clearDirectMediaCursor();
        }
        if (m_ports.setSourceIdentity) {
            m_ports.setSourceIdentity(QUrl());
        }
        if (m_ports.setDocumentKind) {
            m_ports.setDocumentKind(DocumentSessionKind::Empty);
        }
        if (m_ports.clearDirectMediaNavigation) {
            m_ports.clearDirectMediaNavigation();
        }
        break;
    case DocumentSessionVideoDocumentSyncOperation::CommitDirectVideoCursor: {
        bool directMediaScopeChanged = false;
        if (m_ports.setDirectVideoCursor) {
            directMediaScopeChanged = m_ports.setDirectVideoCursor(plan.url);
        }
        if (m_ports.setSourceIdentity) {
            m_ports.setSourceIdentity(plan.url);
        }
        if (directMediaScopeChanged && m_ports.refreshDirectMediaNavigation) {
            m_ports.refreshDirectMediaNavigation();
        }
        break;
    }
    }

    if (m_ports.recomputePublicProjection) {
        m_ports.recomputePublicProjection();
    }
    if (m_ports.recomputeActiveZoomReadout) {
        m_ports.recomputeActiveZoomReadout();
    }
}
}

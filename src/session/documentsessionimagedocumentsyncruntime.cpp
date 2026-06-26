// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionimagedocumentsyncruntime.h"

#include <utility>

namespace kiriview {
DocumentSessionImageDocumentSyncRuntime::DocumentSessionImageDocumentSyncRuntime(
    DocumentSessionImageDocumentSyncRuntimePorts ports)
    : m_ports(std::move(ports))
{
}

void DocumentSessionImageDocumentSyncRuntime::sync(
    const DocumentSessionImageDocumentSyncRuntimeInput& input)
{
    if (input.routingSource || input.documentKind != DocumentSessionKind::Image) {
        return;
    }

    const bool directMediaScopeChanged
        = syncDirectImageCursor(input.documentKind, input.directMediaCursor, input.image);
    apply(documentSessionImageDocumentSyncPlan(DocumentSessionImageDocumentSyncInput {
        input.routingSource,
        input.documentKind,
        input.directImageLoadMayUseImageDocumentSourceScope,
        input.directMediaNavigationActive,
        input.directMediaNavigationKnown,
        directMediaScopeChanged,
        input.previousPageNavigation,
        input.image,
    }));
}

bool DocumentSessionImageDocumentSyncRuntime::syncDirectImageCursor(
    DocumentSessionKind documentKind, const DirectMediaCursor& cursor,
    const DocumentSessionPublicImageLeafSnapshot& image)
{
    const DocumentSessionDirectImageCursorSyncPlan plan = documentSessionDirectImageCursorSyncPlan(
        DocumentSessionDirectImageCursorSyncInput { documentKind, cursor, image });
    switch (plan.operation) {
    case DocumentSessionDirectImageCursorSyncOperation::None:
        return false;
    case DocumentSessionDirectImageCursorSyncOperation::ConfirmDirectImageCursor:
        return m_ports.confirmDirectImageCursor ? m_ports.confirmDirectImageCursor(plan.url)
                                                : false;
    case DocumentSessionDirectImageCursorSyncOperation::RestoreDirectImageCursorAfterFailure:
        return m_ports.restoreDirectImageCursorAfterFailure
            ? m_ports.restoreDirectImageCursorAfterFailure()
            : false;
    }

    return false;
}

void DocumentSessionImageDocumentSyncRuntime::apply(
    const DocumentSessionImageDocumentSyncPlan& plan)
{
    if (!plan.active) {
        return;
    }

    if (plan.setSourceIdentity && m_ports.setSourceIdentity) {
        m_ports.setSourceIdentity(plan.sourceIdentityUrl);
    }
    if (plan.syncFileDeletionProgress && m_ports.setFileDeletionInProgress) {
        m_ports.setFileDeletionInProgress(plan.fileDeletionInProgress);
    }

    switch (plan.directMediaOperation) {
    case DocumentSessionImageDocumentSyncDirectMediaOperation::None:
        break;
    case DocumentSessionImageDocumentSyncDirectMediaOperation::RefreshDirectMediaNavigation:
        if (m_ports.refreshDirectMediaNavigation) {
            m_ports.refreshDirectMediaNavigation();
        }
        break;
    case DocumentSessionImageDocumentSyncDirectMediaOperation::CacheDisplayedMediaPredecodeImages:
        if (m_ports.cacheDisplayedMediaPredecodeImages) {
            m_ports.cacheDisplayedMediaPredecodeImages();
        }
        break;
    }

    switch (plan.projectionOperation) {
    case DocumentSessionImageDocumentSyncProjectionOperation::None:
        return;
    case DocumentSessionImageDocumentSyncProjectionOperation::RecomputePublicProjection:
        if (m_ports.recomputePublicProjection) {
            m_ports.recomputePublicProjection();
        }
        return;
    case DocumentSessionImageDocumentSyncProjectionOperation::PublishImagePageActiveNavigation:
        if (m_ports.publishImagePageActiveNavigation) {
            m_ports.publishImagePageActiveNavigation();
        }
        return;
    }
}
}

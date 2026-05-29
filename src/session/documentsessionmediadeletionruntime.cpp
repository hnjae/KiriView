// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediadeletionruntime.h"

#include "async/imagecallback.h"

#include <memory>
#include <utility>

namespace KiriView {
DocumentSessionMediaDeletionRuntime::DocumentSessionMediaDeletionRuntime(
    FileDeletionProvider fileDeletionProvider)
    : m_fileDeletionProvider(fileDeletionProviderWithDefault(std::move(fileDeletionProvider)))
{
}

DocumentSessionMediaDeletionRuntime::~DocumentSessionMediaDeletionRuntime() { cancel(); }

DocumentSessionMediaDeletionStartPlan DocumentSessionMediaDeletionRuntime::start(QObject *receiver,
    FileDeletionMode mode, std::vector<DirectMediaNavigationCandidate> candidates,
    const QUrl &currentUrl, DocumentSessionKind documentKind, CompletionCallback callback)
{
    const DocumentSessionMediaDeletionStartPlan plan
        = documentSessionMediaDeletionStartPlan(mode, std::move(candidates), currentUrl);
    if (!plan.shouldStartDeletion) {
        return plan;
    }

    m_job.cancel();
    const quint64 operationId = m_operation.start();
    auto sharedCallback = std::make_shared<CompletionCallback>(std::move(callback));
    m_job = m_fileDeletionProvider(receiver, plan.request,
        [this, operationId, documentKind, fallbackPlan = plan.fallbackPlan, sharedCallback](
            FileDeletionResult result, const QString &errorString) {
            finish(operationId, documentKind, fallbackPlan, result, errorString, *sharedCallback);
        });
    return plan;
}

void DocumentSessionMediaDeletionRuntime::cancel()
{
    m_job.cancel();
    m_operation.cancel();
}

bool DocumentSessionMediaDeletionRuntime::active() const { return m_operation.active(); }

void DocumentSessionMediaDeletionRuntime::finish(quint64 operationId,
    DocumentSessionKind documentKind, const DocumentSessionMediaDeletionFallbackPlan &fallbackPlan,
    FileDeletionResult result, const QString &errorString, const CompletionCallback &callback)
{
    if (!m_operation.finish(operationId)) {
        return;
    }

    invokeIfSet(callback,
        DocumentSessionMediaDeletionCompletion {
            documentSessionMediaDeletionCompletionPlan(documentKind, fallbackPlan, result),
            errorString,
        });
}
}

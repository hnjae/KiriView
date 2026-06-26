// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONRUNTIME_H

#include "async/imageasyncoperationstate.h"
#include "async/imageiojob.h"
#include "navigation/directmedianavigationmodel.h"
#include "session/documentsessiondirectmedianavigationruntime.h"
#include "session/documentsessionmediadeletionplan.h"
#include "session/documentsessiontypes.h"
#include "system/filedeletion.h"

#include <QString>
#include <QUrl>
#include <functional>
#include <vector>

class QObject;

namespace kiriview {
struct DocumentSessionMediaDeletionCompletion
{
    DocumentSessionMediaDeletionCompletionPlan plan;
    KioOperationFailure failure;
};

class DocumentSessionMediaDeletionRuntime final
{
public:
    using CompletionCallback = std::function<void(DocumentSessionMediaDeletionCompletion)>;
    using ScopeAccepted = DocumentSessionDirectMediaNavigationRuntime::ScopeAccepted;

    explicit DocumentSessionMediaDeletionRuntime(FileDeletionProvider fileDeletionProvider = {},
        DirectMediaNavigationCandidateProvider candidateProvider = {});
    ~DocumentSessionMediaDeletionRuntime();

    DocumentSessionMediaDeletionStartPlan start(QObject* receiver, FileDeletionMode mode,
        std::vector<DirectMediaNavigationCandidate> candidates, const QUrl& currentUrl,
        DocumentSessionKind documentKind, CompletionCallback callback);
    bool startForDirectMedia(QObject* receiver, FileDeletionMode mode,
        const DirectMediaScope& scope, ScopeAccepted scopeAccepted,
        DocumentSessionKind documentKind, CompletionCallback callback);
    void cancel();
    bool active() const;

private:
    void finish(quint64 operationId, DocumentSessionKind documentKind,
        const DocumentSessionMediaDeletionFallbackPlan& fallbackPlan, FileDeletionResult result,
        const KioOperationFailure& failure, const CompletionCallback& callback);

    FileDeletionProvider m_fileDeletionProvider;
    DocumentSessionDirectMediaNavigationRuntime m_candidateRuntime;
    ImageIoJob m_job;
    ImageAsyncOperationState m_operation;
};
}

#endif

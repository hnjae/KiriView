// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONRUNTIME_H

#include "async/imageasyncoperationstate.h"
#include "async/imageiojob.h"
#include "document/filedeletion.h"
#include "navigation/medianavigationmodel.h"
#include "session/documentsessionmediadeletionplan.h"
#include "session/documentsessionstate.h"

#include <QString>
#include <QUrl>
#include <functional>
#include <vector>

class QObject;

namespace KiriView {
struct DocumentSessionMediaDeletionCompletion {
    DocumentSessionMediaDeletionCompletionPlan plan;
    QString errorString;
};

class DocumentSessionMediaDeletionRuntime final
{
public:
    using CompletionCallback = std::function<void(DocumentSessionMediaDeletionCompletion)>;

    explicit DocumentSessionMediaDeletionRuntime(FileOperationProvider fileOperationProvider = {});
    ~DocumentSessionMediaDeletionRuntime();

    DocumentSessionMediaDeletionStartPlan start(QObject *receiver, FileDeletionMode mode,
        std::vector<MediaNavigationCandidate> candidates, const QUrl &currentUrl,
        DocumentSessionKind documentKind, CompletionCallback callback);
    void cancel();
    bool active() const;

private:
    void finish(quint64 operationId, DocumentSessionKind documentKind,
        const MediaDeletionFallbackPlan &fallbackPlan, FileDeletionResult result,
        const QString &errorString, const CompletionCallback &callback);

    FileOperationProvider m_fileOperationProvider;
    ImageIoJob m_job;
    ImageAsyncOperationState m_operation;
};
}

#endif

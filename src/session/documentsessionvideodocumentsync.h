// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONVIDEODOCUMENTSYNC_H
#define KIRIVIEW_DOCUMENTSESSIONVIDEODOCUMENTSYNC_H

#include "session/documentsessionpublicprojection.h"

#include <QUrl>

namespace kiriview {
enum class DocumentSessionVideoDocumentSyncOperation {
    None,
    ClearSessionDirectMedia,
    CommitDirectVideoCursor,
    CommitOpenedCollectionVideoSource,
};

struct DocumentSessionVideoDocumentSyncInput
{
    DocumentSessionKind documentKind = DocumentSessionKind::Empty;
    DocumentSessionPublicVideoLeafSnapshot video;
    bool openedCollectionVideoActive = false;
};

struct DocumentSessionVideoDocumentSyncPlan
{
    DocumentSessionVideoDocumentSyncOperation operation
        = DocumentSessionVideoDocumentSyncOperation::None;
    QUrl url;
};

DocumentSessionVideoDocumentSyncPlan documentSessionVideoDocumentSyncPlan(
    const DocumentSessionVideoDocumentSyncInput& input);
}

#endif

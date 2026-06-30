// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionvideodocumentsync.h"

namespace kiriview {
DocumentSessionVideoDocumentSyncPlan documentSessionVideoDocumentSyncPlan(
    const DocumentSessionVideoDocumentSyncInput& input)
{
    if (input.documentKind != DocumentSessionKind::Video) {
        return {};
    }

    if (input.video.sourceUrl.isEmpty()) {
        return DocumentSessionVideoDocumentSyncPlan {
            DocumentSessionVideoDocumentSyncOperation::ClearSessionDirectMedia,
            QUrl(),
        };
    }

    if (input.openedCollectionVideoActive) {
        return DocumentSessionVideoDocumentSyncPlan {
            DocumentSessionVideoDocumentSyncOperation::CommitOpenedCollectionVideoSource,
            input.video.sourceUrl,
        };
    }

    return DocumentSessionVideoDocumentSyncPlan {
        DocumentSessionVideoDocumentSyncOperation::CommitDirectVideoCursor,
        input.video.sourceUrl,
    };
}
}

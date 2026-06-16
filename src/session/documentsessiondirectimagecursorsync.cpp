// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectimagecursorsync.h"

#include "location/imageurl.h"

namespace kiriview {
DocumentSessionDirectImageCursorSyncPlan documentSessionDirectImageCursorSyncPlan(
    const DocumentSessionDirectImageCursorSyncInput &input)
{
    if (input.documentKind != DocumentSessionKind::Image) {
        return {};
    }

    const QUrl pendingUrl = input.cursor.pendingUrl;
    const QUrl displayedUrl = input.image.displayedUrl;
    if (!pendingUrl.isEmpty()) {
        if (input.image.ordinaryDirectMediaScopeActive
            && sameNormalizedUrl(displayedUrl, pendingUrl)) {
            return DocumentSessionDirectImageCursorSyncPlan {
                DocumentSessionDirectImageCursorSyncOperation::ConfirmDirectImageCursor,
                displayedUrl,
            };
        }

        if (input.image.error) {
            if (sameNormalizedUrl(input.image.sourceUrl, pendingUrl)) {
                return DocumentSessionDirectImageCursorSyncPlan {
                    DocumentSessionDirectImageCursorSyncOperation::ConfirmDirectImageCursor,
                    pendingUrl,
                };
            }
            return DocumentSessionDirectImageCursorSyncPlan {
                DocumentSessionDirectImageCursorSyncOperation::RestoreDirectImageCursorAfterFailure,
                QUrl(),
            };
        }

        if (!input.image.sourceUrl.isEmpty() && input.image.sourceUrl != pendingUrl) {
            return DocumentSessionDirectImageCursorSyncPlan {
                DocumentSessionDirectImageCursorSyncOperation::RestoreDirectImageCursorAfterFailure,
                QUrl(),
            };
        }
        return {};
    }

    if (input.image.ordinaryDirectMediaScopeActive && !displayedUrl.isEmpty()) {
        return DocumentSessionDirectImageCursorSyncPlan {
            DocumentSessionDirectImageCursorSyncOperation::ConfirmDirectImageCursor,
            displayedUrl,
        };
    }

    if (input.image.error) {
        return DocumentSessionDirectImageCursorSyncPlan {
            DocumentSessionDirectImageCursorSyncOperation::RestoreDirectImageCursorAfterFailure,
            QUrl(),
        };
    }

    return {};
}
}

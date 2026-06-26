// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONDIRECTIMAGECURSORSYNC_H
#define KIRIVIEW_DOCUMENTSESSIONDIRECTIMAGECURSORSYNC_H

#include "session/directmediacursor.h"
#include "session/documentsessionpublicprojection.h"

#include <QUrl>

namespace kiriview {
enum class DocumentSessionDirectImageCursorSyncOperation {
    None,
    ConfirmDirectImageCursor,
    RestoreDirectImageCursorAfterFailure,
};

struct DocumentSessionDirectImageCursorSyncInput
{
    DocumentSessionKind documentKind = DocumentSessionKind::Empty;
    DirectMediaCursor cursor;
    DocumentSessionPublicImageLeafSnapshot image;
};

struct DocumentSessionDirectImageCursorSyncPlan
{
    DocumentSessionDirectImageCursorSyncOperation operation
        = DocumentSessionDirectImageCursorSyncOperation::None;
    QUrl url;
};

DocumentSessionDirectImageCursorSyncPlan documentSessionDirectImageCursorSyncPlan(
    const DocumentSessionDirectImageCursorSyncInput& input);
}

#endif

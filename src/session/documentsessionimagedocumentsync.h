// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONIMAGEDOCUMENTSYNC_H
#define KIRIVIEW_DOCUMENTSESSIONIMAGEDOCUMENTSYNC_H

#include "session/documentsessionpublicprojection.h"

#include <QUrl>

namespace kiriview {
enum class DocumentSessionImageDocumentSyncDirectMediaOperation {
    None,
    RefreshDirectMediaNavigation,
    CacheDisplayedMediaPredecodeImages,
};

enum class DocumentSessionImageDocumentSyncProjectionOperation {
    None,
    RecomputePublicProjection,
    PublishImagePageActiveNavigation,
};

struct DocumentSessionImageDocumentSyncInput
{
    bool routingSource = false;
    DocumentSessionKind documentKind = DocumentSessionKind::Empty;
    bool directImageLoadMayUseImageDocumentSourceScope = false;
    bool directMediaNavigationActive = false;
    bool directMediaNavigationKnown = false;
    bool directMediaScopeChanged = false;
    ImageDocumentPageActiveNavigationSnapshot previousPageNavigation;
    DocumentSessionPublicImageLeafSnapshot image;
};

struct DocumentSessionImageDocumentSyncPlan
{
    bool active = false;
    bool setSourceIdentity = false;
    QUrl sourceIdentityUrl;
    bool syncFileDeletionProgress = false;
    bool fileDeletionInProgress = false;
    DocumentSessionImageDocumentSyncDirectMediaOperation directMediaOperation
        = DocumentSessionImageDocumentSyncDirectMediaOperation::None;
    DocumentSessionImageDocumentSyncProjectionOperation projectionOperation
        = DocumentSessionImageDocumentSyncProjectionOperation::None;
};

DocumentSessionImageDocumentSyncPlan documentSessionImageDocumentSyncPlan(
    const DocumentSessionImageDocumentSyncInput& input);
}

#endif

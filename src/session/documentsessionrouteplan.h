// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONROUTEPLAN_H
#define KIRIVIEW_DOCUMENTSESSIONROUTEPLAN_H

#include "session/documentsessiontypes.h"

#include <QUrl>
#include <optional>
#include <variant>
#include <vector>

namespace kiriview {
enum class DocumentSessionRouteKind {
    Empty,
    DirectVideo,
    DirectImage,
    ImageDocument,
};

struct ClearSessionErrorStringRouteOperation {
};

struct CancelDirectMediaNavigationRouteOperation {
};

struct CancelMediaDeletionRouteOperation {
};

struct ClearDirectMediaNavigationRouteOperation {
};

struct ClearDirectMediaCursorRouteOperation {
};

struct SetDirectVideoCursorRouteOperation {
    QUrl url;
};

struct RequestDirectImageCursorRouteOperation {
    QUrl url;
};

struct ClearThenRequestDirectImageCursorRouteOperation {
    QUrl url;
};

struct ClearImageDocumentRouteOperation {
};

struct LeaveVideoModeRouteOperation {
};

struct EnterEmptyDocumentRouteOperation {
};

struct EnterImageDocumentRouteOperation {
    QUrl url;
};

struct EnterVideoDocumentRouteOperation {
    QUrl url;
};

struct SyncDirectImageCursorFromDocumentRouteOperation {
};

struct ClearSourceIdentityRouteOperation {
};

struct UseOriginalSourceIdentityRouteOperation {
    QUrl url;
};

struct UseImageDocumentSourceIdentityRouteOperation {
};

struct RecomputePublicProjectionRouteOperation {
};

struct RefreshDirectMediaNavigationAfterRoutingRouteOperation {
};

struct ClearMediaPredecodeRouteOperation {
};

using DocumentSessionRouteOperation
    = std::variant<ClearSessionErrorStringRouteOperation, CancelDirectMediaNavigationRouteOperation,
        CancelMediaDeletionRouteOperation, ClearDirectMediaNavigationRouteOperation,
        ClearDirectMediaCursorRouteOperation, SetDirectVideoCursorRouteOperation,
        RequestDirectImageCursorRouteOperation, ClearThenRequestDirectImageCursorRouteOperation,
        ClearImageDocumentRouteOperation, LeaveVideoModeRouteOperation,
        EnterEmptyDocumentRouteOperation, EnterImageDocumentRouteOperation,
        EnterVideoDocumentRouteOperation, SyncDirectImageCursorFromDocumentRouteOperation,
        ClearSourceIdentityRouteOperation, UseOriginalSourceIdentityRouteOperation,
        UseImageDocumentSourceIdentityRouteOperation, RecomputePublicProjectionRouteOperation,
        RefreshDirectMediaNavigationAfterRoutingRouteOperation, ClearMediaPredecodeRouteOperation>;

struct DocumentSessionRoutePlan {
    DocumentSessionRouteKind kind = DocumentSessionRouteKind::Empty;
    QUrl sourceUrl;
    std::vector<DocumentSessionRouteOperation> operations;
};

DocumentSessionRoutePlan documentSessionRoutePlanForSourceUrl(
    const QUrl &sourceUrl, DocumentSessionKind currentKind);
DocumentSessionRoutePlan documentSessionRoutePlanForMediaUrl(
    const QUrl &url, DocumentSessionKind currentKind);
DocumentSessionRoutePlan documentSessionRoutePlanAfterMediaDeletion(
    DocumentSessionKind deletedKind, std::optional<QUrl> fallbackUrl);
}

#endif

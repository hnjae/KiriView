// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONROUTEPLAN_H
#define KIRIVIEW_DOCUMENTSESSIONROUTEPLAN_H

#include "session/documentsessionstate.h"

#include <QUrl>

namespace KiriView {
enum class DocumentSessionRouteKind {
    Empty,
    DirectVideo,
    DirectImage,
    ImageDocument,
};

enum class DocumentSessionRouteCursorAction {
    None,
    Clear,
    SetDirectVideo,
    RequestDirectImage,
    ClearThenRequestDirectImage,
};

enum class DocumentSessionRouteSourceIdentityAction {
    None,
    Clear,
    UseOriginalUrl,
    UseImageDocumentSourceUrl,
};

struct DocumentSessionRoutePlan {
    DocumentSessionRouteKind kind = DocumentSessionRouteKind::Empty;
    QUrl sourceUrl;
    DocumentSessionRouteCursorAction cursorAction = DocumentSessionRouteCursorAction::None;
    DocumentSessionRouteSourceIdentityAction sourceIdentityAction
        = DocumentSessionRouteSourceIdentityAction::None;
    bool clearMediaNavigation = false;
    bool clearImageDocument = false;
    bool clearVideo = false;
    bool enterVideo = false;
    bool enterImage = false;
    bool enterEmpty = false;
    bool syncDirectImageCursorFromDocument = false;
    bool refreshMediaNavigation = false;
    bool clearPredecode = false;
};

bool isDocumentSessionDirectVideoUrl(const QUrl &url);
bool isDocumentSessionDirectImageUrl(const QUrl &url);
DocumentSessionRoutePlan documentSessionRoutePlanForSourceUrl(
    const QUrl &sourceUrl, DocumentSessionKind currentKind);
DocumentSessionRoutePlan documentSessionRoutePlanForMediaUrl(
    const QUrl &url, DocumentSessionKind currentKind);
}

#endif

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

enum class DocumentSessionRouteDocumentClear {
    None,
    Image,
    Video,
    ImageAndVideo,
};

enum class DocumentSessionRouteDocumentEnter {
    None,
    Empty,
    Image,
    Video,
};

struct DocumentSessionRouteMediaNavigationPlan {
    bool clearBeforeRouting = false;
    bool refreshAfterRouting = false;
};

struct DocumentSessionRouteDocumentPlan {
    DocumentSessionRouteDocumentClear clear = DocumentSessionRouteDocumentClear::None;
    DocumentSessionRouteDocumentEnter enter = DocumentSessionRouteDocumentEnter::None;
    bool syncDirectImageCursorFromDocument = false;
};

struct DocumentSessionRoutePredecodePlan {
    bool clear = false;
};

struct DocumentSessionRoutePlan {
    DocumentSessionRouteKind kind = DocumentSessionRouteKind::Empty;
    QUrl sourceUrl;
    DocumentSessionRouteCursorAction cursorAction = DocumentSessionRouteCursorAction::None;
    DocumentSessionRouteSourceIdentityAction sourceIdentityAction
        = DocumentSessionRouteSourceIdentityAction::None;
    DocumentSessionRouteMediaNavigationPlan mediaNavigation;
    DocumentSessionRouteDocumentPlan document;
    DocumentSessionRoutePredecodePlan predecode;
};

DocumentSessionRoutePlan documentSessionRoutePlanForSourceUrl(
    const QUrl &sourceUrl, DocumentSessionKind currentKind);
DocumentSessionRoutePlan documentSessionRoutePlanForMediaUrl(
    const QUrl &url, DocumentSessionKind currentKind);
}

#endif

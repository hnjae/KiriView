// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONROUTERUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONROUTERUNTIME_H

#include "session/documentsessionrouteplan.h"

#include <functional>

namespace kiriview {
struct DocumentSessionRouteSessionPorts {
    std::function<void()> cancelMediaOpenWith;
    std::function<void()> clearSessionErrorString;
    std::function<void(const std::function<void()> &)> executeWithRoutingSuppressed;
    std::function<void()> routeCompleted;
};

struct DocumentSessionRouteDirectMediaPorts {
    std::function<void()> cancelDirectMediaNavigation;
    std::function<void()> cancelMediaDeletion;
    std::function<void()> clearDirectMediaNavigation;
    std::function<bool()> clearDirectMediaCursor;
    std::function<bool(const QUrl &)> setDirectVideoCursor;
    std::function<bool(const QUrl &)> requestDirectImageCursor;
    std::function<bool()> syncDirectImageCursorFromDocument;
    std::function<bool()> directMediaNavigationActive;
    std::function<void()> refreshDirectMediaNavigation;
};

struct DocumentSessionRouteDocumentPorts {
    std::function<void()> clearImageDocument;
    std::function<void()> leaveVideoMode;
    std::function<void()> enterEmptyDocument;
    std::function<void(const QUrl &)> enterImageDocument;
    std::function<void(const QUrl &)> enterVideoDocument;
};

struct DocumentSessionRouteSourceIdentityPorts {
    std::function<void()> clearSourceIdentity;
    std::function<void(const QUrl &)> useOriginalSourceIdentity;
    std::function<void()> useImageDocumentSourceIdentity;
};

struct DocumentSessionRouteFollowUpPorts {
    std::function<void()> recomputePublicProjection;
    std::function<void()> clearMediaPredecode;
};

struct DocumentSessionRouteRuntimePorts {
    DocumentSessionRouteSessionPorts session;
    DocumentSessionRouteDirectMediaPorts directMedia;
    DocumentSessionRouteDocumentPorts documents;
    DocumentSessionRouteSourceIdentityPorts sourceIdentity;
    DocumentSessionRouteFollowUpPorts followUp;
};

class DocumentSessionRouteRuntime final
{
public:
    explicit DocumentSessionRouteRuntime(DocumentSessionRouteRuntimePorts ports = {});

    void routeSourceUrl(const QUrl &sourceUrl, DocumentSessionKind currentKind);
    void routeMediaUrl(const QUrl &url, DocumentSessionKind currentKind);
    void execute(const DocumentSessionRoutePlan &plan);

private:
    void executeSuppressed(const std::function<void()> &mutation);

    DocumentSessionRouteRuntimePorts m_ports;
};
}

#endif

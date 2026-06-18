// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONVIDEODOCUMENTSYNCRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONVIDEODOCUMENTSYNCRUNTIME_H

#include "session/documentsessionvideodocumentsync.h"

#include <functional>

namespace kiriview {
struct DocumentSessionVideoDocumentSyncRuntimePorts {
    std::function<void()> clearDirectMediaCursor;
    std::function<void(const QUrl &)> setSourceIdentity;
    std::function<void(DocumentSessionKind)> setDocumentKind;
    std::function<void()> clearDirectMediaNavigation;
    std::function<bool(const QUrl &)> setDirectVideoCursor;
    std::function<void()> refreshDirectMediaNavigation;
    std::function<void()> recomputePublicProjection;
    std::function<void()> recomputeActiveZoomReadout;
};

class DocumentSessionVideoDocumentSyncRuntime final
{
public:
    explicit DocumentSessionVideoDocumentSyncRuntime(
        DocumentSessionVideoDocumentSyncRuntimePorts ports = {});

    void sync(
        DocumentSessionKind documentKind, const DocumentSessionPublicVideoLeafSnapshot &video);

private:
    void apply(const DocumentSessionVideoDocumentSyncPlan &plan);

    DocumentSessionVideoDocumentSyncRuntimePorts m_ports;
};
}

#endif

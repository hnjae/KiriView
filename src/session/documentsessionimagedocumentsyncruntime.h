// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONIMAGEDOCUMENTSYNCRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONIMAGEDOCUMENTSYNCRUNTIME_H

#include "session/documentsessiondirectimagecursorsync.h"
#include "session/documentsessionimagedocumentsync.h"

#include <functional>

namespace kiriview {
struct DocumentSessionImageDocumentSyncRuntimeInput
{
    bool routingSource = false;
    DocumentSessionKind documentKind = DocumentSessionKind::Empty;
    bool directImageLoadMayUseImageDocumentSourceScope = false;
    bool directMediaNavigationActive = false;
    bool directMediaNavigationKnown = false;
    DirectMediaCursor directMediaCursor;
    ImageDocumentPageActiveNavigationSnapshot previousPageNavigation;
    DocumentSessionPublicImageLeafSnapshot image;
};

struct DocumentSessionImageDocumentSyncRuntimePorts
{
    std::function<bool(const QUrl&)> confirmDirectImageCursor;
    std::function<bool()> restoreDirectImageCursorAfterFailure;
    std::function<void(const QUrl&)> setSourceIdentity;
    std::function<void(bool)> setFileDeletionInProgress;
    std::function<void()> refreshDirectMediaNavigation;
    std::function<void()> cacheDisplayedMediaPredecodeImages;
    std::function<void()> publishImagePageActiveNavigation;
    std::function<void()> recomputePublicProjection;
};

class DocumentSessionImageDocumentSyncRuntime final
{
public:
    explicit DocumentSessionImageDocumentSyncRuntime(
        DocumentSessionImageDocumentSyncRuntimePorts ports = {});

    void sync(const DocumentSessionImageDocumentSyncRuntimeInput& input);
    bool syncDirectImageCursor(DocumentSessionKind documentKind, const DirectMediaCursor& cursor,
        const DocumentSessionPublicImageLeafSnapshot& image);

private:
    void apply(const DocumentSessionImageDocumentSyncPlan& plan);

    DocumentSessionImageDocumentSyncRuntimePorts m_ports;
};
}

#endif

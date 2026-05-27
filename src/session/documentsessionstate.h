// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONSTATE_H

#include "session/directmediacursor.h"
#include "session/documentsessionpublicprojection.h"
#include "session/documentsessiontypes.h"

#include "navigation/medianavigationmodel.h"

#include <QUrl>
#include <functional>
#include <vector>

namespace KiriView {
class DocumentSessionState final
{
public:
    using ChangeCallback = std::function<void(const std::vector<DocumentSessionChange> &)>;

    explicit DocumentSessionState(ChangeCallback changeCallback = {});

    const QUrl &sourceUrl() const;
    DocumentSessionKind documentKind() const;
    const QString &sessionErrorString() const;
    const QString &windowTitleSubject() const;
    bool fileDeletionInProgress() const;
    const ActiveZoomSnapshot &activeZoomSnapshot() const;
    const MediaNavigationBoundaryState &mediaNavigationState() const;
    bool mediaNavigationKnown() const;
    const ActiveNavigationSnapshot &activeNavigationSnapshot() const;
    ActiveNavigationSourceKind activeNavigationSourceKind() const;
    ActiveNavigationBoundaryScope activeNavigationBoundaryScope() const;
    bool displayedMediaOpenWithAvailable() const;
    bool displayedFileDeletionAvailable() const;
    const DocumentSessionPublicProjection &publicProjection() const;
    const DirectMediaCursor &directMediaCursor() const;
    QUrl directMediaCursorUrl() const;
    DirectMediaScope directMediaScope() const;

    void setSourceIdentity(const QUrl &url);
    void setDocumentKind(DocumentSessionKind kind);
    void setDocumentKindAndActiveZoomSnapshot(
        DocumentSessionKind kind, ActiveZoomSnapshot activeZoomSnapshot);
    void setFileDeletionInProgress(bool inProgress);
    void setActiveZoomSnapshot(ActiveZoomSnapshot snapshot);
    void setMediaNavigationState(MediaNavigationBoundaryState state, bool known);
    bool updatePublicProjection(DocumentSessionPublicProjectionInput input);
    bool updatePublicProjectionForSourceKind(
        DocumentSessionPublicProjectionInput input, ActiveNavigationSourceKind sourceKind);
    void setSessionErrorString(const QString &errorString);
    bool clearDirectMediaCursor();
    bool requestDirectImageCursor(const QUrl &url);
    bool confirmDirectImageCursor(const QUrl &url);
    bool restoreDirectImageCursorAfterFailure();
    bool setDirectVideoCursor(const QUrl &url);

    void publish(DocumentSessionChange change);
    void publish(std::vector<DocumentSessionChange> changes);

private:
    bool applyPublicProjection(DocumentSessionPublicProjection projection);

    ChangeCallback m_changeCallback;
    QUrl m_sourceUrl;
    DocumentSessionKind m_documentKind = DocumentSessionKind::Empty;
    DirectMediaCursor m_directMediaCursor;
    MediaNavigationBoundaryState m_mediaNavigationState;
    DocumentSessionPublicProjection m_publicProjection;
    ActiveZoomSnapshot m_activeZoomSnapshot;
    bool m_mediaNavigationKnown = false;
    bool m_fileDeletionInProgress = false;
    QString m_sessionErrorString;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONSTATE_H

#include "navigation/medianavigationmodel.h"
#include "session/activenavigationprojection.h"
#include "session/directmediacursor.h"

#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <vector>

namespace KiriView {
enum class DocumentSessionKind {
    Empty,
    Image,
    Video,
};

enum class DocumentSessionChange {
    SourceUrl,
    DocumentKind,
    ErrorString,
    WindowTitleSubject,
    ActiveZoomReadout,
    FileDeletionAvailability,
    FileDeletionInProgress,
    ActiveNavigation,
};

struct ActiveZoomSnapshot {
    bool available = false;
    bool known = false;
    qreal percent = 0.0;
    bool editable = false;
};

struct DocumentSessionPublicProjection {
    ActiveNavigationSourceKind sourceKind = ActiveNavigationSourceKind::None;
    ActiveNavigationBoundaryScope boundaryScope = ActiveNavigationBoundaryScope::None;
    ActiveNavigationSnapshot activeNavigation;
    QString windowTitleSubject;
    bool displayedFileDeletionAvailable = false;
};

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
    bool displayedFileDeletionAvailable() const;
    const DocumentSessionPublicProjection &publicProjection() const;
    const DirectMediaCursor &directMediaCursor() const;
    QUrl directMediaCursorUrl() const;

    void setSourceIdentity(const QUrl &url);
    void setDocumentKind(DocumentSessionKind kind);
    void setDocumentKindAndActiveZoomSnapshot(
        DocumentSessionKind kind, ActiveZoomSnapshot activeZoomSnapshot);
    void setFileDeletionInProgress(bool inProgress);
    void setActiveZoomSnapshot(ActiveZoomSnapshot snapshot);
    void setMediaNavigationState(MediaNavigationBoundaryState state, bool known);
    void setActiveNavigationSnapshot(ActiveNavigationSnapshot snapshot);
    void setPublicProjection(DocumentSessionPublicProjection projection);
    void setSessionErrorString(const QString &errorString);
    void setWindowTitleSubject(const QString &subject);
    bool clearDirectMediaCursor();
    bool requestDirectImageCursor(const QUrl &url);
    bool confirmDirectImageCursor(const QUrl &url);
    bool restoreDirectImageCursorAfterFailure();
    bool setDirectVideoCursor(const QUrl &url);

    void publish(DocumentSessionChange change);
    void publish(std::vector<DocumentSessionChange> changes);

private:
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

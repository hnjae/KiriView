// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONSTATE_H

#include "navigation/medianavigationmodel.h"
#include "session/activenavigationprojection.h"

#include <QString>
#include <QUrl>
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

struct DirectMediaCursor {
    QUrl stableUrl;
    QUrl pendingUrl;
    quint64 generation = 0;
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
    const MediaNavigationBoundaryState &mediaNavigationState() const;
    bool mediaNavigationKnown() const;
    const ActiveNavigationSnapshot &activeNavigationSnapshot() const;
    const DirectMediaCursor &directMediaCursor() const;
    QUrl directMediaCursorUrl() const;

    void setSourceIdentity(const QUrl &url);
    void setDocumentKind(DocumentSessionKind kind);
    void setFileDeletionInProgress(bool inProgress);
    void setMediaNavigationState(MediaNavigationBoundaryState state, bool known);
    void setActiveNavigationSnapshot(ActiveNavigationSnapshot snapshot);
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
    ActiveNavigationSnapshot m_activeNavigationSnapshot;
    bool m_mediaNavigationKnown = false;
    bool m_fileDeletionInProgress = false;
    QString m_sessionErrorString;
    QString m_windowTitleSubject;
};
}

#endif

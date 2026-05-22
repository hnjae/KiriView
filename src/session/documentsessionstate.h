// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONSTATE_H

#include "navigation/medianavigationmodel.h"

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
    WindowTitleFileName,
    FileDeletionAvailability,
    FileDeletionInProgress,
    MediaNavigationAvailability,
};

class DocumentSessionState final
{
public:
    using ChangeCallback = std::function<void(const std::vector<DocumentSessionChange> &)>;

    explicit DocumentSessionState(ChangeCallback changeCallback = {});

    const QUrl &sourceUrl() const;
    DocumentSessionKind documentKind() const;
    const QString &sessionErrorString() const;
    bool fileDeletionInProgress() const;
    const MediaNavigationBoundaryState &mediaNavigationState() const;
    bool mediaNavigationKnown() const;

    void setSourceIdentity(const QUrl &url);
    void setDocumentKind(DocumentSessionKind kind);
    void setFileDeletionInProgress(bool inProgress);
    void setMediaNavigationState(MediaNavigationBoundaryState state, bool known);
    void setSessionErrorString(const QString &errorString);

    void publish(DocumentSessionChange change);
    void publish(std::vector<DocumentSessionChange> changes);

private:
    ChangeCallback m_changeCallback;
    QUrl m_sourceUrl;
    DocumentSessionKind m_documentKind = DocumentSessionKind::Empty;
    MediaNavigationBoundaryState m_mediaNavigationState;
    bool m_mediaNavigationKnown = false;
    bool m_fileDeletionInProgress = false;
    QString m_sessionErrorString;
};
}

#endif

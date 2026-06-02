// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONSTATE_H
#define KIRIVIEW_DOCUMENTSESSIONSTATE_H

#include "session/directmediacursor.h"
#include "session/documentsessionpublicprojection.h"
#include "session/documentsessiontypes.h"

#include "navigation/directmedianavigationmodel.h"

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
    const DirectMediaNavigationBoundaryState &directMediaNavigationState() const;
    bool directMediaNavigationKnown() const;
    const std::vector<DirectMediaNavigationCandidate> &directMediaNavigationCandidates() const;
    const ActiveNavigationSnapshot &activeNavigationSnapshot() const;
    ActiveNavigationRevealIntent activeNavigationRevealIntent() const;
    ActiveNavigationRevealDirection activeNavigationRevealDirection() const;
    ActiveNavigationSourceKind activeNavigationSourceKind() const;
    ActiveNavigationBoundaryScope activeNavigationBoundaryScope() const;
    bool displayedMediaOpenWithAvailable() const;
    bool displayedFileDeletionAvailable() const;
    const MediaInformationProjectionSnapshot &mediaInformationSnapshot() const;
    const DocumentSessionPublicProjection &publicProjection() const;
    const DocumentSessionPublicSnapshot &publicSnapshot() const;
    const DirectMediaCursor &directMediaCursor() const;
    QUrl directMediaCursorUrl() const;
    DirectMediaScope directMediaScope() const;

    void setSourceIdentity(const QUrl &url);
    void setDocumentKind(DocumentSessionKind kind);
    void setDocumentKindAndActiveZoomSnapshot(
        DocumentSessionKind kind, ActiveZoomSnapshot activeZoomSnapshot);
    void setFileDeletionInProgress(bool inProgress);
    void setActiveZoomSnapshot(ActiveZoomSnapshot snapshot);
    void setActiveNavigationRevealIntent(ActiveNavigationRevealIntent intent);
    void setActiveNavigationRevealDirection(ActiveNavigationRevealDirection direction);
    void setDirectMediaNavigation(DirectMediaNavigationBoundaryState state, bool known,
        std::vector<DirectMediaNavigationCandidate> candidates);
    bool updatePublicSnapshot(const DocumentSessionPublicSnapshotInput &input);
    bool updatePublicSnapshotForSourceKind(
        const DocumentSessionPublicSnapshotInput &input, ActiveNavigationSourceKind sourceKind);
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
    bool applyPublicSnapshot(DocumentSessionPublicSnapshot snapshot);

    ChangeCallback m_changeCallback;
    QUrl m_sourceUrl;
    DocumentSessionKind m_documentKind = DocumentSessionKind::Empty;
    DirectMediaCursor m_directMediaCursor;
    DirectMediaNavigationBoundaryState m_directMediaNavigationState;
    std::vector<DirectMediaNavigationCandidate> m_directMediaNavigationCandidates;
    DocumentSessionPublicProjection m_publicProjection;
    DocumentSessionPublicSnapshot m_publicSnapshot;
    ActiveZoomSnapshot m_activeZoomSnapshot;
    ActiveNavigationRevealIntent m_activeNavigationRevealIntent
        = ActiveNavigationRevealIntent::None;
    ActiveNavigationRevealDirection m_activeNavigationRevealDirection
        = ActiveNavigationRevealDirection::None;
    bool m_directMediaNavigationKnown = false;
    bool m_fileDeletionInProgress = false;
    QString m_sourceErrorString;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONRUNTIME_H

#include "navigation/directmedianavigationcandidateprovider.h"
#include "navigation/directmedianavigationmodel.h"
#include "predecode/mediapredecodedependencies.h"
#include "session/activenavigationprojection.h"
#include "session/activenavigationthumbnailmodel.h"
#include "session/documentsessiondirectmedianavigationruntime.h"
#include "session/documentsessiondocumentports.h"
#include "session/documentsessionmediadeletionruntime.h"
#include "session/documentsessionpublicprojection.h"
#include "session/documentsessionrouteplan.h"
#include "session/documentsessionstate.h"
#include "session/mediaopenwith.h"
#include "system/filedeletion.h"

#include <QMetaObject>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QAbstractListModel;
class QObject;
class QString;

namespace KiriView {
class MediaPredecodeCoordinator;

struct DocumentSessionRuntimeDependencies {
    DirectMediaNavigationCandidateProvider directMediaNavigationCandidateProvider;
    FileDeletionProvider fileDeletionProvider;
    MediaOpenWithProvider mediaOpenWithProvider;
    MediaPredecodeDependencyOverrides directMediaPredecodeDependencies;
};

class DocumentSessionRuntime final
{
public:
    using ChangeCallback = DocumentSessionState::ChangeCallback;

    DocumentSessionRuntime(QObject *owner, DocumentSessionImageDocumentPort imageDocument,
        DocumentSessionVideoDocumentPort videoDocument, ChangeCallback changeCallback = {},
        DocumentSessionRuntimeDependencies dependencies = {});
    ~DocumentSessionRuntime();

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    DocumentSessionKind documentKind() const;
    QString errorString() const;
    QString windowTitleSubject() const;
    bool displayedFileDeletionAvailable() const;
    bool displayedMediaOpenWithAvailable() const;
    bool fileDeletionInProgress() const;
    bool activeZoomPercentAvailable() const;
    bool activeZoomPercentKnown() const;
    qreal activeZoomPercent() const;
    bool activeZoomEditable() const;
    bool activeNavigationAvailable() const;
    bool activeNavigationKnown() const;
    bool activeNavigationEditable() const;
    int activeNavigationCurrentNumber() const;
    int activeNavigationCount() const;
    bool canOpenPreviousActiveNavigation() const;
    bool canOpenNextActiveNavigation() const;
    bool atKnownFirstActiveNavigation() const;
    bool atKnownLastActiveNavigation() const;
    ActiveNavigationBoundaryScope activeNavigationBoundaryScope() const;
    ActiveNavigationRevealIntent activeNavigationRevealIntent() const;
    ActiveNavigationRevealDirection activeNavigationRevealDirection() const;
    QAbstractListModel *activeNavigationThumbnailModel() const;
    std::optional<PredecodedImage> findPredecodedImage(const QUrl &url) const;

    void openPreviousActiveNavigation();
    void openNextActiveNavigation();
    void openFirstActiveNavigation();
    void openLastActiveNavigation();
    void openActiveNavigationAtNumber(int number);
    void openActiveNavigationThumbnailAtNumber(int number);
    ActiveNavigationDispatchOutcome requestPreviousActiveNavigation();
    ActiveNavigationDispatchOutcome requestNextActiveNavigation();
    void deleteDisplayedFile(FileDeletionMode mode);
    void openCurrentMediaWith(MediaOpenWithCallback callback);

private:
    ActiveNavigationDispatchOutcome executeActiveNavigationDispatchRequest(
        ActiveNavigationDispatchRequest request, ActiveNavigationRevealContext context);
    void executeActiveNavigationDispatchPlan(const ActiveNavigationDispatchPlan &plan);
    void setPendingActiveNavigationRevealContext(ActiveNavigationRevealContext context);
    ActiveNavigationRevealContext takePendingActiveNavigationRevealContext(
        ActiveNavigationRevealIntent fallbackIntent);
    void setActiveNavigationRevealContext(ActiveNavigationRevealContext context);
    void clearActiveNavigationRevealContextIfUnavailable();
    void connectDocuments();
    void syncImageDocumentFileDeletionProgress();
    void setDocumentKind(DocumentSessionKind kind);
    void recomputeActiveZoomReadout();
    void recomputeActiveZoomReadoutForKind(DocumentSessionKind kind);
    void publishActiveNavigationForImagePages();
    void recomputePublicProjection();
    void syncActiveNavigationThumbnailRows();
    void routeSourceUrl(const QUrl &sourceUrl);
    void openMediaUrl(const QUrl &url);
    bool directMediaNavigationActive() const;
    void openPreviousMedia();
    void openNextMedia();
    void openMediaAtNumber(int mediaNumber);
    void openMedia(DirectMediaNavigationOpenRequest request);
    void executeRoutePlan(const DocumentSessionRoutePlan &plan);
    void leaveVideoMode();
    void syncFromImageDocument();
    void syncFromVideoDocument();
    void refreshDirectMediaNavigation();
    void loadDirectMediaNavigationCandidates(
        DocumentSessionDirectMediaNavigationRuntime::CandidatesCallback callback);
    void finishDirectMediaNavigation(DocumentSessionDirectMediaNavigationOpenResult result);
    void updateDirectMediaNavigationBoundaryState(
        DocumentSessionDirectMediaNavigationRefreshResult result);
    void scheduleMediaPredecode(const std::vector<DirectMediaNavigationCandidate> &candidates);
    void cacheDisplayedMediaPredecodeImages();
    std::vector<DisplayedPredecodeImage> displayedPredecodeImages() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;
    void cancelMediaDeletion();
    void startMediaDeletion(
        FileDeletionMode mode, std::vector<DirectMediaNavigationCandidate> candidates = {});
    MediaOpenWithPlan currentMediaOpenWithPlan() const;
    void finishMediaDeletion(DocumentSessionMediaDeletionCompletion completion);
    void executeMediaDeletionCompletionPlan(
        const DocumentSessionMediaDeletionCompletionPlan &plan, const QString &errorString);
    DirectMediaScope directMediaNavigationLoadScope() const;
    QUrl activeDirectMediaCursorUrl() const;
    bool directMediaCursorMatches(const DirectMediaScope &scope) const;
    bool activeImageUsesImageDocumentSourceScope() const;
    bool directImageLoadMayUseImageDocumentSourceScope() const;
    bool syncDirectImageCursorFromDocument();
    ActiveZoomSnapshot activeZoomSnapshotForKind(DocumentSessionKind kind) const;
    DocumentSessionPublicProjectionInput publicProjectionInput() const;
    DirectMediaActiveNavigationInput directMediaActiveNavigationInput() const;
    ImageDocumentPageActiveNavigationSnapshot imageDocumentPageActiveNavigationSnapshot() const;

    QObject *m_owner = nullptr;
    DocumentSessionImageDocumentPort m_imageDocument;
    DocumentSessionVideoDocumentPort m_videoDocument;
    DocumentSessionState m_state;
    std::unique_ptr<ActiveNavigationThumbnailModel> m_activeNavigationThumbnailModel;
    DocumentSessionDirectMediaNavigationRuntime m_directMediaNavigationRuntime;
    DocumentSessionDirectMediaNavigationRuntime m_directMediaDeletionCandidateRuntime;
    DocumentSessionMediaDeletionRuntime m_mediaDeletionRuntime;
    MediaOpenWithProvider m_mediaOpenWithProvider;
    ImageIoJob m_mediaOpenWithJob;
    std::unique_ptr<MediaPredecodeCoordinator> m_mediaPredecodeCoordinator;
    std::vector<QMetaObject::Connection> m_documentConnections;
    ActiveNavigationRevealContext m_pendingActiveNavigationRevealContext;
    bool m_routingSource = false;
};
}

#endif

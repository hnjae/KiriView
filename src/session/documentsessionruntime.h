// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONRUNTIME_H

#include "document/filedeletion.h"
#include "document/imagedocumentruntimedependencies.h"
#include "navigation/mediacandidateprovider.h"
#include "navigation/medianavigationmodel.h"
#include "session/activenavigationprojection.h"
#include "session/documentsessionmediadeletionruntime.h"
#include "session/documentsessionmedianavigationruntime.h"
#include "session/documentsessionpublicprojection.h"
#include "session/documentsessionrouteplan.h"
#include "session/documentsessionstate.h"

#include <QSize>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class KiriImageDocument;
class KiriVideoDocument;
class QObject;
class QString;

namespace KiriView {
class MediaPredecodeCoordinator;

struct DocumentSessionRuntimeDependencies {
    MediaNavigationCandidateProvider mediaCandidateProvider;
    FileOperationProvider fileOperationProvider;
    ImageDocumentRuntimeDependencyOverrides imageDocumentDependencies;
};

class DocumentSessionRuntime final
{
public:
    using ChangeCallback = DocumentSessionState::ChangeCallback;

    DocumentSessionRuntime(QObject *owner, KiriImageDocument &imageDocument,
        KiriVideoDocument &videoDocument, ChangeCallback changeCallback = {},
        DocumentSessionRuntimeDependencies dependencies = {});
    ~DocumentSessionRuntime();

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    DocumentSessionKind documentKind() const;
    QString errorString() const;
    QString windowTitleSubject() const;
    bool displayedFileDeletionAvailable() const;
    bool fileDeletionInProgress() const;
    bool activeZoomPercentAvailable() const;
    bool activeZoomPercentKnown() const;
    qreal activeZoomPercent() const;
    bool activeZoomEditable() const;
    bool mediaNavigationActive() const;
    bool mediaNavigationKnown() const;
    int currentMediaNumber() const;
    int mediaCount() const;
    bool canOpenPreviousMedia() const;
    bool canOpenNextMedia() const;
    bool atKnownFirstMedia() const;
    bool atKnownLastMedia() const;
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
    std::optional<PredecodedImage> findPredecodedImage(const QUrl &url) const;

    void openPreviousMedia();
    void openNextMedia();
    void openMediaAtNumber(int mediaNumber);
    void openPreviousActiveNavigation();
    void openNextActiveNavigation();
    void openFirstActiveNavigation();
    void openLastActiveNavigation();
    void openActiveNavigationAtNumber(int number);
    ActiveNavigationDispatchOutcome requestPreviousActiveNavigation();
    ActiveNavigationDispatchOutcome requestNextActiveNavigation();
    void deleteDisplayedFile(FileDeletionMode mode);

private:
    ActiveNavigationDispatchOutcome executeActiveNavigationDispatchRequest(
        ActiveNavigationDispatchRequest request);
    void executeActiveNavigationDispatchPlan(const ActiveNavigationDispatchPlan &plan);
    void connectDocuments();
    void syncImageDocumentFileDeletionProgress();
    void setDocumentKind(DocumentSessionKind kind);
    void recomputeActiveZoomReadout();
    void recomputeActiveZoomReadoutForKind(DocumentSessionKind kind);
    void publishActiveNavigationForImagePages();
    void recomputePublicProjection();
    void routeSourceUrl(const QUrl &sourceUrl);
    void openMediaUrl(const QUrl &url);
    void openMedia(MediaNavigationOpenRequest request);
    void executeRoutePlan(const DocumentSessionRoutePlan &plan);
    void leaveVideoMode();
    void syncFromImageDocument();
    void syncFromVideoDocument();
    void refreshMediaNavigation();
    void loadMediaCandidates(DocumentSessionMediaNavigationRuntime::CandidatesCallback callback);
    void finishMediaNavigation(DocumentSessionMediaNavigationOpenResult result);
    void updateMediaBoundaryState(DocumentSessionMediaNavigationRefreshResult result);
    void scheduleMediaPredecode(const std::vector<MediaNavigationCandidate> &candidates);
    std::vector<DisplayedPredecodeImage> displayedPredecodeImages() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;
    void cancelMediaDeletion();
    void startMediaDeletion(
        FileDeletionMode mode, std::vector<MediaNavigationCandidate> candidates = {});
    void finishMediaDeletion(DocumentSessionMediaDeletionCompletion completion);
    void executeMediaDeletionCompletionPlan(
        const DocumentSessionMediaDeletionCompletionPlan &plan, const QString &errorString);
    DocumentSessionMediaNavigationLoadScope mediaNavigationLoadScope() const;
    QUrl activeDirectMediaCursorUrl() const;
    QUrl activeDirectMediaScopeUrl() const;
    bool directMediaCursorMatches(const DocumentSessionMediaNavigationLoadScope &scope) const;
    bool activeImageUsesMediaScope() const;
    bool directImageLoadMayUseMediaScope() const;
    bool syncDirectImageCursorFromDocument();
    ActiveZoomSnapshot activeZoomSnapshotForKind(DocumentSessionKind kind) const;
    DocumentSessionPublicProjectionInput publicProjectionInput() const;
    DocumentSessionPublicProjection projectedPublicState() const;
    MediaActiveNavigationInput mediaActiveNavigationInput() const;
    ImageDocumentActiveNavigationInput imageDocumentActiveNavigationInput() const;

    QObject *m_owner = nullptr;
    KiriImageDocument &m_imageDocument;
    KiriVideoDocument &m_videoDocument;
    DocumentSessionState m_state;
    DocumentSessionMediaNavigationRuntime m_mediaNavigationRuntime;
    DocumentSessionMediaDeletionRuntime m_mediaDeletionRuntime;
    std::unique_ptr<MediaPredecodeCoordinator> m_mediaPredecodeCoordinator;
    bool m_routingSource = false;
};
}

#endif

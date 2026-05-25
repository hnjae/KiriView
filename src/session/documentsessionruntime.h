// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONRUNTIME_H

#include "async/imageasyncoperationstate.h"
#include "async/imageiojob.h"
#include "document/filedeletion.h"
#include "document/imagedocumentruntimedependencies.h"
#include "navigation/mediacandidateprovider.h"
#include "navigation/medianavigationmodel.h"
#include "session/activenavigationprojection.h"
#include "session/documentsessionmediacandidateloadstate.h"
#include "session/documentsessionmediadeletionplan.h"
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

struct MediaCandidateLoadResult {
    std::vector<MediaNavigationCandidate> candidates;
    bool succeeded = false;
    QString errorString;
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
    void deleteDisplayedFile(FileDeletionMode mode);

private:
    void connectDocuments();
    void syncImageDocumentFileDeletionProgress();
    void setDocumentKind(DocumentSessionKind kind);
    void recomputeActiveZoomReadout();
    void recomputeActiveZoomReadoutForKind(DocumentSessionKind kind);
    void publishActiveNavigationForImagePages();
    void recomputeActiveNavigation();
    void recomputeWindowTitleSubject();
    void routeSourceUrl(const QUrl &sourceUrl);
    void openMediaUrl(const QUrl &url);
    void executeRoutePlan(const DocumentSessionRoutePlan &plan);
    void leaveVideoMode();
    void syncFromImageDocument();
    void syncFromVideoDocument();
    void refreshMediaNavigation();
    void loadMediaCandidates(std::function<void(MediaCandidateLoadResult)> callback);
    void finishMediaCandidateLoad(DocumentSessionMediaCandidateLoad load,
        MediaCandidateLoadResult result,
        const std::shared_ptr<std::function<void(MediaCandidateLoadResult)>> &callback);
    void finishMediaNavigation(MediaCandidateLoadResult result, MediaNavigationOpenRequest request);
    void updateMediaBoundaryState(MediaCandidateLoadResult result);
    void scheduleMediaPredecode(const std::vector<MediaNavigationCandidate> &candidates);
    std::vector<DisplayedPredecodeImage> displayedPredecodeImages() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;
    void cancelMediaDeletion();
    void startMediaDeletion(
        FileDeletionMode mode, std::vector<MediaNavigationCandidate> candidates = {});
    void finishMediaDeletion(quint64 operationId, const MediaDeletionFallbackPlan &fallbackPlan,
        FileDeletionResult result, const QString &errorString);
    void executeMediaDeletionCompletionPlan(
        const DocumentSessionMediaDeletionCompletionPlan &plan, const QString &errorString);
    QUrl activeDirectMediaCursorUrl() const;
    QUrl activeDirectMediaScopeUrl() const;
    bool directMediaCursorMatches(const DocumentSessionMediaCandidateLoad &load) const;
    bool activeImageUsesMediaScope() const;
    bool directImageLoadMayUseMediaScope() const;
    bool syncDirectImageCursorFromDocument();
    ActiveZoomSnapshot activeZoomSnapshotForKind(DocumentSessionKind kind) const;
    ActiveNavigationSourceKind activeNavigationSourceKind() const;
    ActiveNavigationSnapshot projectedActiveNavigationSnapshot() const;
    QString projectedWindowTitleSubject() const;
    QSize directMediaWindowTitleSizeForKind(
        DocumentSessionKind kind, ActiveNavigationSourceKind sourceKind) const;
    MediaActiveNavigationInput mediaActiveNavigationInput() const;
    ImageDocumentActiveNavigationInput imageDocumentActiveNavigationInput() const;

    QObject *m_owner = nullptr;
    KiriImageDocument &m_imageDocument;
    KiriVideoDocument &m_videoDocument;
    DocumentSessionState m_state;
    MediaNavigationCandidateProvider m_mediaCandidateProvider;
    FileOperationProvider m_fileOperationProvider;
    std::unique_ptr<MediaPredecodeCoordinator> m_mediaPredecodeCoordinator;
    ImageIoJob m_mediaCandidateJob;
    DocumentSessionMediaCandidateLoadState m_mediaCandidateLoadState;
    ImageIoJob m_fileDeletionJob;
    ImageAsyncOperationState m_fileDeletionOperation;
    bool m_routingSource = false;
};
}

#endif

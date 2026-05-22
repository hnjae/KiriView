// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONRUNTIME_H

#include "async/imageiojob.h"
#include "document/filedeletion.h"
#include "document/imagedocumentruntimedependencies.h"
#include "navigation/mediacandidateprovider.h"
#include "navigation/medianavigationmodel.h"
#include "session/documentsessionmediacandidateloadstate.h"
#include "session/documentsessionstate.h"

#include <QUrl>
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
    QString windowTitleFileName() const;
    bool displayedFileDeletionAvailable() const;
    bool fileDeletionInProgress() const;
    bool mediaNavigationActive() const;
    bool mediaNavigationKnown() const;
    int currentMediaNumber() const;
    int mediaCount() const;
    bool canOpenPreviousMedia() const;
    bool canOpenNextMedia() const;
    bool atKnownFirstMedia() const;
    bool atKnownLastMedia() const;
    std::optional<PredecodedImage> findPredecodedImage(const QUrl &url) const;

    void openPreviousMedia();
    void openNextMedia();
    void openMediaAtNumber(int mediaNumber);
    void deleteDisplayedFile(FileDeletionMode mode);

private:
    void connectDocuments();
    void syncImageDocumentFileDeletionProgress();
    void routeSourceUrl(const QUrl &sourceUrl);
    void openMediaUrl(const QUrl &url);
    void leaveVideoMode();
    void syncFromImageDocument();
    void syncFromVideoDocument();
    void refreshMediaNavigation();
    void loadMediaCandidates(std::function<void(std::vector<MediaNavigationCandidate>)> callback);
    void finishMediaCandidateLoad(DocumentSessionMediaCandidateLoad load,
        std::vector<MediaNavigationCandidate> candidates,
        const std::shared_ptr<std::function<void(std::vector<MediaNavigationCandidate>)>>
            &callback);
    void updateMediaBoundaryState(const std::vector<MediaNavigationCandidate> &candidates);
    void scheduleMediaPredecode(const std::vector<MediaNavigationCandidate> &candidates);
    std::vector<DisplayedPredecodeImage> displayedPredecodeImages() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;
    void startMediaDeletion(
        FileDeletionMode mode, std::vector<MediaNavigationCandidate> candidates = {});
    void finishMediaDeletion(const MediaDeletionFallbackPlan &fallbackPlan,
        FileDeletionResult result, const QString &errorString);
    QUrl currentMediaUrl() const;
    bool activeImageUsesMediaScope() const;
    bool pendingDirectImageLoadMayUseMediaScope() const;

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
    bool m_routingSource = false;
};
}

#endif

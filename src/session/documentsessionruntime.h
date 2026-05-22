// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONRUNTIME_H

#include "async/imageiojob.h"
#include "document/filedeletion.h"
#include "navigation/mediacandidateprovider.h"
#include "navigation/medianavigationmodel.h"

#include <QUrl>
#include <functional>
#include <vector>

class KiriImageDocument;
class KiriVideoDocument;
class QObject;
class QString;

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

struct DocumentSessionRuntimeDependencies {
    MediaNavigationCandidateProvider mediaCandidateProvider;
    FileOperationProvider fileOperationProvider;
};

class DocumentSessionRuntime final
{
public:
    using ChangeCallback = std::function<void(const std::vector<DocumentSessionChange> &)>;

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

    void openPreviousMedia();
    void openNextMedia();
    void openMediaAtNumber(int mediaNumber);
    void deleteDisplayedFile(FileDeletionMode mode);

private:
    void connectDocuments();
    void routeSourceUrl(const QUrl &sourceUrl);
    void openMediaUrl(const QUrl &url);
    void switchToKind(DocumentSessionKind kind);
    void leaveVideoMode();
    void syncFromImageDocument();
    void syncFromVideoDocument();
    void refreshMediaNavigation();
    void loadMediaCandidates(std::function<void(std::vector<MediaNavigationCandidate>)> callback);
    void updateMediaBoundaryState(const std::vector<MediaNavigationCandidate> &candidates);
    void startMediaDeletion(
        FileDeletionMode mode, std::vector<MediaNavigationCandidate> candidates = {});
    void finishMediaDeletion(const MediaDeletionFallbackPlan &fallbackPlan,
        FileDeletionResult result, const QString &errorString);
    QUrl currentMediaUrl() const;
    bool activeImageUsesMediaScope() const;
    void setSourceIdentity(const QUrl &url);
    void setDocumentKind(DocumentSessionKind kind);
    void setFileDeletionInProgress(bool inProgress);
    void setMediaNavigationState(MediaNavigationBoundaryState state, bool known);
    void setSessionErrorString(const QString &errorString);
    void publish(DocumentSessionChange change);
    void publish(std::vector<DocumentSessionChange> changes);

    QObject *m_owner = nullptr;
    KiriImageDocument &m_imageDocument;
    KiriVideoDocument &m_videoDocument;
    ChangeCallback m_changeCallback;
    MediaNavigationCandidateProvider m_mediaCandidateProvider;
    FileOperationProvider m_fileOperationProvider;
    ImageIoJob m_mediaCandidateJob;
    ImageIoJob m_fileDeletionJob;
    QUrl m_sourceUrl;
    DocumentSessionKind m_documentKind = DocumentSessionKind::Empty;
    MediaNavigationBoundaryState m_mediaNavigationState;
    bool m_mediaNavigationKnown = false;
    bool m_fileDeletionInProgress = false;
    bool m_routingSource = false;
    QString m_sessionErrorString;
};
}

#endif

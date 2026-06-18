// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONRUNTIME_H

#include "navigation/directmedianavigationcandidateprovider.h"
#include "navigation/directmedianavigationmodel.h"
#include "session/activenavigationprojection.h"
#include "session/documentsessionactivenavigationruntime.h"
#include "session/documentsessiondirectimagecursorsync.h"
#include "session/documentsessiondirectmediaactivityport.h"
#include "session/documentsessiondirectmedianavigationcoordinator.h"
#include "session/documentsessiondirectmedianavigationinputport.h"
#include "session/documentsessiondirectmediascopeport.h"
#include "session/documentsessiondocumentports.h"
#include "session/documentsessionimagedocumentcommandruntime.h"
#include "session/documentsessionimagedocumentsync.h"
#include "session/documentsessionmediadeletioncompletionruntime.h"
#include "session/documentsessionmediadeletionruntime.h"
#include "session/documentsessionmediaopenwithplanport.h"
#include "session/documentsessionmediaopenwithruntime.h"
#include "session/documentsessionmediapredecodeinputport.h"
#include "session/documentsessionmediapredecoderuntime.h"
#include "session/documentsessionprojectionruntime.h"
#include "session/documentsessionpublicleafsnapshotbuilder.h"
#include "session/documentsessionpublicprojection.h"
#include "session/documentsessionpublicsnapshotinputbuilder.h"
#include "session/documentsessionrouteruntime.h"
#include "session/documentsessionstate.h"
#include "session/documentsessionthumbnailruntime.h"
#include "session/documentsessionvideodocumentcommandruntime.h"
#include "session/documentsessionvideodocumentsync.h"
#include "session/documentsessionvideooutputruntime.h"
#include "system/filedeletion.h"

#include <QMetaObject>
#include <QRectF>
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

namespace kiriview {
enum class DocumentSessionDirectMediaNavigationRevealAction;
struct DocumentSessionRuntimeDependencies {
    DirectMediaNavigationCandidateProvider directMediaNavigationCandidateProvider;
    FileDeletionProvider fileDeletionProvider;
    MediaOpenWithProvider mediaOpenWithProvider;
    ActiveNavigationThumbnailRuntimeDependencies activeNavigationThumbnails;
    MediaPredecodeDependencyOverrides directMediaPredecodeDependencies;
};

class DocumentSessionRuntime final
{
public:
    using ChangeCallback = DocumentSessionState::ChangeCallback;

    DocumentSessionRuntime(QObject *owner, DocumentSessionImageDocumentSnapshotPort imageDocument,
        DocumentSessionImageDocumentCommandPort imageCommands,
        DocumentSessionVideoDocumentSnapshotPort videoDocument,
        DocumentSessionVideoDocumentCommandPort videoCommands, ChangeCallback changeCallback = {},
        DocumentSessionRuntimeDependencies dependencies = {});
    ~DocumentSessionRuntime();

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    DocumentSessionKind documentKind() const;
    quint64 publicProjectionRevision() const;
    QString errorString() const;
    QString windowTitleSubject() const;
    bool displayedFileDeletionAvailable() const;
    bool displayedMediaOpenWithAvailable() const;
    bool fileDeletionInProgress() const;
    const MediaInformationProjectionSnapshot &mediaInformationSnapshot() const;
    bool activeZoomPercentAvailable() const;
    bool activeZoomPercentKnown() const;
    qreal activeZoomPercent() const;
    bool activeZoomEditable() const;
    bool activeImageReady() const;
    bool activeImageUnsupportedOpenedCollectionVideo() const;
    bool activeImageOpenedCollectionScopeActive() const;
    bool activeImageRightToLeftReadingActive() const;
    bool activeVideoReady() const;
    bool activeVideoControlsReady() const;
    const DocumentSessionActionAvailabilityFacts &actionAvailabilityFacts() const;
    bool activeNavigationAvailable() const;
    bool activeNavigationKnown() const;
    bool activeNavigationEditable() const;
    bool activeNavigationHasTargets() const;
    bool activeNavigationDispatchAvailable() const;
    int activeNavigationCurrentNumber() const;
    int activeNavigationCount() const;
    bool canOpenPreviousActiveNavigation() const;
    bool canOpenNextActiveNavigation() const;
    bool atKnownFirstActiveNavigation() const;
    bool atKnownLastActiveNavigation() const;
    bool directMediaNavigationBoundaryActive() const;
    ActiveNavigationBoundaryScope activeNavigationBoundaryScope() const;
    ActiveNavigationRevealIntent activeNavigationRevealIntent() const;
    ActiveNavigationRevealDirection activeNavigationRevealDirection() const;
    QAbstractListModel *activeNavigationThumbnailModel() const;
    ActiveNavigationThumbnailDemandBucket activeNavigationThumbnailDemandBucket(
        int physicalMaxEdge) const;
    bool reportActiveNavigationThumbnailDemand(int number, const QUrl &url, int physicalMaxEdge,
        ActiveNavigationThumbnailDemandPriority priority, quint64 navigationGeneration);
    QString nextVideoOutputSurfaceClaimToken();
    bool reportVideoOutputSurfaceClaim(const QString &claimToken, quint64 projectionRevision,
        QObject *surfaceOwner, QObject *videoOutput, bool active, const QRectF &contentRect,
        const QRectF &sourceRect);
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
    void applyDirectMediaNavigationRevealAction(
        DocumentSessionDirectMediaNavigationRevealAction action);
    ActiveNavigationDispatchOutcome executeActiveNavigationDispatchRequest(
        ActiveNavigationDispatchRequest request, ActiveNavigationRevealContext context);
    void setPendingActiveNavigationRevealContext(ActiveNavigationRevealContext context);
    ActiveNavigationRevealContext takePendingActiveNavigationRevealContext(
        ActiveNavigationRevealIntent fallbackIntent);
    void setActiveNavigationRevealContext(ActiveNavigationRevealContext context);
    void applyActiveNavigationRevealContext(ActiveNavigationRevealContext context);
    void clearActiveNavigationRevealContextIfUnavailable();
    void connectDocuments();
    void handleImageDocumentSnapshotChanged();
    void handleVideoDocumentSnapshotChanged();
    void refreshImagePublicSnapshot();
    void refreshVideoPublicSnapshot();
    void refreshLeafPublicSnapshots();
    void syncImageDocumentFileDeletionProgress();
    void setDocumentKind(DocumentSessionKind kind);
    void recomputeActiveZoomReadout();
    void recomputeActiveZoomReadoutForKind(DocumentSessionKind kind);
    void publishActiveNavigationForImagePages();
    void recomputePublicProjection();
    void routeSourceUrl(const QUrl &sourceUrl);
    void openMediaUrl(const QUrl &url);
    void executeRoutePlan(const DocumentSessionRoutePlan &plan);
    void leaveVideoMode();
    void syncFromVideoDocument();
    void cacheDisplayedMediaPredecodeImages();
    void cancelMediaDeletion();
    void cancelMediaOpenWith();
    DocumentSessionVideoOutputAttachmentPort videoOutputAttachmentPort();
    void finishMediaDeletion(DocumentSessionMediaDeletionCompletion completion);
    bool syncDirectImageCursorFromDocument();
    ActiveZoomSnapshot activeZoomSnapshotForKind(DocumentSessionKind kind) const;
    DocumentSessionPublicSnapshotInput publicSnapshotInput(quint64 inputRevision) const;

    QObject *m_owner = nullptr;
    DocumentSessionImageDocumentSnapshotPort m_imageDocument;
    DocumentSessionImageDocumentCommandRuntime m_imageDocumentCommandRuntime;
    DocumentSessionVideoDocumentSnapshotPort m_videoDocument;
    DocumentSessionVideoDocumentCommandRuntime m_videoDocumentCommandRuntime;
    DocumentSessionState m_state;
    DocumentSessionDirectMediaScopePort m_directMediaScopePort;
    DocumentSessionDirectMediaActivityPort m_directMediaActivityPort;
    DocumentSessionDirectMediaNavigationInputPort m_directMediaNavigationInputPort;
    DocumentSessionProjectionRuntime m_projectionRuntime;
    DocumentSessionRouteRuntime m_routeRuntime;
    DocumentSessionActiveNavigationRuntime m_activeNavigationRuntime;
    DocumentSessionThumbnailRuntime m_activeNavigationThumbnailRuntime;
    DocumentSessionDirectMediaNavigationCoordinator m_directMediaNavigationCoordinator;
    DocumentSessionMediaDeletionRuntime m_mediaDeletionRuntime;
    DocumentSessionMediaDeletionCompletionRuntime m_mediaDeletionCompletionRuntime;
    DocumentSessionMediaOpenWithRuntime m_mediaOpenWithRuntime;
    DocumentSessionMediaPredecodeRuntime m_mediaPredecodeRuntime;
    std::vector<QMetaObject::Connection> m_documentConnections;
    DocumentSessionPublicImageLeafSnapshot m_imagePublicSnapshot;
    DocumentSessionPublicVideoLeafSnapshot m_videoPublicSnapshot;
    DocumentSessionMediaPredecodeInputPort m_mediaPredecodeInputPort;
    DocumentSessionMediaOpenWithPlanPort m_mediaOpenWithPlanPort;
    DocumentSessionVideoOutputRuntime m_videoOutputRuntime;
    quint64 m_publicSnapshotInputRevision = 0;
    bool m_routingSource = false;
};
}

#endif

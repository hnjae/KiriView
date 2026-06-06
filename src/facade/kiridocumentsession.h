// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIDOCUMENTSESSION_H
#define KIRIVIEW_KIRIDOCUMENTSESSION_H

#include "facade/kiriimagedocument.h"
#include "facade/kirimediainformation.h"
#include "facade/kirivideodocument.h"
#include "session/documentsessionruntime.h"

#include <QAbstractListModel>
#include <QObject>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtGlobal>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <vector>

namespace KiriView {
struct KiriDocumentSessionDependencies {
    DocumentSessionRuntimeDependencies sessionRuntime;
    ImageDocumentRuntimeDependencyOverrides imageDocument;
};
}

class KiriDocumentSession : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(quint64 publicProjectionRevision READ publicProjectionRevision NOTIFY
            publicProjectionRevisionChanged)
    Q_PROPERTY(DocumentKind documentKind READ documentKind NOTIFY documentKindChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString windowTitleSubject READ windowTitleSubject NOTIFY windowTitleSubjectChanged)
    Q_PROPERTY(QStringList openDialogNameFilters READ openDialogNameFilters CONSTANT)
    Q_PROPERTY(bool displayedFileDeletionAvailable READ displayedFileDeletionAvailable NOTIFY
            displayedFileDeletionAvailabilityChanged)
    Q_PROPERTY(bool displayedMediaOpenWithAvailable READ displayedMediaOpenWithAvailable NOTIFY
            displayedMediaOpenWithAvailabilityChanged)
    Q_PROPERTY(bool fileDeletionInProgress READ fileDeletionInProgress NOTIFY
            fileDeletionInProgressChanged)
    Q_PROPERTY(bool activeZoomPercentAvailable READ activeZoomPercentAvailable NOTIFY
            activeZoomReadoutChanged)
    Q_PROPERTY(
        bool activeZoomPercentKnown READ activeZoomPercentKnown NOTIFY activeZoomReadoutChanged)
    Q_PROPERTY(double activeZoomPercent READ activeZoomPercent NOTIFY activeZoomReadoutChanged)
    Q_PROPERTY(bool activeZoomEditable READ activeZoomEditable NOTIFY activeZoomReadoutChanged)
    Q_PROPERTY(bool activeImageReady READ activeImageReady NOTIFY activeMediaReadinessChanged)
    Q_PROPERTY(bool activeImageUnsupportedOpenedCollectionVideo READ
            activeImageUnsupportedOpenedCollectionVideo NOTIFY activeMediaReadinessChanged)
    Q_PROPERTY(bool activeImageOpenedCollectionScopeActive READ
            activeImageOpenedCollectionScopeActive NOTIFY publicProjectionRevisionChanged)
    Q_PROPERTY(bool activeImageRightToLeftReadingActive READ activeImageRightToLeftReadingActive
            NOTIFY publicProjectionRevisionChanged)
    Q_PROPERTY(bool activeVideoReady READ activeVideoReady NOTIFY publicProjectionRevisionChanged)
    Q_PROPERTY(bool activeVideoControlsReady READ activeVideoControlsReady NOTIFY
            publicProjectionRevisionChanged)
    Q_PROPERTY(bool activeNavigationAvailable READ activeNavigationAvailable NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool activeNavigationKnown READ activeNavigationKnown NOTIFY activeNavigationChanged)
    Q_PROPERTY(
        bool activeNavigationEditable READ activeNavigationEditable NOTIFY activeNavigationChanged)
    Q_PROPERTY(bool activeNavigationHasTargets READ activeNavigationHasTargets NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool activeNavigationDispatchAvailable READ activeNavigationDispatchAvailable NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(int activeNavigationCurrentNumber READ activeNavigationCurrentNumber NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(int activeNavigationCount READ activeNavigationCount NOTIFY activeNavigationChanged)
    Q_PROPERTY(bool canOpenPreviousActiveNavigation READ canOpenPreviousActiveNavigation NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool canOpenNextActiveNavigation READ canOpenNextActiveNavigation NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool atKnownFirstActiveNavigation READ atKnownFirstActiveNavigation NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool atKnownLastActiveNavigation READ atKnownLastActiveNavigation NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool directMediaNavigationBoundaryActive READ directMediaNavigationBoundaryActive
            NOTIFY activeNavigationChanged)
    Q_PROPERTY(ActiveNavigationBoundaryScope activeNavigationBoundaryScope READ
            activeNavigationBoundaryScope NOTIFY activeNavigationChanged)
    Q_PROPERTY(ActiveNavigationRevealIntent activeNavigationRevealIntent READ
            activeNavigationRevealIntent NOTIFY activeNavigationRevealIntentChanged)
    Q_PROPERTY(ActiveNavigationRevealDirection activeNavigationRevealDirection READ
            activeNavigationRevealDirection NOTIFY activeNavigationRevealDirectionChanged)
    Q_PROPERTY(QAbstractListModel *activeNavigationThumbnailModel READ
            activeNavigationThumbnailModel CONSTANT)
    Q_PROPERTY(KiriMediaInformation *mediaInformation READ mediaInformation CONSTANT)
    Q_PROPERTY(KiriImageDocument *imageDocument READ imageDocument CONSTANT)
    Q_PROPERTY(KiriVideoDocument *videoDocument READ videoDocument CONSTANT)

public:
    enum class DocumentKind {
        Empty,
        Image,
        Video,
    };
    Q_ENUM(DocumentKind)

    enum class DeletionMode {
        MoveToTrash,
        DeletePermanently,
    };
    Q_ENUM(DeletionMode)

    enum class ActiveNavigationBoundaryScope {
        NoNavigationBoundary,
        DirectMediaNavigationBoundary,
        ImageDocumentPageNavigationBoundary,
    };
    Q_ENUM(ActiveNavigationBoundaryScope)

    enum class ActiveNavigationRequestResult {
        NoActiveNavigationRequestResult,
        ActiveNavigationRequestDispatched,
        FirstActiveNavigationBoundary,
        LastActiveNavigationBoundary,
    };
    Q_ENUM(ActiveNavigationRequestResult)

    enum class ActiveNavigationRevealIntent {
        None,
        ThumbnailActivation,
        AdjacentNavigation,
        LargeJump,
        LoadOrOpen,
        ProgrammaticSync,
    };
    Q_ENUM(ActiveNavigationRevealIntent)

    enum class ActiveNavigationRevealDirection {
        None,
        Previous,
        Next,
    };
    Q_ENUM(ActiveNavigationRevealDirection)

    enum class ThumbnailDemandBucket {
        NoThumbnailDemandBucket,
        NormalThumbnailDemandBucket,
        LargeThumbnailDemandBucket,
        XLargeThumbnailDemandBucket,
        XXLargeThumbnailDemandBucket,
    };
    Q_ENUM(ThumbnailDemandBucket)

    enum class ThumbnailDemandPriority {
        VisibleThumbnailDemand,
        NearbyThumbnailDemand,
    };
    Q_ENUM(ThumbnailDemandPriority)

    enum class ThumbnailResultStatus {
        NoThumbnailResult,
        PendingThumbnailResult,
        ReadyThumbnailResult,
        UnsupportedThumbnailResult,
        FailedThumbnailResult,
    };
    Q_ENUM(ThumbnailResultStatus)

    explicit KiriDocumentSession(QObject *parent = nullptr);
    explicit KiriDocumentSession(
        KiriView::KiriDocumentSessionDependencies dependencies, QObject *parent = nullptr);
    ~KiriDocumentSession() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    quint64 publicProjectionRevision() const;
    DocumentKind documentKind() const;
    QString errorString() const;
    QString windowTitleSubject() const;
    QStringList openDialogNameFilters() const;
    bool displayedFileDeletionAvailable() const;
    bool displayedMediaOpenWithAvailable() const;
    bool fileDeletionInProgress() const;
    bool activeZoomPercentAvailable() const;
    bool activeZoomPercentKnown() const;
    double activeZoomPercent() const;
    bool activeZoomEditable() const;
    bool activeImageReady() const;
    bool activeImageUnsupportedOpenedCollectionVideo() const;
    bool activeImageOpenedCollectionScopeActive() const;
    bool activeImageRightToLeftReadingActive() const;
    bool activeVideoReady() const;
    bool activeVideoControlsReady() const;
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
    KiriMediaInformation *mediaInformation() const;
    const KiriView::MediaInformationProjectionSnapshot &mediaInformationSnapshot() const;
    const KiriView::DocumentSessionActionAvailabilityFacts &actionAvailabilityFacts() const;
    KiriImageDocument *imageDocument() const;
    KiriVideoDocument *videoDocument() const;

    Q_INVOKABLE void openPreviousActiveNavigation();
    Q_INVOKABLE void openNextActiveNavigation();
    Q_INVOKABLE void openFirstActiveNavigation();
    Q_INVOKABLE void openLastActiveNavigation();
    Q_INVOKABLE void openActiveNavigationAtNumber(int number);
    Q_INVOKABLE void openActiveNavigationThumbnailAtNumber(int number);
    Q_INVOKABLE KiriDocumentSession::ActiveNavigationRequestResult
    requestPreviousActiveNavigation();
    Q_INVOKABLE KiriDocumentSession::ActiveNavigationRequestResult requestNextActiveNavigation();
    Q_INVOKABLE QString requestPreviousActiveNavigationBoundaryText();
    Q_INVOKABLE QString requestNextActiveNavigationBoundaryText();
    Q_INVOKABLE KiriDocumentSession::ThumbnailDemandBucket activeNavigationThumbnailDemandBucket(
        int physicalMaxEdge) const;
    Q_INVOKABLE bool reportActiveNavigationThumbnailDemand(int number, QUrl url,
        int physicalMaxEdge, KiriDocumentSession::ThumbnailDemandPriority priority,
        quint64 navigationGeneration);
    Q_INVOKABLE QString nextVideoOutputSurfaceClaimToken();
    Q_INVOKABLE bool reportVideoOutputSurfaceClaim(const QString &claimToken,
        quint64 projectionRevision, QObject *surfaceOwner, QObject *videoOutput, bool active,
        QRectF contentRect, QRectF sourceRect);
    Q_INVOKABLE void deleteDisplayedFile(KiriDocumentSession::DeletionMode mode);
    Q_INVOKABLE void openCurrentMediaWith();

Q_SIGNALS:
    void publicProjectionRevisionChanged();
    void sourceUrlChanged();
    void documentKindChanged();
    void errorStringChanged();
    void windowTitleSubjectChanged();
    void displayedFileDeletionAvailabilityChanged();
    void displayedMediaOpenWithAvailabilityChanged();
    void fileDeletionInProgressChanged();
    void activeZoomReadoutChanged();
    void activeMediaReadinessChanged();
    void activeNavigationChanged();
    void activeNavigationRevealIntentChanged();
    void activeNavigationRevealDirectionChanged();
    void openWithFailed(const QString &errorString);

private:
    struct ResolvedDependenciesTag {
    };

    static KiriView::DocumentSessionImageDocumentPort imageDocumentPort(
        KiriImageDocument &document);
    static KiriView::DocumentSessionVideoDocumentPort videoDocumentPort(
        KiriVideoDocument &document);

    KiriDocumentSession(KiriView::KiriDocumentSessionDependencies dependencies,
        ResolvedDependenciesTag, QObject *parent = nullptr);

    void handleSessionChanges(const std::vector<KiriView::DocumentSessionChange> &changes);

    std::unique_ptr<KiriImageDocument> m_imageDocument;
    std::unique_ptr<KiriVideoDocument> m_videoDocument;
    std::unique_ptr<KiriView::DocumentSessionRuntime> m_runtime;
    std::unique_ptr<KiriMediaInformation> m_mediaInformation;
};

#endif

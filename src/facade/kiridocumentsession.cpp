// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"

#include "cache/imagecachepolicy.h"
#include "facade/documentsessionpublicsignals.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "facade/mediaopendialogfilters.h"
#include "localization/activenavigationboundarytext.h"
#include "rendering/displayimagestore.h"
#include "session/thumbnailimagestore.h"

#include <memory>
#include <optional>
#include <utility>

namespace {
KiriDocumentSession::DocumentKind fromRuntimeKind(kiriview::DocumentSessionKind kind)
{
    switch (kind) {
    case kiriview::DocumentSessionKind::Empty:
        return KiriDocumentSession::DocumentKind::Empty;
    case kiriview::DocumentSessionKind::Image:
        return KiriDocumentSession::DocumentKind::Image;
    case kiriview::DocumentSessionKind::Video:
        return KiriDocumentSession::DocumentKind::Video;
    }

    return KiriDocumentSession::DocumentKind::Empty;
}

kiriview::FileDeletionMode toFileDeletionMode(KiriDocumentSession::DeletionMode mode)
{
    switch (mode) {
    case KiriDocumentSession::DeletionMode::MoveToTrash:
        return kiriview::FileDeletionMode::MoveToTrash;
    case KiriDocumentSession::DeletionMode::DeletePermanently:
        return kiriview::FileDeletionMode::DeletePermanently;
    }

    return kiriview::FileDeletionMode::MoveToTrash;
}

KiriImageDocument::DeletionMode toImageDocumentDeletionMode(kiriview::FileDeletionMode mode)
{
    switch (mode) {
    case kiriview::FileDeletionMode::MoveToTrash:
        return KiriImageDocument::DeletionMode::MoveToTrash;
    case kiriview::FileDeletionMode::DeletePermanently:
        return KiriImageDocument::DeletionMode::DeletePermanently;
    }

    return KiriImageDocument::DeletionMode::MoveToTrash;
}

kiriview::DocumentSessionImageDocumentSnapshot imageDocumentSessionSnapshot(
    KiriImageDocument& document)
{
    return kiriview::DocumentSessionImageDocumentSnapshot {
        document.sourceUrl(),
        document.errorString(),
        document.windowTitleFileName(),
        document.displayedUrl(),
        document.displayedOpenedCollectionScope(),
        document.primaryImageSize(),
        document.status() == KiriImageDocument::Status::Ready,
        document.status() == KiriImageDocument::Status::Error,
        document.unsupportedOpenedCollectionVideo(),
        document.fileDeletionInProgress(),
        document.openedCollectionScopeActive(),
        document.ordinaryDirectMediaScopeActive(),
        document.containerNavigationAvailable(),
        document.twoPageModeEnabled(),
        document.twoPageModeAvailable(),
        document.rightToLeftReadingEnabled(),
        document.rightToLeftReadingAvailable(),
        document.zoomMode() == KiriImageDocument::ZoomMode::Fit,
        document.zoomMode() == KiriImageDocument::ZoomMode::FitHeight,
        document.zoomMode() == KiriImageDocument::ZoomMode::FitWidth,
        document.zoomPercentKnown(),
        document.zoomPercent(),
        document.embeddedMetadata(),
        document.pageNavigationSnapshot(),
        document.activeNavigationSnapshot(),
        document.primaryDisplayedPredecodeImage(),
        document.firstDisplayDecodeContext(),
    };
}

kiriview::DocumentSessionVideoDocumentSnapshot videoDocumentSessionSnapshot(
    KiriVideoDocument& document)
{
    return kiriview::DocumentSessionVideoDocumentSnapshot {
        document.sourceUrl(),
        document.errorString(),
        document.windowTitleFileName(),
        document.videoSize(),
        document.status() == KiriVideoDocument::Status::Ready,
        document.status() == KiriVideoDocument::Status::Error,
        document.hasVideo(),
        document.zoomPercentKnown(),
        document.zoomPercent(),
        document.embeddedMetadata(),
    };
}

KiriDocumentSession::ActiveNavigationBoundaryScope fromRuntimeBoundaryScope(
    kiriview::ActiveNavigationBoundaryScope scope)
{
    switch (scope) {
    case kiriview::ActiveNavigationBoundaryScope::DirectMedia:
        return KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary;
    case kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage:
        return KiriDocumentSession::ActiveNavigationBoundaryScope::
            ImageDocumentPageNavigationBoundary;
    case kiriview::ActiveNavigationBoundaryScope::None:
        return KiriDocumentSession::ActiveNavigationBoundaryScope::NoNavigationBoundary;
    }

    return KiriDocumentSession::ActiveNavigationBoundaryScope::NoNavigationBoundary;
}

KiriDocumentSession::ActiveNavigationRequestResult fromRuntimeRequestOutcome(
    kiriview::ActiveNavigationDispatchOutcome outcome)
{
    switch (outcome) {
    case kiriview::ActiveNavigationDispatchOutcome::NoOp:
        return KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult;
    case kiriview::ActiveNavigationDispatchOutcome::Dispatch:
        return KiriDocumentSession::ActiveNavigationRequestResult::
            ActiveNavigationRequestDispatched;
    case kiriview::ActiveNavigationDispatchOutcome::FirstBoundary:
        return KiriDocumentSession::ActiveNavigationRequestResult::FirstActiveNavigationBoundary;
    case kiriview::ActiveNavigationDispatchOutcome::LastBoundary:
        return KiriDocumentSession::ActiveNavigationRequestResult::LastActiveNavigationBoundary;
    }

    return KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult;
}

KiriDocumentSession::ActiveNavigationRevealIntent fromRuntimeRevealIntent(
    kiriview::ActiveNavigationRevealIntent intent)
{
    switch (intent) {
    case kiriview::ActiveNavigationRevealIntent::None:
        return KiriDocumentSession::ActiveNavigationRevealIntent::None;
    case kiriview::ActiveNavigationRevealIntent::ThumbnailActivation:
        return KiriDocumentSession::ActiveNavigationRevealIntent::ThumbnailActivation;
    case kiriview::ActiveNavigationRevealIntent::AdjacentNavigation:
        return KiriDocumentSession::ActiveNavigationRevealIntent::AdjacentNavigation;
    case kiriview::ActiveNavigationRevealIntent::LargeJump:
        return KiriDocumentSession::ActiveNavigationRevealIntent::LargeJump;
    case kiriview::ActiveNavigationRevealIntent::LoadOrOpen:
        return KiriDocumentSession::ActiveNavigationRevealIntent::LoadOrOpen;
    case kiriview::ActiveNavigationRevealIntent::ProgrammaticSync:
        return KiriDocumentSession::ActiveNavigationRevealIntent::ProgrammaticSync;
    }

    return KiriDocumentSession::ActiveNavigationRevealIntent::None;
}

KiriDocumentSession::ActiveNavigationRevealDirection fromRuntimeRevealDirection(
    kiriview::ActiveNavigationRevealDirection direction)
{
    switch (direction) {
    case kiriview::ActiveNavigationRevealDirection::None:
        return KiriDocumentSession::ActiveNavigationRevealDirection::None;
    case kiriview::ActiveNavigationRevealDirection::Previous:
        return KiriDocumentSession::ActiveNavigationRevealDirection::Previous;
    case kiriview::ActiveNavigationRevealDirection::Next:
        return KiriDocumentSession::ActiveNavigationRevealDirection::Next;
    }

    return KiriDocumentSession::ActiveNavigationRevealDirection::None;
}

KiriDocumentSession::ThumbnailDemandBucket fromRuntimeThumbnailDemandBucket(
    kiriview::ActiveNavigationThumbnailDemandBucket bucket)
{
    switch (bucket) {
    case kiriview::ActiveNavigationThumbnailDemandBucket::None:
        return KiriDocumentSession::ThumbnailDemandBucket::NoThumbnailDemandBucket;
    case kiriview::ActiveNavigationThumbnailDemandBucket::Normal:
        return KiriDocumentSession::ThumbnailDemandBucket::NormalThumbnailDemandBucket;
    case kiriview::ActiveNavigationThumbnailDemandBucket::Large:
        return KiriDocumentSession::ThumbnailDemandBucket::LargeThumbnailDemandBucket;
    case kiriview::ActiveNavigationThumbnailDemandBucket::XLarge:
        return KiriDocumentSession::ThumbnailDemandBucket::XLargeThumbnailDemandBucket;
    case kiriview::ActiveNavigationThumbnailDemandBucket::XXLarge:
        return KiriDocumentSession::ThumbnailDemandBucket::XXLargeThumbnailDemandBucket;
    }

    return KiriDocumentSession::ThumbnailDemandBucket::NoThumbnailDemandBucket;
}

kiriview::ActiveNavigationThumbnailDemandPriority toRuntimeThumbnailDemandPriority(
    KiriDocumentSession::ThumbnailDemandPriority priority)
{
    switch (priority) {
    case KiriDocumentSession::ThumbnailDemandPriority::VisibleThumbnailDemand:
        return kiriview::ActiveNavigationThumbnailDemandPriority::Visible;
    case KiriDocumentSession::ThumbnailDemandPriority::NearbyThumbnailDemand:
        return kiriview::ActiveNavigationThumbnailDemandPriority::Nearby;
    }

    return kiriview::ActiveNavigationThumbnailDemandPriority::Nearby;
}

template <typename Document>
kiriview::DocumentSessionDocumentSignalConnector documentSignalConnector(
    Document& document, std::vector<void (Document::*)()> signalMethods)
{
    return [&document, signalMethods = std::move(signalMethods)](
               QObject* context, kiriview::DocumentSessionDocumentChangeHandler handler) {
        std::vector<QMetaObject::Connection> connections;
        connections.reserve(signalMethods.size());
        for (void (Document::*signal)() : signalMethods) {
            connections.push_back(QObject::connect(&document, signal, context, [handler]() {
                if (handler) {
                    handler();
                }
            }));
        }
        return connections;
    };
}

kiriview::ImageDocumentRuntimeDependencyOverrides imageDocumentDependenciesWithPredecodeFinder(
    kiriview::ImageDocumentRuntimeDependencyOverrides dependencies,
    kiriview::ExternalPredecodedImageFinder predecodedImageFinder)
{
    dependencies.externalPredecodedImageFinder = std::move(predecodedImageFinder);
    dependencies.ordinaryDirectMediaPredecodeEnabled = false;
    return dependencies;
}

void inheritMissingDirectMediaPredecodeDependencies(
    kiriview::KiriDocumentSessionDependencies& dependencies)
{
    kiriview::MediaPredecodeDependencyOverrides& directMediaPredecode
        = dependencies.sessionRuntime.directMediaPredecodeDependencies;
    const kiriview::ImageDocumentRuntimeDependencyOverrides& imageDocument
        = dependencies.imageDocument;

    if (!directMediaPredecode.imageDecode.dataLoader) {
        directMediaPredecode.imageDecode.dataLoader = imageDocument.imageDecode.dataLoader;
    }
    if (!directMediaPredecode.imageDecode.dataDecoder) {
        directMediaPredecode.imageDecode.dataDecoder = imageDocument.imageDecode.dataDecoder;
    }
    if (!directMediaPredecode.powerSaver.monitor) {
        directMediaPredecode.powerSaver.monitor = imageDocument.powerSaver.monitor;
    }
    if (!directMediaPredecode.timerScheduler.currentMonotonicMsec) {
        directMediaPredecode.timerScheduler.currentMonotonicMsec
            = imageDocument.predecodeTimerScheduler.currentMonotonicMsec;
    }
    if (!directMediaPredecode.timerScheduler.singleShotTimer) {
        directMediaPredecode.timerScheduler.singleShotTimer
            = imageDocument.predecodeTimerScheduler.singleShotTimer;
    }
    if (directMediaPredecode.cacheBudgetRequest.predecodeCacheByteBudget <= 0) {
        directMediaPredecode.cacheBudgetRequest.predecodeCacheByteBudget
            = imageDocument.cacheBudgetRequest.predecodeCacheByteBudget;
    }
    if (!directMediaPredecode.systemMemorySnapshot.has_value()) {
        directMediaPredecode.systemMemorySnapshot = imageDocument.systemMemorySnapshot;
    }
}

kiriview::KiriDocumentSessionDependencies documentSessionDependenciesWithComposedDefaults(
    kiriview::KiriDocumentSessionDependencies dependencies)
{
    kiriview::ImageCacheBudgetRequest request
        = kiriview::imageDocumentCacheBudgetRequestWithDefaults(
            dependencies.imageDocument.cacheBudgetRequest);
    if (request.predecodeCacheByteBudget <= 0 || request.displayImageCacheByteBudget <= 0
        || request.thumbnailCacheByteBudget <= 0) {
        const kiriview::SystemMemorySnapshot systemMemory
            = dependencies.imageDocument.systemMemorySnapshot.value_or(
                kiriview::systemMemorySnapshot());
        const kiriview::ImageCacheBudgets cacheBudgets
            = kiriview::resolvedImageCacheBudgets(request, systemMemory);
        request.predecodeCacheByteBudget = cacheBudgets.predecodeCacheByteBudget;
        request.displayImageCacheByteBudget = cacheBudgets.displayImageCacheByteBudget;
        request.thumbnailCacheByteBudget = cacheBudgets.thumbnailCacheByteBudget;
    }
    dependencies.imageDocument.cacheBudgetRequest = request;
    kiriview::configureSharedDisplayImageStoreByteBudget(request.displayImageCacheByteBudget);
    kiriview::configureSharedThumbnailImageStoreByteBudget(request.thumbnailCacheByteBudget);
    inheritMissingDirectMediaPredecodeDependencies(dependencies);
    return dependencies;
}

kiriview::DocumentSessionPublicSignalOperations publicSignalOperations(KiriDocumentSession& session)
{
    kiriview::DocumentSessionPublicSignalOperations operations;
    operations.publicProjectionRevisionChanged
        = [&session]() { Q_EMIT session.publicProjectionRevisionChanged(); };
    operations.sourceUrlChanged = [&session]() { Q_EMIT session.sourceUrlChanged(); };
    operations.documentKindChanged = [&session]() { Q_EMIT session.documentKindChanged(); };
    operations.errorStringChanged = [&session]() { Q_EMIT session.errorStringChanged(); };
    operations.windowTitleSubjectChanged
        = [&session]() { Q_EMIT session.windowTitleSubjectChanged(); };
    operations.displayedFileDeletionAvailabilityChanged
        = [&session]() { Q_EMIT session.displayedFileDeletionAvailabilityChanged(); };
    operations.displayedMediaOpenWithAvailabilityChanged
        = [&session]() { Q_EMIT session.displayedMediaOpenWithAvailabilityChanged(); };
    operations.fileDeletionInProgressChanged
        = [&session]() { Q_EMIT session.fileDeletionInProgressChanged(); };
    operations.activeZoomReadoutChanged
        = [&session]() { Q_EMIT session.activeZoomReadoutChanged(); };
    operations.activeMediaReadinessChanged
        = [&session]() { Q_EMIT session.activeMediaReadinessChanged(); };
    operations.activeNavigationChanged = [&session]() { Q_EMIT session.activeNavigationChanged(); };
    operations.activeNavigationRevealIntentChanged
        = [&session]() { Q_EMIT session.activeNavigationRevealIntentChanged(); };
    operations.activeNavigationRevealDirectionChanged
        = [&session]() { Q_EMIT session.activeNavigationRevealDirectionChanged(); };
    return operations;
}
}

kiriview::DocumentSessionImageDocumentSnapshotPort KiriDocumentSession::imageDocumentSnapshotPort(
    KiriImageDocument& document)
{
    return kiriview::DocumentSessionImageDocumentSnapshotPort {
        [&document]() { return imageDocumentSessionSnapshot(document); },
        documentSignalConnector(document,
            { &KiriImageDocument::sourceUrlChanged, &KiriImageDocument::statusChanged,
                &KiriImageDocument::windowTitleFileNameChanged,
                &KiriImageDocument::imageSizeChanged, &KiriImageDocument::errorStringChanged,
                &KiriImageDocument::imageDocumentSourceScopeChanged,
                &KiriImageDocument::unsupportedOpenedCollectionVideoChanged,
                &KiriImageDocument::fileDeletionInProgressChanged,
                &KiriImageDocument::zoomPercentKnownChanged, &KiriImageDocument::zoomPercentChanged,
                &KiriImageDocument::zoomModeChanged, &KiriImageDocument::pageNavigationChanged,
                &KiriImageDocument::containerNavigationChanged,
                &KiriImageDocument::twoPageModeChanged,
                &KiriImageDocument::rightToLeftReadingChanged,
                &KiriImageDocument::embeddedMetadataChanged }),
    };
}

kiriview::DocumentSessionImageDocumentCommandPort KiriDocumentSession::imageDocumentCommandPort(
    KiriImageDocument& document)
{
    return kiriview::DocumentSessionImageDocumentCommandPort {
        { [&document](const QUrl& url) { document.setSourceUrl(url); } },
        { [&document]() { document.openPreviousPage(); },
            [&document]() { document.openNextPage(); },
            [&document](int pageNumber) { document.openImageAtPage(pageNumber); } },
        { [&document](kiriview::FileDeletionMode mode) {
            document.deleteDisplayedFile(toImageDocumentDeletionMode(mode));
        } },
    };
}

kiriview::DocumentSessionVideoDocumentSnapshotPort KiriDocumentSession::videoDocumentSnapshotPort(
    KiriVideoDocument& document)
{
    return kiriview::DocumentSessionVideoDocumentSnapshotPort {
        [&document]() { return videoDocumentSessionSnapshot(document); },
        documentSignalConnector(document,
            { &KiriVideoDocument::sourceUrlChanged, &KiriVideoDocument::statusChanged,
                &KiriVideoDocument::hasVideoChanged, &KiriVideoDocument::windowTitleFileNameChanged,
                &KiriVideoDocument::videoSizeChanged, &KiriVideoDocument::errorStringChanged,
                &KiriVideoDocument::zoomPercentKnownChanged, &KiriVideoDocument::zoomPercentChanged,
                &KiriVideoDocument::embeddedMetadataChanged }),
    };
}

kiriview::DocumentSessionVideoDocumentCommandPort KiriDocumentSession::videoDocumentCommandPort(
    KiriVideoDocument& document)
{
    return kiriview::DocumentSessionVideoDocumentCommandPort {
        { [&document](const QUrl& url) { document.setSourceUrl(url); } },
        { [&document]() { document.stop(); } },
        { [&document]() { return document.videoOutput(); },
            [&document](QObject* videoOutput) { document.setVideoOutput(videoOutput); },
            [&document](const QRectF& contentRect, const QRectF& sourceRect) {
                document.setVideoOutputGeometry(contentRect, sourceRect);
            } },
    };
}

KiriDocumentSession::KiriDocumentSession(QObject* parent)
    : KiriDocumentSession(kiriview::KiriDocumentSessionDependencies {}, parent)
{
}

KiriDocumentSession::KiriDocumentSession(
    kiriview::KiriDocumentSessionDependencies dependencies, QObject* parent)
    : KiriDocumentSession(documentSessionDependenciesWithComposedDefaults(std::move(dependencies)),
          ResolvedDependenciesTag {}, parent)
{
}

KiriDocumentSession::KiriDocumentSession(kiriview::KiriDocumentSessionDependencies dependencies,
    ResolvedDependenciesTag, QObject* parent)
    : QObject(parent)
    , m_imageDocument(std::make_unique<KiriImageDocument>(
          imageDocumentDependenciesWithPredecodeFinder(dependencies.imageDocument,
              [this](const QUrl& url) {
                  return m_runtime != nullptr ? m_runtime->findPredecodedImage(url)
                                              : std::optional<kiriview::PredecodedImage>();
              }),
          this))
    , m_videoDocument(std::make_unique<KiriVideoDocument>(this))
{
    m_runtime = std::make_unique<kiriview::DocumentSessionRuntime>(
        this, imageDocumentSnapshotPort(*m_imageDocument),
        imageDocumentCommandPort(*m_imageDocument), videoDocumentSnapshotPort(*m_videoDocument),
        videoDocumentCommandPort(*m_videoDocument),
        [this](const std::vector<kiriview::DocumentSessionChange>& changes) {
            handleSessionChanges(changes);
        },
        std::move(dependencies.sessionRuntime));
    m_mediaInformation = std::make_unique<KiriMediaInformation>(*this, this);
}

KiriDocumentSession::~KiriDocumentSession() = default;

QUrl KiriDocumentSession::sourceUrl() const { return m_runtime->sourceUrl(); }

void KiriDocumentSession::setSourceUrl(const QUrl& sourceUrl)
{
    m_runtime->setSourceUrl(sourceUrl);
}

quint64 KiriDocumentSession::publicProjectionRevision() const
{
    return m_runtime->publicProjectionRevision();
}

KiriDocumentSession::DocumentKind KiriDocumentSession::documentKind() const
{
    return fromRuntimeKind(m_runtime->documentKind());
}

QString KiriDocumentSession::errorString() const { return m_runtime->errorString(); }

QString KiriDocumentSession::windowTitleSubject() const { return m_runtime->windowTitleSubject(); }

QStringList KiriDocumentSession::openDialogNameFilters() const
{
    return kiriview::ordinaryMediaOpenDialogNameFilters();
}

bool KiriDocumentSession::displayedFileDeletionAvailable() const
{
    return m_runtime->displayedFileDeletionAvailable();
}

bool KiriDocumentSession::displayedMediaOpenWithAvailable() const
{
    return m_runtime->displayedMediaOpenWithAvailable();
}

bool KiriDocumentSession::fileDeletionInProgress() const
{
    return m_runtime->fileDeletionInProgress();
}

bool KiriDocumentSession::activeZoomPercentAvailable() const
{
    return m_runtime->activeZoomPercentAvailable();
}

bool KiriDocumentSession::activeZoomPercentKnown() const
{
    return m_runtime->activeZoomPercentKnown();
}

double KiriDocumentSession::activeZoomPercent() const { return m_runtime->activeZoomPercent(); }

bool KiriDocumentSession::activeZoomEditable() const { return m_runtime->activeZoomEditable(); }

bool KiriDocumentSession::activeImageReady() const { return m_runtime->activeImageReady(); }

bool KiriDocumentSession::activeImageUnsupportedOpenedCollectionVideo() const
{
    return m_runtime->activeImageUnsupportedOpenedCollectionVideo();
}

bool KiriDocumentSession::activeImageOpenedCollectionScopeActive() const
{
    return m_runtime->activeImageOpenedCollectionScopeActive();
}

bool KiriDocumentSession::activeImageRightToLeftReadingActive() const
{
    return m_runtime->activeImageRightToLeftReadingActive();
}

bool KiriDocumentSession::activeVideoReady() const { return m_runtime->activeVideoReady(); }

bool KiriDocumentSession::activeVideoControlsReady() const
{
    return m_runtime->activeVideoControlsReady();
}

bool KiriDocumentSession::activeNavigationAvailable() const
{
    return m_runtime->activeNavigationAvailable();
}

bool KiriDocumentSession::activeNavigationKnown() const
{
    return m_runtime->activeNavigationKnown();
}

bool KiriDocumentSession::activeNavigationEditable() const
{
    return m_runtime->activeNavigationEditable();
}

bool KiriDocumentSession::activeNavigationHasTargets() const
{
    return m_runtime->activeNavigationHasTargets();
}

bool KiriDocumentSession::activeNavigationDispatchAvailable() const
{
    return m_runtime->activeNavigationDispatchAvailable();
}

int KiriDocumentSession::activeNavigationCurrentNumber() const
{
    return m_runtime->activeNavigationCurrentNumber();
}

int KiriDocumentSession::activeNavigationCount() const
{
    return m_runtime->activeNavigationCount();
}

bool KiriDocumentSession::canOpenPreviousActiveNavigation() const
{
    return m_runtime->canOpenPreviousActiveNavigation();
}

bool KiriDocumentSession::canOpenNextActiveNavigation() const
{
    return m_runtime->canOpenNextActiveNavigation();
}

bool KiriDocumentSession::atKnownFirstActiveNavigation() const
{
    return m_runtime->atKnownFirstActiveNavigation();
}

bool KiriDocumentSession::atKnownLastActiveNavigation() const
{
    return m_runtime->atKnownLastActiveNavigation();
}

bool KiriDocumentSession::directMediaNavigationBoundaryActive() const
{
    return m_runtime->directMediaNavigationBoundaryActive();
}

KiriDocumentSession::ActiveNavigationBoundaryScope
KiriDocumentSession::activeNavigationBoundaryScope() const
{
    return fromRuntimeBoundaryScope(m_runtime->activeNavigationBoundaryScope());
}

KiriDocumentSession::ActiveNavigationRevealIntent
KiriDocumentSession::activeNavigationRevealIntent() const
{
    return fromRuntimeRevealIntent(m_runtime->activeNavigationRevealIntent());
}

KiriDocumentSession::ActiveNavigationRevealDirection
KiriDocumentSession::activeNavigationRevealDirection() const
{
    return fromRuntimeRevealDirection(m_runtime->activeNavigationRevealDirection());
}

QAbstractListModel* KiriDocumentSession::activeNavigationThumbnailModel() const
{
    return m_runtime->activeNavigationThumbnailModel();
}

KiriMediaInformation* KiriDocumentSession::mediaInformation() const
{
    return m_mediaInformation.get();
}

const kiriview::MediaInformationProjectionSnapshot&
KiriDocumentSession::mediaInformationSnapshot() const
{
    return m_runtime->mediaInformationSnapshot();
}

const kiriview::DocumentSessionActionAvailabilityFacts&
KiriDocumentSession::actionAvailabilityFacts() const
{
    return m_runtime->actionAvailabilityFacts();
}

KiriImageDocument* KiriDocumentSession::imageDocument() const { return m_imageDocument.get(); }

KiriVideoDocument* KiriDocumentSession::videoDocument() const { return m_videoDocument.get(); }

void KiriDocumentSession::openPreviousActiveNavigation()
{
    m_runtime->openPreviousActiveNavigation();
}

void KiriDocumentSession::openNextActiveNavigation() { m_runtime->openNextActiveNavigation(); }

void KiriDocumentSession::openFirstActiveNavigation() { m_runtime->openFirstActiveNavigation(); }

void KiriDocumentSession::openLastActiveNavigation() { m_runtime->openLastActiveNavigation(); }

void KiriDocumentSession::openActiveNavigationAtNumber(int number)
{
    m_runtime->openActiveNavigationAtNumber(number);
}

void KiriDocumentSession::openActiveNavigationThumbnailAtNumber(int number)
{
    m_runtime->openActiveNavigationThumbnailAtNumber(number);
}

KiriDocumentSession::ActiveNavigationRequestResult
KiriDocumentSession::requestPreviousActiveNavigation()
{
    return fromRuntimeRequestOutcome(m_runtime->requestPreviousActiveNavigation());
}

KiriDocumentSession::ActiveNavigationRequestResult
KiriDocumentSession::requestNextActiveNavigation()
{
    return fromRuntimeRequestOutcome(m_runtime->requestNextActiveNavigation());
}

QString KiriDocumentSession::requestPreviousActiveNavigationBoundaryText()
{
    return kiriview::activeNavigationBoundaryFeedbackText(
        m_runtime->activeNavigationBoundaryScope(), m_runtime->requestPreviousActiveNavigation());
}

QString KiriDocumentSession::requestNextActiveNavigationBoundaryText()
{
    return kiriview::activeNavigationBoundaryFeedbackText(
        m_runtime->activeNavigationBoundaryScope(), m_runtime->requestNextActiveNavigation());
}

KiriDocumentSession::ThumbnailDemandBucket
KiriDocumentSession::activeNavigationThumbnailDemandBucket(int physicalMaxEdge) const
{
    return fromRuntimeThumbnailDemandBucket(
        m_runtime->activeNavigationThumbnailDemandBucket(physicalMaxEdge));
}

bool KiriDocumentSession::reportActiveNavigationThumbnailDemand(int number, QUrl url,
    int physicalMaxEdge, ThumbnailDemandPriority priority, quint64 navigationGeneration)
{
    return m_runtime->reportActiveNavigationThumbnailDemand(number, url, physicalMaxEdge,
        toRuntimeThumbnailDemandPriority(priority), navigationGeneration);
}

QString KiriDocumentSession::nextVideoOutputSurfaceClaimToken()
{
    return m_runtime->nextVideoOutputSurfaceClaimToken();
}

bool KiriDocumentSession::reportVideoOutputSurfaceClaim(const QString& claimToken,
    quint64 projectionRevision, QObject* surfaceOwner, QObject* videoOutput, bool active,
    QRectF contentRect, QRectF sourceRect)
{
    return m_runtime->reportVideoOutputSurfaceClaim(
        claimToken, projectionRevision, surfaceOwner, videoOutput, active, contentRect, sourceRect);
}

void KiriDocumentSession::deleteDisplayedFile(DeletionMode mode)
{
    m_runtime->deleteDisplayedFile(toFileDeletionMode(mode));
}

void KiriDocumentSession::openCurrentMediaWith()
{
    m_runtime->openCurrentMediaWith(
        [this](kiriview::MediaOpenWithResult result, const kiriview::KioOperationFailure& failure) {
            if (result == kiriview::MediaOpenWithResult::Failed) {
                Q_EMIT openWithFailed(failure.userMessage);
            }
        });
}

void KiriDocumentSession::handleSessionChanges(
    const std::vector<kiriview::DocumentSessionChange>& changes)
{
    kiriview::DocumentSessionPublicSignalEmitter(publicSignalOperations(*this))
        .emitChanges(changes);
}

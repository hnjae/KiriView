// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"

#include "cache/imagecachepolicy.h"
#include "facade/documentsessionpublicsignals.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "localization/activenavigationboundarytext.h"
#include "navigation/mediaformatregistry.h"

#include <memory>
#include <optional>
#include <utility>

namespace {
KiriDocumentSession::DocumentKind fromRuntimeKind(KiriView::DocumentSessionKind kind)
{
    switch (kind) {
    case KiriView::DocumentSessionKind::Empty:
        return KiriDocumentSession::DocumentKind::Empty;
    case KiriView::DocumentSessionKind::Image:
        return KiriDocumentSession::DocumentKind::Image;
    case KiriView::DocumentSessionKind::Video:
        return KiriDocumentSession::DocumentKind::Video;
    }

    return KiriDocumentSession::DocumentKind::Empty;
}

KiriView::FileDeletionMode toFileDeletionMode(KiriDocumentSession::DeletionMode mode)
{
    switch (mode) {
    case KiriDocumentSession::DeletionMode::MoveToTrash:
        return KiriView::FileDeletionMode::MoveToTrash;
    case KiriDocumentSession::DeletionMode::DeletePermanently:
        return KiriView::FileDeletionMode::DeletePermanently;
    }

    return KiriView::FileDeletionMode::MoveToTrash;
}

KiriImageDocument::DeletionMode toImageDocumentDeletionMode(KiriView::FileDeletionMode mode)
{
    switch (mode) {
    case KiriView::FileDeletionMode::MoveToTrash:
        return KiriImageDocument::DeletionMode::MoveToTrash;
    case KiriView::FileDeletionMode::DeletePermanently:
        return KiriImageDocument::DeletionMode::DeletePermanently;
    }

    return KiriImageDocument::DeletionMode::MoveToTrash;
}

KiriDocumentSession::ActiveNavigationBoundaryScope fromRuntimeBoundaryScope(
    KiriView::ActiveNavigationBoundaryScope scope)
{
    switch (scope) {
    case KiriView::ActiveNavigationBoundaryScope::DirectMedia:
        return KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary;
    case KiriView::ActiveNavigationBoundaryScope::ImageDocumentPage:
        return KiriDocumentSession::ActiveNavigationBoundaryScope::
            ImageDocumentPageNavigationBoundary;
    case KiriView::ActiveNavigationBoundaryScope::None:
        return KiriDocumentSession::ActiveNavigationBoundaryScope::NoNavigationBoundary;
    }

    return KiriDocumentSession::ActiveNavigationBoundaryScope::NoNavigationBoundary;
}

KiriDocumentSession::ActiveNavigationRequestResult fromRuntimeRequestOutcome(
    KiriView::ActiveNavigationDispatchOutcome outcome)
{
    switch (outcome) {
    case KiriView::ActiveNavigationDispatchOutcome::NoOp:
        return KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult;
    case KiriView::ActiveNavigationDispatchOutcome::Dispatch:
        return KiriDocumentSession::ActiveNavigationRequestResult::
            ActiveNavigationRequestDispatched;
    case KiriView::ActiveNavigationDispatchOutcome::FirstBoundary:
        return KiriDocumentSession::ActiveNavigationRequestResult::FirstActiveNavigationBoundary;
    case KiriView::ActiveNavigationDispatchOutcome::LastBoundary:
        return KiriDocumentSession::ActiveNavigationRequestResult::LastActiveNavigationBoundary;
    }

    return KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult;
}

KiriDocumentSession::ActiveNavigationRevealIntent fromRuntimeRevealIntent(
    KiriView::ActiveNavigationRevealIntent intent)
{
    switch (intent) {
    case KiriView::ActiveNavigationRevealIntent::None:
        return KiriDocumentSession::ActiveNavigationRevealIntent::None;
    case KiriView::ActiveNavigationRevealIntent::ThumbnailActivation:
        return KiriDocumentSession::ActiveNavigationRevealIntent::ThumbnailActivation;
    case KiriView::ActiveNavigationRevealIntent::AdjacentNavigation:
        return KiriDocumentSession::ActiveNavigationRevealIntent::AdjacentNavigation;
    case KiriView::ActiveNavigationRevealIntent::LargeJump:
        return KiriDocumentSession::ActiveNavigationRevealIntent::LargeJump;
    case KiriView::ActiveNavigationRevealIntent::LoadOrOpen:
        return KiriDocumentSession::ActiveNavigationRevealIntent::LoadOrOpen;
    case KiriView::ActiveNavigationRevealIntent::ProgrammaticSync:
        return KiriDocumentSession::ActiveNavigationRevealIntent::ProgrammaticSync;
    }

    return KiriDocumentSession::ActiveNavigationRevealIntent::None;
}

KiriDocumentSession::ActiveNavigationRevealDirection fromRuntimeRevealDirection(
    KiriView::ActiveNavigationRevealDirection direction)
{
    switch (direction) {
    case KiriView::ActiveNavigationRevealDirection::None:
        return KiriDocumentSession::ActiveNavigationRevealDirection::None;
    case KiriView::ActiveNavigationRevealDirection::Previous:
        return KiriDocumentSession::ActiveNavigationRevealDirection::Previous;
    case KiriView::ActiveNavigationRevealDirection::Next:
        return KiriDocumentSession::ActiveNavigationRevealDirection::Next;
    }

    return KiriDocumentSession::ActiveNavigationRevealDirection::None;
}

template <typename Document, typename Signal>
KiriView::DocumentSessionDocumentSignalConnector documentSignalConnector(
    Document &document, Signal signal)
{
    return [&document, signal](
               QObject *context, KiriView::DocumentSessionDocumentChangeHandler handler) {
        return QObject::connect(&document, signal, context, [handler = std::move(handler)]() {
            if (handler) {
                handler();
            }
        });
    };
}

KiriView::DocumentSessionImageDocumentPort imageDocumentPort(KiriImageDocument &document)
{
    return KiriView::DocumentSessionImageDocumentPort {
        [&document]() { return document.sourceUrl(); },
        [&document](const QUrl &url) { document.setSourceUrl(url); },
        [&document]() { return document.errorString(); },
        [&document]() { return document.windowTitleFileName(); },
        [&document]() { return document.displayedUrl(); },
        [&document]() { return document.displayedOpenedCollectionScope(); },
        [&document]() { return document.primaryImageSize(); },
        [&document]() { return document.status() == KiriImageDocument::Status::Ready; },
        [&document]() { return document.status() == KiriImageDocument::Status::Error; },
        [&document]() { return document.fileDeletionInProgress(); },
        [&document]() { return document.ordinaryDirectMediaScopeActive(); },
        [&document]() { return document.zoomPercentKnown(); },
        [&document]() { return document.zoomPercent(); },
        [&document]() { return document.pageNavigationSnapshot(); },
        [&document]() { return document.activeNavigationSnapshot(); },
        [&document]() { return document.primaryDisplayedPredecodeImage(); },
        [&document]() { return document.firstDisplayDecodeContext(); },
        [&document]() { document.openPreviousPage(); },
        [&document]() { document.openNextPage(); },
        [&document](int pageNumber) { document.openImageAtPage(pageNumber); },
        [&document](KiriView::FileDeletionMode mode) {
            document.deleteDisplayedFile(toImageDocumentDeletionMode(mode));
        },
        KiriView::DocumentSessionImageDocumentSignals {
            documentSignalConnector(document, &KiriImageDocument::sourceUrlChanged),
            documentSignalConnector(document, &KiriImageDocument::statusChanged),
            documentSignalConnector(document, &KiriImageDocument::windowTitleFileNameChanged),
            documentSignalConnector(document, &KiriImageDocument::imageSizeChanged),
            documentSignalConnector(document, &KiriImageDocument::errorStringChanged),
            documentSignalConnector(document, &KiriImageDocument::imageDocumentSourceScopeChanged),
            documentSignalConnector(document, &KiriImageDocument::fileDeletionInProgressChanged),
            documentSignalConnector(document, &KiriImageDocument::zoomPercentKnownChanged),
            documentSignalConnector(document, &KiriImageDocument::zoomPercentChanged),
            documentSignalConnector(document, &KiriImageDocument::pageNavigationChanged),
        },
    };
}

KiriView::DocumentSessionVideoDocumentPort videoDocumentPort(KiriVideoDocument &document)
{
    return KiriView::DocumentSessionVideoDocumentPort {
        [&document]() { return document.sourceUrl(); },
        [&document](const QUrl &url) { document.setSourceUrl(url); },
        [&document]() { return document.errorString(); },
        [&document]() { return document.windowTitleFileName(); },
        [&document]() { return document.videoSize(); },
        [&document]() { return document.status() == KiriVideoDocument::Status::Ready; },
        [&document]() { return document.status() == KiriVideoDocument::Status::Error; },
        [&document]() { return document.zoomPercentKnown(); },
        [&document]() { return document.zoomPercent(); },
        [&document]() { return document.videoOutput(); },
        [&document]() { document.stop(); },
        [&document](QObject *videoOutput) { document.setVideoOutput(videoOutput); },
        KiriView::DocumentSessionVideoDocumentSignals {
            documentSignalConnector(document, &KiriVideoDocument::sourceUrlChanged),
            documentSignalConnector(document, &KiriVideoDocument::statusChanged),
            documentSignalConnector(document, &KiriVideoDocument::windowTitleFileNameChanged),
            documentSignalConnector(document, &KiriVideoDocument::videoSizeChanged),
            documentSignalConnector(document, &KiriVideoDocument::errorStringChanged),
            documentSignalConnector(document, &KiriVideoDocument::zoomPercentKnownChanged),
            documentSignalConnector(document, &KiriVideoDocument::zoomPercentChanged),
        },
    };
}

KiriView::ImageDocumentRuntimeDependencyOverrides imageDocumentDependenciesWithPredecodeFinder(
    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies,
    KiriView::ExternalPredecodedImageFinder predecodedImageFinder)
{
    dependencies.externalPredecodedImageFinder = std::move(predecodedImageFinder);
    dependencies.ordinaryDirectMediaPredecodeEnabled = false;
    return dependencies;
}

void inheritMissingDirectMediaPredecodeDependencies(
    KiriView::KiriDocumentSessionDependencies &dependencies)
{
    KiriView::MediaPredecodeDependencyOverrides &directMediaPredecode
        = dependencies.sessionRuntime.directMediaPredecodeDependencies;
    const KiriView::ImageDocumentRuntimeDependencyOverrides &imageDocument
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
    if (directMediaPredecode.cacheBudgetRequest.predecodeCacheByteBudget <= 0) {
        directMediaPredecode.cacheBudgetRequest.predecodeCacheByteBudget
            = imageDocument.cacheBudgetRequest.predecodeCacheByteBudget;
    }
}

KiriView::KiriDocumentSessionDependencies documentSessionDependenciesWithComposedDefaults(
    KiriView::KiriDocumentSessionDependencies dependencies)
{
    KiriView::ImageCacheBudgetRequest request
        = KiriView::imageDocumentCacheBudgetRequestWithDefaults(
            dependencies.imageDocument.cacheBudgetRequest);
    if (request.predecodeCacheByteBudget <= 0 || request.staticTileCacheByteBudget <= 0) {
        const KiriView::ImageCacheBudgets cacheBudgets
            = KiriView::resolvedImageCacheBudgets(request, KiriView::systemMemorySnapshot());
        request.predecodeCacheByteBudget = cacheBudgets.predecodeCacheByteBudget;
        request.staticTileCacheByteBudget = cacheBudgets.staticTileCacheByteBudget;
    }
    dependencies.imageDocument.cacheBudgetRequest = request;
    inheritMissingDirectMediaPredecodeDependencies(dependencies);
    return dependencies;
}

KiriView::DocumentSessionPublicSignalOperations publicSignalOperations(KiriDocumentSession &session)
{
    KiriView::DocumentSessionPublicSignalOperations operations;
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
    operations.activeNavigationChanged = [&session]() { Q_EMIT session.activeNavigationChanged(); };
    operations.activeNavigationRevealIntentChanged
        = [&session]() { Q_EMIT session.activeNavigationRevealIntentChanged(); };
    operations.activeNavigationRevealDirectionChanged
        = [&session]() { Q_EMIT session.activeNavigationRevealDirectionChanged(); };
    return operations;
}
}

KiriDocumentSession::KiriDocumentSession(QObject *parent)
    : KiriDocumentSession(KiriView::KiriDocumentSessionDependencies {}, parent)
{
}

KiriDocumentSession::KiriDocumentSession(
    KiriView::KiriDocumentSessionDependencies dependencies, QObject *parent)
    : KiriDocumentSession(documentSessionDependenciesWithComposedDefaults(std::move(dependencies)),
          ResolvedDependenciesTag {}, parent)
{
}

KiriDocumentSession::KiriDocumentSession(KiriView::KiriDocumentSessionDependencies dependencies,
    ResolvedDependenciesTag, QObject *parent)
    : QObject(parent)
    , m_imageDocument(std::make_unique<KiriImageDocument>(
          imageDocumentDependenciesWithPredecodeFinder(dependencies.imageDocument,
              [this](const QUrl &url) {
                  return m_runtime != nullptr ? m_runtime->findPredecodedImage(url)
                                              : std::optional<KiriView::PredecodedImage>();
              }),
          this))
    , m_videoDocument(std::make_unique<KiriVideoDocument>(this))
{
    m_runtime = std::make_unique<KiriView::DocumentSessionRuntime>(
        this, imageDocumentPort(*m_imageDocument), videoDocumentPort(*m_videoDocument),
        [this](const std::vector<KiriView::DocumentSessionChange> &changes) {
            handleSessionChanges(changes);
        },
        std::move(dependencies.sessionRuntime));
    m_mediaInformation = std::make_unique<KiriMediaInformation>(*this, this);
}

KiriDocumentSession::~KiriDocumentSession() = default;

QUrl KiriDocumentSession::sourceUrl() const { return m_runtime->sourceUrl(); }

void KiriDocumentSession::setSourceUrl(const QUrl &sourceUrl)
{
    m_runtime->setSourceUrl(sourceUrl);
}

KiriDocumentSession::DocumentKind KiriDocumentSession::documentKind() const
{
    return fromRuntimeKind(m_runtime->documentKind());
}

QString KiriDocumentSession::errorString() const { return m_runtime->errorString(); }

QString KiriDocumentSession::windowTitleSubject() const { return m_runtime->windowTitleSubject(); }

QStringList KiriDocumentSession::openDialogNameFilters() const
{
    return KiriView::ordinaryMediaOpenDialogNameFilters();
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

QAbstractListModel *KiriDocumentSession::activeNavigationThumbnailModel() const
{
    return m_runtime->activeNavigationThumbnailModel();
}

KiriMediaInformation *KiriDocumentSession::mediaInformation() const
{
    return m_mediaInformation.get();
}

KiriImageDocument *KiriDocumentSession::imageDocument() const { return m_imageDocument.get(); }

KiriVideoDocument *KiriDocumentSession::videoDocument() const { return m_videoDocument.get(); }

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
    return KiriView::activeNavigationBoundaryFeedbackText(
        m_runtime->activeNavigationBoundaryScope(), m_runtime->requestPreviousActiveNavigation());
}

QString KiriDocumentSession::requestNextActiveNavigationBoundaryText()
{
    return KiriView::activeNavigationBoundaryFeedbackText(
        m_runtime->activeNavigationBoundaryScope(), m_runtime->requestNextActiveNavigation());
}

void KiriDocumentSession::deleteDisplayedFile(DeletionMode mode)
{
    m_runtime->deleteDisplayedFile(toFileDeletionMode(mode));
}

void KiriDocumentSession::openCurrentMediaWith()
{
    m_runtime->openCurrentMediaWith(
        [this](KiriView::MediaOpenWithResult result, const QString &errorString) {
            if (result == KiriView::MediaOpenWithResult::Failed) {
                Q_EMIT openWithFailed(errorString);
            }
        });
}

void KiriDocumentSession::handleSessionChanges(
    const std::vector<KiriView::DocumentSessionChange> &changes)
{
    KiriView::DocumentSessionPublicSignalEmitter(publicSignalOperations(*this))
        .emitChanges(changes);
}

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"

#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "navigation/mediaformatregistry.h"
#include "predecode/predecodecache.h"
#include "session/documentsessionpublicsignals.h"
#include "system/systemmemory.h"

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

KiriDocumentSession::ActiveNavigationBoundaryScope fromRuntimeBoundaryScope(
    KiriView::ActiveNavigationBoundaryScope scope)
{
    switch (scope) {
    case KiriView::ActiveNavigationBoundaryScope::Media:
        return KiriDocumentSession::ActiveNavigationBoundaryScope::MediaNavigationBoundary;
    case KiriView::ActiveNavigationBoundaryScope::ImageDocument:
        return KiriDocumentSession::ActiveNavigationBoundaryScope::ImageNavigationBoundary;
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

KiriView::ImageDocumentRuntimeDependencyOverrides imageDocumentDependenciesWithPredecodeFinder(
    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies,
    KiriView::ExternalPredecodedImageFinder predecodedImageFinder)
{
    dependencies.externalPredecodedImageFinder = std::move(predecodedImageFinder);
    return dependencies;
}

qsizetype defaultPredecodeCacheByteBudget()
{
    return KiriView::PredecodeCache::byteBudgetForSystemMemory(
        KiriView::systemMemorySnapshot().physicalByteSize);
}

KiriView::DocumentSessionRuntimeDependencies documentSessionDependenciesWithPredecodeCacheBudget(
    KiriView::DocumentSessionRuntimeDependencies dependencies)
{
    if (dependencies.imageDocumentDependencies.predecodeCacheByteBudget <= 0) {
        dependencies.imageDocumentDependencies.predecodeCacheByteBudget
            = defaultPredecodeCacheByteBudget();
    }

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
    operations.fileDeletionInProgressChanged
        = [&session]() { Q_EMIT session.fileDeletionInProgressChanged(); };
    operations.activeZoomReadoutChanged
        = [&session]() { Q_EMIT session.activeZoomReadoutChanged(); };
    operations.activeNavigationChanged = [&session]() { Q_EMIT session.activeNavigationChanged(); };
    return operations;
}
}

KiriDocumentSession::KiriDocumentSession(QObject *parent)
    : KiriDocumentSession(KiriView::DocumentSessionRuntimeDependencies {}, parent)
{
}

KiriDocumentSession::KiriDocumentSession(
    KiriView::DocumentSessionRuntimeDependencies dependencies, QObject *parent)
    : KiriDocumentSession(
          documentSessionDependenciesWithPredecodeCacheBudget(std::move(dependencies)),
          ResolvedDependenciesTag {}, parent)
{
}

KiriDocumentSession::KiriDocumentSession(KiriView::DocumentSessionRuntimeDependencies dependencies,
    ResolvedDependenciesTag, QObject *parent)
    : QObject(parent)
    , m_imageDocument(std::make_unique<KiriImageDocument>(
          imageDocumentDependenciesWithPredecodeFinder(dependencies.imageDocumentDependencies,
              [this](const QUrl &url) {
                  return m_runtime != nullptr ? m_runtime->findPredecodedImage(url)
                                              : std::optional<KiriView::PredecodedImage>();
              }),
          this))
    , m_videoDocument(std::make_unique<KiriVideoDocument>(this))
{
    m_runtime = std::make_unique<KiriView::DocumentSessionRuntime>(
        this, *m_imageDocument, *m_videoDocument,
        [this](const std::vector<KiriView::DocumentSessionChange> &changes) {
            handleSessionChanges(changes);
        },
        std::move(dependencies));
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

void KiriDocumentSession::deleteDisplayedFile(DeletionMode mode)
{
    m_runtime->deleteDisplayedFile(toFileDeletionMode(mode));
}

void KiriDocumentSession::handleSessionChanges(
    const std::vector<KiriView::DocumentSessionChange> &changes)
{
    KiriView::DocumentSessionPublicSignalEmitter(publicSignalOperations(*this))
        .emitChanges(changes);
}

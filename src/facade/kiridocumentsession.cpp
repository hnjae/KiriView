// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"

#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "navigation/mediaformatregistry.h"
#include "session/documentsessionpublicsignals.h"

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

KiriView::ImageDocumentRuntimeDependencyOverrides imageDocumentDependenciesWithPredecodeFinder(
    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies,
    KiriView::ExternalPredecodedImageFinder predecodedImageFinder)
{
    dependencies.externalPredecodedImageFinder = std::move(predecodedImageFinder);
    return dependencies;
}

KiriView::DocumentSessionPublicSignalOperations publicSignalOperations(KiriDocumentSession &session)
{
    KiriView::DocumentSessionPublicSignalOperations operations;
    operations.sourceUrlChanged = [&session]() { Q_EMIT session.sourceUrlChanged(); };
    operations.documentKindChanged = [&session]() { Q_EMIT session.documentKindChanged(); };
    operations.errorStringChanged = [&session]() { Q_EMIT session.errorStringChanged(); };
    operations.windowTitleFileNameChanged
        = [&session]() { Q_EMIT session.windowTitleFileNameChanged(); };
    operations.displayedFileDeletionAvailabilityChanged
        = [&session]() { Q_EMIT session.displayedFileDeletionAvailabilityChanged(); };
    operations.fileDeletionInProgressChanged
        = [&session]() { Q_EMIT session.fileDeletionInProgressChanged(); };
    operations.mediaNavigationAvailabilityChanged
        = [&session]() { Q_EMIT session.mediaNavigationAvailabilityChanged(); };
    return operations;
}
}

KiriDocumentSession::KiriDocumentSession(QObject *parent)
    : KiriDocumentSession(KiriView::DocumentSessionRuntimeDependencies {}, parent)
{
}

KiriDocumentSession::KiriDocumentSession(
    KiriView::DocumentSessionRuntimeDependencies dependencies, QObject *parent)
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

QString KiriDocumentSession::windowTitleFileName() const
{
    return m_runtime->windowTitleFileName();
}

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

bool KiriDocumentSession::mediaNavigationActive() const
{
    return m_runtime->mediaNavigationActive();
}

bool KiriDocumentSession::mediaNavigationKnown() const { return m_runtime->mediaNavigationKnown(); }

int KiriDocumentSession::currentMediaNumber() const { return m_runtime->currentMediaNumber(); }

int KiriDocumentSession::mediaCount() const { return m_runtime->mediaCount(); }

bool KiriDocumentSession::canOpenPreviousMedia() const { return m_runtime->canOpenPreviousMedia(); }

bool KiriDocumentSession::canOpenNextMedia() const { return m_runtime->canOpenNextMedia(); }

bool KiriDocumentSession::atKnownFirstMedia() const { return m_runtime->atKnownFirstMedia(); }

bool KiriDocumentSession::atKnownLastMedia() const { return m_runtime->atKnownLastMedia(); }

KiriImageDocument *KiriDocumentSession::imageDocument() const { return m_imageDocument.get(); }

KiriVideoDocument *KiriDocumentSession::videoDocument() const { return m_videoDocument.get(); }

void KiriDocumentSession::openPreviousMedia() { m_runtime->openPreviousMedia(); }

void KiriDocumentSession::openNextMedia() { m_runtime->openNextMedia(); }

void KiriDocumentSession::openMediaAtNumber(int mediaNumber)
{
    m_runtime->openMediaAtNumber(mediaNumber);
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

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionstate.h"

#include <QtGlobal>
#include <algorithm>
#include <cstddef>
#include <utility>

namespace {
template <typename Value> bool replaceIfChanged(Value &current, const Value &next)
{
    if (current == next) {
        return false;
    }

    current = next;
    return true;
}

bool sameDirectMediaNavigationState(const KiriView::DirectMediaNavigationBoundaryState &left,
    const KiriView::DirectMediaNavigationBoundaryState &right)
{
    return left.canOpenPrevious == right.canOpenPrevious && left.canOpenNext == right.canOpenNext
        && left.atKnownFirst == right.atKnownFirst && left.atKnownLast == right.atKnownLast
        && left.currentNumber == right.currentNumber && left.count == right.count;
}

bool sameDirectMediaNavigationCandidates(
    const std::vector<KiriView::DirectMediaNavigationCandidate> &left,
    const std::vector<KiriView::DirectMediaNavigationCandidate> &right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.size(); ++index) {
        if (left.at(index).url != right.at(index).url
            || left.at(index).name != right.at(index).name) {
            return false;
        }
    }

    return true;
}

bool sameActiveNavigationSnapshot(
    const KiriView::ActiveNavigationSnapshot &left, const KiriView::ActiveNavigationSnapshot &right)
{
    return left.available == right.available && left.known == right.known
        && left.editable == right.editable && left.canOpenPrevious == right.canOpenPrevious
        && left.canOpenNext == right.canOpenNext && left.atKnownFirst == right.atKnownFirst
        && left.atKnownLast == right.atKnownLast && left.currentNumber == right.currentNumber
        && left.count == right.count;
}

bool sameActiveZoomSnapshot(
    const KiriView::ActiveZoomSnapshot &left, const KiriView::ActiveZoomSnapshot &right)
{
    return left.available == right.available && left.known == right.known
        && qAbs(left.percent - right.percent) < 0.000001 && left.editable == right.editable;
}

bool samePublicProjection(const KiriView::DocumentSessionPublicProjection &left,
    const KiriView::DocumentSessionPublicProjection &right)
{
    return left.sourceKind == right.sourceKind && left.boundaryScope == right.boundaryScope
        && sameActiveNavigationSnapshot(left.activeNavigation, right.activeNavigation)
        && left.windowTitleSubject == right.windowTitleSubject
        && left.displayedMediaOpenWithAvailable == right.displayedMediaOpenWithAvailable
        && left.displayedFileDeletionAvailable == right.displayedFileDeletionAvailable;
}

bool samePublicSnapshot(const KiriView::DocumentSessionPublicSnapshot &left,
    const KiriView::DocumentSessionPublicSnapshot &right)
{
    return left.sourceUrl == right.sourceUrl && left.documentKind == right.documentKind
        && left.errorString == right.errorString
        && left.fileDeletionInProgress == right.fileDeletionInProgress
        && sameActiveZoomSnapshot(left.activeZoom, right.activeZoom)
        && samePublicProjection(left.projection, right.projection)
        && KiriView::sameMediaInformationProjectionSnapshot(
            left.mediaInformation, right.mediaInformation)
        && left.activeNavigationRevealIntent == right.activeNavigationRevealIntent
        && left.activeNavigationRevealDirection == right.activeNavigationRevealDirection;
}

}

namespace KiriView {
DocumentSessionState::DocumentSessionState(ChangeCallback changeCallback)
    : m_changeCallback(std::move(changeCallback))
{
    m_publicSnapshot.mediaInformation = projectMediaInformation({}, 0);
}

const QUrl &DocumentSessionState::sourceUrl() const { return m_sourceUrl; }

DocumentSessionKind DocumentSessionState::documentKind() const { return m_documentKind; }

const QString &DocumentSessionState::sessionErrorString() const { return m_sourceErrorString; }

const QString &DocumentSessionState::windowTitleSubject() const
{
    return m_publicSnapshot.projection.windowTitleSubject;
}

bool DocumentSessionState::fileDeletionInProgress() const { return m_fileDeletionInProgress; }

const ActiveZoomSnapshot &DocumentSessionState::activeZoomSnapshot() const
{
    return m_publicSnapshot.activeZoom;
}

const DirectMediaNavigationBoundaryState &DocumentSessionState::directMediaNavigationState() const
{
    return m_directMediaNavigationState;
}

bool DocumentSessionState::directMediaNavigationKnown() const
{
    return m_directMediaNavigationKnown;
}

const std::vector<DirectMediaNavigationCandidate> &
DocumentSessionState::directMediaNavigationCandidates() const
{
    return m_directMediaNavigationCandidates;
}

const ActiveNavigationSnapshot &DocumentSessionState::activeNavigationSnapshot() const
{
    return m_publicSnapshot.projection.activeNavigation;
}

ActiveNavigationRevealIntent DocumentSessionState::activeNavigationRevealIntent() const
{
    return m_activeNavigationRevealIntent;
}

ActiveNavigationRevealDirection DocumentSessionState::activeNavigationRevealDirection() const
{
    return m_activeNavigationRevealDirection;
}

ActiveNavigationSourceKind DocumentSessionState::activeNavigationSourceKind() const
{
    return m_publicSnapshot.projection.sourceKind;
}

ActiveNavigationBoundaryScope DocumentSessionState::activeNavigationBoundaryScope() const
{
    return m_publicSnapshot.projection.boundaryScope;
}

bool DocumentSessionState::displayedFileDeletionAvailable() const
{
    return m_publicSnapshot.projection.displayedFileDeletionAvailable;
}

bool DocumentSessionState::displayedMediaOpenWithAvailable() const
{
    return m_publicSnapshot.projection.displayedMediaOpenWithAvailable;
}

const MediaInformationProjectionSnapshot &DocumentSessionState::mediaInformationSnapshot() const
{
    return m_publicSnapshot.mediaInformation;
}

const DocumentSessionPublicProjection &DocumentSessionState::publicProjection() const
{
    return m_publicSnapshot.projection;
}

const DocumentSessionPublicSnapshot &DocumentSessionState::publicSnapshot() const
{
    return m_publicSnapshot;
}

const DirectMediaCursor &DocumentSessionState::directMediaCursor() const
{
    return m_directMediaCursor;
}

QUrl DocumentSessionState::directMediaCursorUrl() const
{
    return effectiveDirectMediaCursorUrl(m_directMediaCursor);
}

DirectMediaScope DocumentSessionState::directMediaScope() const
{
    return directMediaScopeForCursor(m_directMediaCursor);
}

void DocumentSessionState::setSourceIdentity(const QUrl &url)
{
    replaceIfChanged(m_sourceUrl, url);
}

void DocumentSessionState::setDocumentKind(DocumentSessionKind kind)
{
    setDocumentKindAndActiveZoomSnapshot(kind, m_activeZoomSnapshot);
}

void DocumentSessionState::setDocumentKindAndActiveZoomSnapshot(
    DocumentSessionKind kind, ActiveZoomSnapshot activeZoomSnapshot)
{
    if (!replaceIfChanged(m_documentKind, kind)) {
        m_activeZoomSnapshot = activeZoomSnapshot;
        return;
    }

    m_activeZoomSnapshot = activeZoomSnapshot;
}

void DocumentSessionState::setFileDeletionInProgress(bool inProgress)
{
    replaceIfChanged(m_fileDeletionInProgress, inProgress);
}

void DocumentSessionState::setActiveZoomSnapshot(ActiveZoomSnapshot snapshot)
{
    m_activeZoomSnapshot = snapshot;
}

void DocumentSessionState::setActiveNavigationRevealIntent(ActiveNavigationRevealIntent intent)
{
    replaceIfChanged(m_activeNavigationRevealIntent, intent);
}

void DocumentSessionState::setActiveNavigationRevealDirection(
    ActiveNavigationRevealDirection direction)
{
    replaceIfChanged(m_activeNavigationRevealDirection, direction);
}

void DocumentSessionState::setDirectMediaNavigation(DirectMediaNavigationBoundaryState state,
    bool known, std::vector<DirectMediaNavigationCandidate> candidates)
{
    if (m_directMediaNavigationKnown == known
        && sameDirectMediaNavigationState(m_directMediaNavigationState, state)
        && sameDirectMediaNavigationCandidates(m_directMediaNavigationCandidates, candidates)) {
        return;
    }

    m_directMediaNavigationKnown = known;
    m_directMediaNavigationState = state;
    m_directMediaNavigationCandidates = std::move(candidates);
}

bool DocumentSessionState::updatePublicSnapshot(const DocumentSessionPublicSnapshotInput &input)
{
    return applyPublicSnapshot(
        projectDocumentSessionPublicSnapshot(input, m_publicSnapshot.revision + 1));
}

bool DocumentSessionState::updatePublicSnapshotForSourceKind(
    const DocumentSessionPublicSnapshotInput &input, ActiveNavigationSourceKind sourceKind)
{
    DocumentSessionPublicSnapshot snapshot
        = projectDocumentSessionPublicSnapshot(input, m_publicSnapshot.revision + 1);
    if (snapshot.projection.sourceKind != sourceKind) {
        return false;
    }

    applyPublicSnapshot(std::move(snapshot));
    return true;
}

bool DocumentSessionState::applyPublicSnapshot(DocumentSessionPublicSnapshot snapshot)
{
    if (samePublicSnapshot(m_publicSnapshot, snapshot)) {
        return false;
    }

    const bool sourceUrlChanged = m_publicSnapshot.sourceUrl != snapshot.sourceUrl;
    const bool documentKindChanged = m_publicSnapshot.documentKind != snapshot.documentKind;
    const bool errorStringChanged = m_publicSnapshot.errorString != snapshot.errorString;
    const bool fileDeletionInProgressChanged
        = m_publicSnapshot.fileDeletionInProgress != snapshot.fileDeletionInProgress;
    const bool activeZoomChanged
        = !sameActiveZoomSnapshot(m_publicSnapshot.activeZoom, snapshot.activeZoom);
    const bool activeNavigationChanged
        = !sameActiveNavigationSnapshot(
              m_publicSnapshot.projection.activeNavigation, snapshot.projection.activeNavigation)
        || m_publicSnapshot.projection.sourceKind != snapshot.projection.sourceKind
        || m_publicSnapshot.projection.boundaryScope != snapshot.projection.boundaryScope;
    const bool windowTitleSubjectChanged
        = m_publicSnapshot.projection.windowTitleSubject != snapshot.projection.windowTitleSubject;
    const bool displayedFileDeletionAvailabilityChanged
        = m_publicSnapshot.projection.displayedFileDeletionAvailable
        != snapshot.projection.displayedFileDeletionAvailable;
    const bool displayedMediaOpenWithAvailabilityChanged
        = m_publicSnapshot.projection.displayedMediaOpenWithAvailable
        != snapshot.projection.displayedMediaOpenWithAvailable;
    const bool activeNavigationRevealIntentChanged
        = m_publicSnapshot.activeNavigationRevealIntent != snapshot.activeNavigationRevealIntent;
    const bool activeNavigationRevealDirectionChanged
        = m_publicSnapshot.activeNavigationRevealDirection
        != snapshot.activeNavigationRevealDirection;

    m_sourceUrl = snapshot.sourceUrl;
    m_documentKind = snapshot.documentKind;
    m_fileDeletionInProgress = snapshot.fileDeletionInProgress;
    m_activeZoomSnapshot = snapshot.activeZoom;
    m_activeNavigationRevealIntent = snapshot.activeNavigationRevealIntent;
    m_activeNavigationRevealDirection = snapshot.activeNavigationRevealDirection;
    m_publicSnapshot = std::move(snapshot);

    std::vector<DocumentSessionChange> changes { DocumentSessionChange::PublicProjectionRevision };
    if (sourceUrlChanged) {
        changes.push_back(DocumentSessionChange::SourceUrl);
    }
    if (documentKindChanged) {
        changes.push_back(DocumentSessionChange::DocumentKind);
    }
    if (errorStringChanged) {
        changes.push_back(DocumentSessionChange::ErrorString);
    }
    if (fileDeletionInProgressChanged) {
        changes.push_back(DocumentSessionChange::FileDeletionInProgress);
    }
    if (activeZoomChanged) {
        changes.push_back(DocumentSessionChange::ActiveZoomReadout);
    }
    if (activeNavigationChanged) {
        changes.push_back(DocumentSessionChange::ActiveNavigation);
    }
    if (windowTitleSubjectChanged) {
        changes.push_back(DocumentSessionChange::WindowTitleSubject);
    }
    if (displayedFileDeletionAvailabilityChanged) {
        changes.push_back(DocumentSessionChange::FileDeletionAvailability);
    }
    if (displayedMediaOpenWithAvailabilityChanged) {
        changes.push_back(DocumentSessionChange::OpenWithAvailability);
    }
    if (activeNavigationRevealIntentChanged) {
        changes.push_back(DocumentSessionChange::ActiveNavigationRevealIntent);
    }
    if (activeNavigationRevealDirectionChanged) {
        changes.push_back(DocumentSessionChange::ActiveNavigationRevealDirection);
    }

    publish(std::move(changes));
    return true;
}

void DocumentSessionState::setSessionErrorString(const QString &errorString)
{
    replaceIfChanged(m_sourceErrorString, errorString);
}

bool DocumentSessionState::clearDirectMediaCursor()
{
    return KiriView::clearDirectMediaCursor(m_directMediaCursor);
}

bool DocumentSessionState::requestDirectImageCursor(const QUrl &url)
{
    return KiriView::requestDirectImageCursor(m_directMediaCursor, url);
}

bool DocumentSessionState::confirmDirectImageCursor(const QUrl &url)
{
    return KiriView::confirmDirectImageCursor(m_directMediaCursor, url);
}

bool DocumentSessionState::restoreDirectImageCursorAfterFailure()
{
    return KiriView::restoreDirectImageCursorAfterFailure(m_directMediaCursor);
}

bool DocumentSessionState::setDirectVideoCursor(const QUrl &url)
{
    return KiriView::setDirectVideoCursor(m_directMediaCursor, url);
}

void DocumentSessionState::publish(DocumentSessionChange change)
{
    publish(std::vector<DocumentSessionChange> { change });
}

void DocumentSessionState::publish(std::vector<DocumentSessionChange> changes)
{
    if (changes.empty()) {
        return;
    }

    std::vector<DocumentSessionChange> uniqueChanges;
    for (DocumentSessionChange change : changes) {
        if (std::find(uniqueChanges.cbegin(), uniqueChanges.cend(), change)
            == uniqueChanges.cend()) {
            uniqueChanges.push_back(change);
        }
    }

    if (m_changeCallback) {
        m_changeCallback(uniqueChanges);
    }
}
}

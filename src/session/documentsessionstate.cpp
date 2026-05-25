// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionstate.h"

#include "location/imageurl.h"

#include <QtGlobal>
#include <algorithm>
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

bool sameMediaNavigationState(const KiriView::MediaNavigationBoundaryState &left,
    const KiriView::MediaNavigationBoundaryState &right)
{
    return left.canOpenPrevious == right.canOpenPrevious && left.canOpenNext == right.canOpenNext
        && left.atKnownFirst == right.atKnownFirst && left.atKnownLast == right.atKnownLast
        && left.currentNumber == right.currentNumber && left.count == right.count;
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

QUrl effectiveDirectMediaCursorUrl(const KiriView::DirectMediaCursor &cursor)
{
    return !cursor.pendingUrl.isEmpty() ? cursor.pendingUrl : cursor.stableUrl;
}

bool sameEffectiveDirectMediaCursorUrl(
    const KiriView::DirectMediaCursor &left, const KiriView::DirectMediaCursor &right)
{
    return KiriView::sameNormalizedUrlOrEmpty(
        effectiveDirectMediaCursorUrl(left), effectiveDirectMediaCursorUrl(right));
}

bool replaceDirectMediaCursor(
    KiriView::DirectMediaCursor &current, KiriView::DirectMediaCursor next)
{
    if (current.stableUrl == next.stableUrl && current.pendingUrl == next.pendingUrl) {
        return false;
    }

    const bool effectiveUrlChanged = !sameEffectiveDirectMediaCursorUrl(current, next);
    next.generation = effectiveUrlChanged ? current.generation + 1 : current.generation;
    current = std::move(next);
    return effectiveUrlChanged;
}
}

namespace KiriView {
DocumentSessionState::DocumentSessionState(ChangeCallback changeCallback)
    : m_changeCallback(std::move(changeCallback))
{
}

const QUrl &DocumentSessionState::sourceUrl() const { return m_sourceUrl; }

DocumentSessionKind DocumentSessionState::documentKind() const { return m_documentKind; }

const QString &DocumentSessionState::sessionErrorString() const { return m_sessionErrorString; }

const QString &DocumentSessionState::windowTitleSubject() const { return m_windowTitleSubject; }

bool DocumentSessionState::fileDeletionInProgress() const { return m_fileDeletionInProgress; }

const ActiveZoomSnapshot &DocumentSessionState::activeZoomSnapshot() const
{
    return m_activeZoomSnapshot;
}

const MediaNavigationBoundaryState &DocumentSessionState::mediaNavigationState() const
{
    return m_mediaNavigationState;
}

bool DocumentSessionState::mediaNavigationKnown() const { return m_mediaNavigationKnown; }

const ActiveNavigationSnapshot &DocumentSessionState::activeNavigationSnapshot() const
{
    return m_activeNavigationSnapshot;
}

const DirectMediaCursor &DocumentSessionState::directMediaCursor() const
{
    return m_directMediaCursor;
}

QUrl DocumentSessionState::directMediaCursorUrl() const
{
    return effectiveDirectMediaCursorUrl(m_directMediaCursor);
}

void DocumentSessionState::setSourceIdentity(const QUrl &url)
{
    if (replaceIfChanged(m_sourceUrl, url)) {
        publish(DocumentSessionChange::SourceUrl);
    }
}

void DocumentSessionState::setDocumentKind(DocumentSessionKind kind)
{
    setDocumentKindAndActiveZoomSnapshot(kind, m_activeZoomSnapshot);
}

void DocumentSessionState::setDocumentKindAndActiveZoomSnapshot(
    DocumentSessionKind kind, ActiveZoomSnapshot activeZoomSnapshot)
{
    if (!replaceIfChanged(m_documentKind, kind)) {
        setActiveZoomSnapshot(activeZoomSnapshot);
        return;
    }

    const bool activeZoomChanged
        = !sameActiveZoomSnapshot(m_activeZoomSnapshot, activeZoomSnapshot);
    m_activeZoomSnapshot = activeZoomSnapshot;

    std::vector<DocumentSessionChange> changes { DocumentSessionChange::DocumentKind };
    if (activeZoomChanged) {
        changes.push_back(DocumentSessionChange::ActiveZoomReadout);
    }
    changes.push_back(DocumentSessionChange::ErrorString);
    changes.push_back(DocumentSessionChange::FileDeletionAvailability);
    publish(std::move(changes));
}

void DocumentSessionState::setFileDeletionInProgress(bool inProgress)
{
    if (!replaceIfChanged(m_fileDeletionInProgress, inProgress)) {
        return;
    }

    publish({ DocumentSessionChange::FileDeletionInProgress,
        DocumentSessionChange::FileDeletionAvailability });
}

void DocumentSessionState::setActiveZoomSnapshot(ActiveZoomSnapshot snapshot)
{
    if (sameActiveZoomSnapshot(m_activeZoomSnapshot, snapshot)) {
        return;
    }

    m_activeZoomSnapshot = snapshot;
    publish(DocumentSessionChange::ActiveZoomReadout);
}

void DocumentSessionState::setMediaNavigationState(MediaNavigationBoundaryState state, bool known)
{
    if (m_mediaNavigationKnown == known
        && sameMediaNavigationState(m_mediaNavigationState, state)) {
        return;
    }

    m_mediaNavigationKnown = known;
    m_mediaNavigationState = state;
}

void DocumentSessionState::setActiveNavigationSnapshot(ActiveNavigationSnapshot snapshot)
{
    if (sameActiveNavigationSnapshot(m_activeNavigationSnapshot, snapshot)) {
        return;
    }

    m_activeNavigationSnapshot = snapshot;
    publish(DocumentSessionChange::ActiveNavigation);
}

void DocumentSessionState::setSessionErrorString(const QString &errorString)
{
    if (replaceIfChanged(m_sessionErrorString, errorString)) {
        publish(DocumentSessionChange::ErrorString);
    }
}

void DocumentSessionState::setWindowTitleSubject(const QString &subject)
{
    if (replaceIfChanged(m_windowTitleSubject, subject)) {
        publish(DocumentSessionChange::WindowTitleSubject);
    }
}

bool DocumentSessionState::clearDirectMediaCursor()
{
    DirectMediaCursor next;
    next.generation = m_directMediaCursor.generation;
    return replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
}

bool DocumentSessionState::requestDirectImageCursor(const QUrl &url)
{
    DirectMediaCursor next = m_directMediaCursor;
    next.pendingUrl = url;
    return replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
}

bool DocumentSessionState::confirmDirectImageCursor(const QUrl &url)
{
    DirectMediaCursor next = m_directMediaCursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    return replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
}

bool DocumentSessionState::restoreDirectImageCursorAfterFailure()
{
    DirectMediaCursor next = m_directMediaCursor;
    next.pendingUrl = QUrl();
    return replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
}

bool DocumentSessionState::setDirectVideoCursor(const QUrl &url)
{
    DirectMediaCursor next = m_directMediaCursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    return replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
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

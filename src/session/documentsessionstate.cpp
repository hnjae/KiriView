// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionstate.h"

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

void DocumentSessionState::setActiveNavigationProjection(
    ActiveNavigationSnapshot snapshot, const QString &windowTitleSubject)
{
    std::vector<DocumentSessionChange> changes;
    if (!sameActiveNavigationSnapshot(m_activeNavigationSnapshot, snapshot)) {
        m_activeNavigationSnapshot = snapshot;
        changes.push_back(DocumentSessionChange::ActiveNavigation);
    }
    if (m_windowTitleSubject != windowTitleSubject) {
        m_windowTitleSubject = windowTitleSubject;
        changes.push_back(DocumentSessionChange::WindowTitleSubject);
    }

    publish(std::move(changes));
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

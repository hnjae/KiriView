// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionstate.h"

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

bool replaceDirectMediaCursor(
    KiriView::DirectMediaCursor &current, KiriView::DirectMediaCursor next)
{
    if (current.stableUrl == next.stableUrl && current.pendingUrl == next.pendingUrl) {
        return false;
    }

    next.generation = current.generation + 1;
    current = std::move(next);
    return true;
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

bool DocumentSessionState::fileDeletionInProgress() const { return m_fileDeletionInProgress; }

const MediaNavigationBoundaryState &DocumentSessionState::mediaNavigationState() const
{
    return m_mediaNavigationState;
}

bool DocumentSessionState::mediaNavigationKnown() const { return m_mediaNavigationKnown; }

const DirectMediaCursor &DocumentSessionState::directMediaCursor() const
{
    return m_directMediaCursor;
}

QUrl DocumentSessionState::directMediaCursorUrl() const
{
    return !m_directMediaCursor.pendingUrl.isEmpty() ? m_directMediaCursor.pendingUrl
                                                     : m_directMediaCursor.stableUrl;
}

void DocumentSessionState::setSourceIdentity(const QUrl &url)
{
    if (replaceIfChanged(m_sourceUrl, url)) {
        publish(DocumentSessionChange::SourceUrl);
    }
}

void DocumentSessionState::setDocumentKind(DocumentSessionKind kind)
{
    if (!replaceIfChanged(m_documentKind, kind)) {
        return;
    }

    publish({ DocumentSessionChange::DocumentKind, DocumentSessionChange::ActiveZoomReadout,
        DocumentSessionChange::WindowTitleFileName, DocumentSessionChange::ErrorString,
        DocumentSessionChange::FileDeletionAvailability,
        DocumentSessionChange::MediaNavigationAvailability,
        DocumentSessionChange::ActiveNavigation });
}

void DocumentSessionState::setFileDeletionInProgress(bool inProgress)
{
    if (!replaceIfChanged(m_fileDeletionInProgress, inProgress)) {
        return;
    }

    publish({ DocumentSessionChange::FileDeletionInProgress,
        DocumentSessionChange::FileDeletionAvailability });
}

void DocumentSessionState::setMediaNavigationState(MediaNavigationBoundaryState state, bool known)
{
    if (m_mediaNavigationKnown == known
        && sameMediaNavigationState(m_mediaNavigationState, state)) {
        return;
    }

    m_mediaNavigationKnown = known;
    m_mediaNavigationState = state;
    publish({ DocumentSessionChange::MediaNavigationAvailability,
        DocumentSessionChange::ActiveNavigation });
}

void DocumentSessionState::setSessionErrorString(const QString &errorString)
{
    if (replaceIfChanged(m_sessionErrorString, errorString)) {
        publish(DocumentSessionChange::ErrorString);
    }
}

void DocumentSessionState::clearDirectMediaCursor()
{
    DirectMediaCursor next;
    next.generation = m_directMediaCursor.generation;
    replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
}

void DocumentSessionState::requestDirectImageCursor(const QUrl &url)
{
    DirectMediaCursor next = m_directMediaCursor;
    next.pendingUrl = url;
    replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
}

void DocumentSessionState::confirmDirectImageCursor(const QUrl &url)
{
    DirectMediaCursor next = m_directMediaCursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
}

void DocumentSessionState::restoreDirectImageCursorAfterFailure()
{
    DirectMediaCursor next = m_directMediaCursor;
    next.pendingUrl = QUrl();
    replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
}

void DocumentSessionState::setDirectVideoCursor(const QUrl &url)
{
    DirectMediaCursor next = m_directMediaCursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    replaceDirectMediaCursor(m_directMediaCursor, std::move(next));
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

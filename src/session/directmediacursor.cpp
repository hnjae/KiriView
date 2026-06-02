// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "directmediacursor.h"

#include "navigation/directmedianavigationmodel.h"
#include "navigation/navigationlogging.h"

#include <QDebug>
#include <utility>

namespace {
bool sameDirectMediaCurrentUrlKeyOrEmpty(const QUrl &left, const QUrl &right)
{
    if (left.isEmpty() || right.isEmpty()) {
        return left.isEmpty() && right.isEmpty();
    }

    return KiriView::sameSourceKey(KiriView::sourceKeyForDirectMediaCurrentUrl(left),
        KiriView::sourceKeyForDirectMediaCurrentUrl(right));
}

bool sameDirectMediaParentUrlKeyOrEmpty(const QUrl &left, const QUrl &right)
{
    if (left.isEmpty() || right.isEmpty()) {
        return left.isEmpty() && right.isEmpty();
    }

    return KiriView::sameSourceKey(KiriView::sourceKeyForDirectMediaParentUrl(left),
        KiriView::sourceKeyForDirectMediaParentUrl(right));
}

bool sameEffectiveDirectMediaCursorUrl(
    const KiriView::DirectMediaCursor &left, const KiriView::DirectMediaCursor &right)
{
    const KiriView::DirectMediaScope leftScope = KiriView::directMediaScopeForCursor(left);
    const KiriView::DirectMediaScope rightScope = KiriView::directMediaScopeForCursor(right);
    return sameDirectMediaCurrentUrlKeyOrEmpty(leftScope.currentUrl, rightScope.currentUrl)
        && sameDirectMediaParentUrlKeyOrEmpty(leftScope.parentUrl, rightScope.parentUrl);
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

void logCursorOperation(
    const char *operation, const KiriView::DirectMediaCursor &cursor, bool effectiveUrlChanged)
{
    const KiriView::DirectMediaScope scope = KiriView::directMediaScopeForCursor(cursor);
    qCDebug(kiriviewNavigationLog)
        << "direct media cursor operation"
        << "operation" << operation << "effectiveUrlChanged" << effectiveUrlChanged << "stableUrl"
        << cursor.stableUrl << "pendingUrl" << cursor.pendingUrl << "currentUrl" << scope.currentUrl
        << "parentUrl" << scope.parentUrl << "generation" << scope.generation;
}
}

namespace KiriView {
QUrl effectiveDirectMediaCursorUrl(const DirectMediaCursor &cursor)
{
    return !cursor.pendingUrl.isEmpty() ? cursor.pendingUrl : cursor.stableUrl;
}

DirectMediaScope directMediaScopeForCursor(const DirectMediaCursor &cursor)
{
    const QUrl currentUrl = effectiveDirectMediaCursorUrl(cursor);
    return DirectMediaScope {
        currentUrl,
        directMediaNavigationParentUrl(currentUrl),
        cursor.generation,
    };
}

bool directMediaScopeMatchesCursor(const DirectMediaCursor &cursor, const DirectMediaScope &scope)
{
    const DirectMediaScope currentScope = directMediaScopeForCursor(cursor);
    return currentScope == scope;
}

bool clearDirectMediaCursor(DirectMediaCursor &cursor)
{
    DirectMediaCursor next;
    next.generation = cursor.generation;
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("clear", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}

bool requestDirectImageCursor(DirectMediaCursor &cursor, const QUrl &url)
{
    DirectMediaCursor next = cursor;
    next.pendingUrl = url;
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("request-direct-image", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}

bool confirmDirectImageCursor(DirectMediaCursor &cursor, const QUrl &url)
{
    DirectMediaCursor next = cursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("confirm-direct-image", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}

bool restoreDirectImageCursorAfterFailure(DirectMediaCursor &cursor)
{
    DirectMediaCursor next = cursor;
    next.pendingUrl = QUrl();
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("restore-direct-image-after-failure", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}

bool setDirectVideoCursor(DirectMediaCursor &cursor, const QUrl &url)
{
    DirectMediaCursor next = cursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("set-direct-video", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}
}

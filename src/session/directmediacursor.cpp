// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "directmediacursor.h"

#include "navigation/directmedianavigationmodel.h"
#include "navigation/navigationlogging.h"

#include <QDebug>
#include <utility>

namespace {
bool sameDirectMediaCurrentUrlKeyOrEmpty(const QUrl& left, const QUrl& right)
{
    if (left.isEmpty() || right.isEmpty()) {
        return left.isEmpty() && right.isEmpty();
    }

    return kiriview::sameSourceKey(kiriview::sourceKeyForDirectMediaCurrentUrl(left),
        kiriview::sourceKeyForDirectMediaCurrentUrl(right));
}

bool sameDirectMediaParentUrlKeyOrEmpty(const QUrl& left, const QUrl& right)
{
    if (left.isEmpty() || right.isEmpty()) {
        return left.isEmpty() && right.isEmpty();
    }

    return kiriview::sameSourceKey(kiriview::sourceKeyForDirectMediaParentUrl(left),
        kiriview::sourceKeyForDirectMediaParentUrl(right));
}

bool sameEffectiveDirectMediaCursorUrl(
    const kiriview::DirectMediaCursor& left, const kiriview::DirectMediaCursor& right)
{
    const kiriview::DirectMediaScope leftScope = kiriview::directMediaScopeForCursor(left);
    const kiriview::DirectMediaScope rightScope = kiriview::directMediaScopeForCursor(right);
    return sameDirectMediaCurrentUrlKeyOrEmpty(leftScope.currentUrl, rightScope.currentUrl)
        && sameDirectMediaParentUrlKeyOrEmpty(leftScope.parentUrl, rightScope.parentUrl);
}

bool replaceDirectMediaCursor(
    kiriview::DirectMediaCursor& current, kiriview::DirectMediaCursor next)
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
    const char* operation, const kiriview::DirectMediaCursor& cursor, bool effectiveUrlChanged)
{
    const kiriview::DirectMediaScope scope = kiriview::directMediaScopeForCursor(cursor);
    qCDebug(kiriviewNavigationLog)
        << "direct media cursor operation"
        << "operation" << operation << "effectiveUrlChanged" << effectiveUrlChanged << "stableUrl"
        << cursor.stableUrl << "pendingUrl" << cursor.pendingUrl << "currentUrl" << scope.currentUrl
        << "parentUrl" << scope.parentUrl << "generation" << scope.generation;
}
}

namespace kiriview {
QUrl effectiveDirectMediaCursorUrl(const DirectMediaCursor& cursor)
{
    return !cursor.pendingUrl.isEmpty() ? cursor.pendingUrl : cursor.stableUrl;
}

DirectMediaScope directMediaScopeForCursor(const DirectMediaCursor& cursor)
{
    const QUrl currentUrl = effectiveDirectMediaCursorUrl(cursor);
    return DirectMediaScope {
        currentUrl,
        directMediaNavigationParentUrl(currentUrl),
        cursor.generation,
    };
}

bool directMediaScopeMatchesCursor(const DirectMediaCursor& cursor, const DirectMediaScope& scope)
{
    const DirectMediaScope currentScope = directMediaScopeForCursor(cursor);
    return currentScope == scope;
}

bool clearDirectMediaCursor(DirectMediaCursor& cursor)
{
    DirectMediaCursor next;
    next.generation = cursor.generation;
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("clear", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}

bool requestDirectImageCursor(DirectMediaCursor& cursor, const QUrl& url)
{
    DirectMediaCursor next = cursor;
    next.pendingUrl = url;
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("request-direct-image", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}

bool confirmDirectImageCursor(DirectMediaCursor& cursor, const QUrl& url)
{
    DirectMediaCursor next = cursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("confirm-direct-image", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}

bool restoreDirectImageCursorAfterFailure(DirectMediaCursor& cursor)
{
    DirectMediaCursor next = cursor;
    next.pendingUrl = QUrl();
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("restore-direct-image-after-failure", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}

bool setDirectVideoCursor(DirectMediaCursor& cursor, const QUrl& url)
{
    DirectMediaCursor next = cursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    const bool effectiveUrlChanged = replaceDirectMediaCursor(cursor, std::move(next));
    logCursorOperation("set-direct-video", cursor, effectiveUrlChanged);
    return effectiveUrlChanged;
}
}

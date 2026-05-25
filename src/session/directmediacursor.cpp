// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "directmediacursor.h"

#include "location/imageurl.h"

#include <utility>

namespace {
bool sameEffectiveDirectMediaCursorUrl(
    const KiriView::DirectMediaCursor &left, const KiriView::DirectMediaCursor &right)
{
    return KiriView::sameNormalizedUrlOrEmpty(KiriView::effectiveDirectMediaCursorUrl(left),
        KiriView::effectiveDirectMediaCursorUrl(right));
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
QUrl effectiveDirectMediaCursorUrl(const DirectMediaCursor &cursor)
{
    return !cursor.pendingUrl.isEmpty() ? cursor.pendingUrl : cursor.stableUrl;
}

bool clearDirectMediaCursor(DirectMediaCursor &cursor)
{
    DirectMediaCursor next;
    next.generation = cursor.generation;
    return replaceDirectMediaCursor(cursor, std::move(next));
}

bool requestDirectImageCursor(DirectMediaCursor &cursor, const QUrl &url)
{
    DirectMediaCursor next = cursor;
    next.pendingUrl = url;
    return replaceDirectMediaCursor(cursor, std::move(next));
}

bool confirmDirectImageCursor(DirectMediaCursor &cursor, const QUrl &url)
{
    DirectMediaCursor next = cursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    return replaceDirectMediaCursor(cursor, std::move(next));
}

bool restoreDirectImageCursorAfterFailure(DirectMediaCursor &cursor)
{
    DirectMediaCursor next = cursor;
    next.pendingUrl = QUrl();
    return replaceDirectMediaCursor(cursor, std::move(next));
}

bool setDirectVideoCursor(DirectMediaCursor &cursor, const QUrl &url)
{
    DirectMediaCursor next = cursor;
    next.stableUrl = url;
    next.pendingUrl = QUrl();
    return replaceDirectMediaCursor(cursor, std::move(next));
}
}

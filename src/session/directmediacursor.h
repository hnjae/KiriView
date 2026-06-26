// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DIRECTMEDIACURSOR_H
#define KIRIVIEW_DIRECTMEDIACURSOR_H

#include "location/sourcekey.h"

#include <QUrl>
#include <QtGlobal>

namespace kiriview {
struct DirectMediaCursor
{
    QUrl stableUrl;
    QUrl pendingUrl;
    quint64 generation = 0;
};

struct DirectMediaScope
{
    QUrl currentUrl;
    QUrl parentUrl;
    quint64 generation = 0;

    friend bool operator==(const DirectMediaScope& left, const DirectMediaScope& right)
    {
        return sameSourceKey(sourceKeyForDirectMediaCurrentUrl(left.currentUrl),
                   sourceKeyForDirectMediaCurrentUrl(right.currentUrl))
            && sameSourceKey(sourceKeyForDirectMediaParentUrl(left.parentUrl),
                sourceKeyForDirectMediaParentUrl(right.parentUrl))
            && left.generation == right.generation;
    }
};

QUrl effectiveDirectMediaCursorUrl(const DirectMediaCursor& cursor);
DirectMediaScope directMediaScopeForCursor(const DirectMediaCursor& cursor);
bool directMediaScopeMatchesCursor(const DirectMediaCursor& cursor, const DirectMediaScope& scope);
bool clearDirectMediaCursor(DirectMediaCursor& cursor);
bool requestDirectImageCursor(DirectMediaCursor& cursor, const QUrl& url);
bool confirmDirectImageCursor(DirectMediaCursor& cursor, const QUrl& url);
bool restoreDirectImageCursorAfterFailure(DirectMediaCursor& cursor);
bool setDirectVideoCursor(DirectMediaCursor& cursor, const QUrl& url);
}

#endif

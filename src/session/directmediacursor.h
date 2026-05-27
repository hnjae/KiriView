// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DIRECTMEDIACURSOR_H
#define KIRIVIEW_DIRECTMEDIACURSOR_H

#include <QUrl>
#include <QtGlobal>

namespace KiriView {
struct DirectMediaCursor {
    QUrl stableUrl;
    QUrl pendingUrl;
    quint64 generation = 0;
};

struct DirectMediaScope {
    QUrl currentUrl;
    QUrl parentUrl;
    quint64 generation = 0;

    friend bool operator==(const DirectMediaScope &left, const DirectMediaScope &right)
    {
        return left.currentUrl == right.currentUrl && left.parentUrl == right.parentUrl
            && left.generation == right.generation;
    }
};

QUrl effectiveDirectMediaCursorUrl(const DirectMediaCursor &cursor);
DirectMediaScope directMediaScopeForCursor(const DirectMediaCursor &cursor);
bool directMediaScopeMatchesCursor(const DirectMediaCursor &cursor, const DirectMediaScope &scope);
bool clearDirectMediaCursor(DirectMediaCursor &cursor);
bool requestDirectImageCursor(DirectMediaCursor &cursor, const QUrl &url);
bool confirmDirectImageCursor(DirectMediaCursor &cursor, const QUrl &url);
bool restoreDirectImageCursorAfterFailure(DirectMediaCursor &cursor);
bool setDirectVideoCursor(DirectMediaCursor &cursor, const QUrl &url);
}

#endif

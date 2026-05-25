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

QUrl effectiveDirectMediaCursorUrl(const DirectMediaCursor &cursor);
bool clearDirectMediaCursor(DirectMediaCursor &cursor);
bool requestDirectImageCursor(DirectMediaCursor &cursor, const QUrl &url);
bool confirmDirectImageCursor(DirectMediaCursor &cursor, const QUrl &url);
bool restoreDirectImageCursorAfterFailure(DirectMediaCursor &cursor);
bool setDirectVideoCursor(DirectMediaCursor &cursor, const QUrl &url);
}

#endif

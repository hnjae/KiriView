// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SOURCEKEY_H
#define KIRIVIEW_SOURCEKEY_H

#include <QString>
#include <QUrl>

namespace KiriView {
struct SourceKey {
    QUrl normalizedUrl;
    QString identity;
    bool valid = false;
};

SourceKey sourceKeyForUrl(const QUrl &url);
SourceKey sourceKeyForDirectMediaCurrentUrl(const QUrl &url);
SourceKey sourceKeyForDirectMediaParentUrl(const QUrl &url);
bool sameSourceKey(const SourceKey &left, const SourceKey &right);
bool sameSourceUrlKey(const QUrl &left, const QUrl &right);
bool sameSourceUrlKeyOrEmpty(const QUrl &left, const QUrl &right);
}

#endif

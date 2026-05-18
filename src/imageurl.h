// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEURL_H
#define KIRIVIEW_IMAGEURL_H

#include <QString>
#include <QUrl>
#include <optional>

namespace KiriView {
QUrl normalizedUrlForIdentity(const QUrl &url);
QString normalizedUrlIdentityKey(
    const QUrl &url, QUrl::ComponentFormattingOptions options = QUrl::PrettyDecoded);
QUrl normalizedImageUrl(const QUrl &url);
std::optional<QUrl> normalizedValidImageUrl(const QUrl &url);
QUrl normalizedFileContainerUrl(const QUrl &url);
QUrl normalizedDirectoryContainerUrl(const QUrl &url);
QUrl parentUrlForContainerNavigation(const QUrl &containerUrl);
QUrl navigationSourceUrl(const QUrl &url);
bool sameNormalizedUrl(const QUrl &left, const QUrl &right);
bool sameContainerNavigationUrl(const QUrl &left, const QUrl &right);
}

#endif

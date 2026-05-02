// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEURL_H
#define KIRIVIEW_IMAGEURL_H

#include <QUrl>

namespace KiriView {
QUrl normalizedImageUrl(const QUrl &url);
QUrl normalizedFileContainerUrl(const QUrl &url);
QUrl normalizedDirectoryContainerUrl(const QUrl &url);
QUrl parentUrlForContainerNavigation(const QUrl &containerUrl);
QUrl navigationSourceUrl(const QUrl &url);
bool sameContainerNavigationUrl(const QUrl &left, const QUrl &right);
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOCATION_H
#define KIRIVIEW_IMAGELOCATION_H

#include <QImage>
#include <QUrl>

namespace KiriView {
struct DisplayedImageLocation {
    QUrl imageUrl;
    QUrl comicBookRootUrl;

    bool isEmpty() const { return imageUrl.isEmpty(); }
};

struct ImageLoadRequest {
    QUrl sourceUrl;
    QUrl displayedComicBookRootUrl;
    QUrl containerNavigationUrl;

    bool isEmpty() const { return sourceUrl.isEmpty(); }
    bool isContainerNavigation() const { return !containerNavigationUrl.isEmpty(); }
};

struct ContainerImageOpenRequest {
    QUrl imageUrl;
    QUrl containerNavigationUrl;
};

struct PredecodedImage {
    QImage image;
    DisplayedImageLocation location;
};
}

#endif

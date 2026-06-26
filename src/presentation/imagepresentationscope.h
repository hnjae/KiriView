// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONSCOPE_H
#define KIRIVIEW_IMAGEPRESENTATIONSCOPE_H

#include <QUrl>

namespace kiriview {
struct ImagePresentationScopeKey
{
    enum class Kind {
        Empty,
        DirectImage,
        OpenedCollection,
    };

    Kind kind = Kind::Empty;
    QUrl url;

    static ImagePresentationScopeKey directImage(const QUrl& url);
    static ImagePresentationScopeKey openedCollection(const QUrl& url);
    bool preservesPageNavigationZoom() const;

    friend bool operator==(
        const ImagePresentationScopeKey& left, const ImagePresentationScopeKey& right)
    {
        return left.kind == right.kind && left.url == right.url;
    }

    friend bool operator!=(
        const ImagePresentationScopeKey& left, const ImagePresentationScopeKey& right)
    {
        return !(left == right);
    }
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaformatregistry.h"

#include "bridge/rustqtconversion.h"
#include "decoding/imageformatregistry.h"
#include "kiriview/src/policy/mediaformatregistry.cxx.h"
#include "navigation/directmedianavigationmodel.h"

#include <QUrl>

namespace {
template <typename Predicate> bool matchesUrlFileNameOrString(const QUrl& url, Predicate predicate)
{
    const QString fileName = url.fileName(QUrl::PrettyDecoded);
    if (predicate(fileName)) {
        return true;
    }

    return predicate(url.toString(QUrl::PrettyDecoded));
}
}

namespace kiriview {
QStringList supportedOrdinaryMediaExtensions()
{
    return Bridge::qtStringList(rustSupportedOrdinaryMediaExtensions());
}

QStringList supportedOrdinaryMediaMimeTypes()
{
    return Bridge::qtStringList(rustSupportedOrdinaryMediaMimeTypes());
}

bool isSupportedOrdinaryMediaFileName(const QString& name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedOrdinaryMediaFileName);
}

bool isSupportedDirectVideoFileName(const QString& name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedDirectVideoFileName);
}

bool isSupportedDirectImageUrl(const QUrl& url)
{
    return matchesUrlFileNameOrString(url, kiriview::isSupportedImageFileName);
}

bool isSupportedDirectVideoUrl(const QUrl& url)
{
    return matchesUrlFileNameOrString(url, kiriview::isSupportedDirectVideoFileName);
}

bool isSupportedStillImageDirectMediaNavigationCandidate(
    const DirectMediaNavigationCandidate& candidate)
{
    return isSupportedImageFileName(candidate.name) || isSupportedDirectImageUrl(candidate.url);
}

}

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaopenwithplan.h"

#include "archive/archiveformat.h"

namespace {
bool openedCollectionScopeOpenWithAvailable(const kiriview::OpenedCollectionScopeLocation &scope)
{
    if (scope.isEmpty() || scope.isDirectory()) {
        return true;
    }

    return kiriview::archiveRootSchemeUsesKioFuse(scope.rootUrl().scheme());
}

QUrl imageOpenWithTargetUrl(const kiriview::MediaOpenWithPlanInput &input)
{
    if (!input.imageReady || input.imageDisplayedUrl.isEmpty()
        || !openedCollectionScopeOpenWithAvailable(input.openedCollectionScope)) {
        return {};
    }

    return input.imageDisplayedUrl;
}

QUrl videoOpenWithTargetUrl(const kiriview::MediaOpenWithPlanInput &input)
{
    if (!input.videoReady || input.videoSourceUrl.isEmpty()) {
        return {};
    }

    return input.videoSourceUrl;
}
}

namespace kiriview {
MediaOpenWithPlan mediaOpenWithPlan(const MediaOpenWithPlanInput &input)
{
    QUrl targetUrl;

    switch (input.documentKind) {
    case DocumentSessionKind::Image:
        targetUrl = imageOpenWithTargetUrl(input);
        break;
    case DocumentSessionKind::Video:
        targetUrl = videoOpenWithTargetUrl(input);
        break;
    case DocumentSessionKind::Empty:
        break;
    }

    if (targetUrl.isEmpty()) {
        return {};
    }

    return MediaOpenWithPlan { MediaOpenWithRequest { targetUrl } };
}
}

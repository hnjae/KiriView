// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaopenwithtarget.h"

#include "archive/archiveformat.h"

namespace {
bool imagePageScopeOpenWithAvailable(const KiriView::ImagePageScopeLocation &scope)
{
    if (scope.isEmpty() || scope.isDirectory()) {
        return true;
    }

    return KiriView::archiveRootSchemeUsesKioFuse(scope.rootUrl().scheme());
}

QUrl imageOpenWithTargetUrl(const KiriView::MediaOpenWithTargetInput &input)
{
    if (!input.imageReady || input.imageDisplayedUrl.isEmpty()
        || !imagePageScopeOpenWithAvailable(input.imagePageScope)) {
        return {};
    }

    return input.imageDisplayedUrl;
}

QUrl videoOpenWithTargetUrl(const KiriView::MediaOpenWithTargetInput &input)
{
    if (!input.videoReady || input.videoSourceUrl.isEmpty()) {
        return {};
    }

    return input.videoSourceUrl;
}
}

namespace KiriView {
QUrl mediaOpenWithTargetUrl(const MediaOpenWithTargetInput &input)
{
    switch (input.documentKind) {
    case DocumentSessionKind::Image:
        return imageOpenWithTargetUrl(input);
    case DocumentSessionKind::Video:
        return videoOpenWithTargetUrl(input);
    case DocumentSessionKind::Empty:
        return {};
    }

    return {};
}
}

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAOPENWITHPLAN_H
#define KIRIVIEW_MEDIAOPENWITHPLAN_H

#include "location/imagelocation.h"
#include "session/documentsessiontypes.h"

#include <QUrl>
#include <optional>

namespace kiriview {
struct MediaOpenWithRequest
{
    QUrl targetUrl;
};

struct MediaOpenWithPlanInput
{
    DocumentSessionKind documentKind = DocumentSessionKind::Empty;
    bool imageReady = false;
    QUrl imageDisplayedUrl;
    OpenedCollectionScopeLocation openedCollectionScope;
    bool videoReady = false;
    QUrl videoSourceUrl;
};

struct MediaOpenWithPlan
{
    std::optional<MediaOpenWithRequest> request;

    bool hasRequest() const { return request.has_value(); }
};

MediaOpenWithPlan mediaOpenWithPlan(const MediaOpenWithPlanInput& input);
}

#endif

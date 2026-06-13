// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILDEMAND_H
#define KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILDEMAND_H

#include "thumbnail/thumbnailbucket.h"

#include <QUrl>
#include <QtGlobal>
#include <optional>

namespace kiriview {
enum class ActiveNavigationThumbnailDemandPriority {
    Visible,
    Nearby,
};

struct ActiveNavigationThumbnailDemand {
    int number = 0;
    QUrl url;
    ActiveNavigationThumbnailDemandBucket bucket = ActiveNavigationThumbnailDemandBucket::None;
    ActiveNavigationThumbnailDemandPriority priority
        = ActiveNavigationThumbnailDemandPriority::Nearby;
    quint64 navigationGeneration = 0;
};

ActiveNavigationThumbnailDemandBucket activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(
    int physicalMaxEdge);

class ActiveNavigationThumbnailDemandTracker final
{
public:
    bool record(const ActiveNavigationThumbnailDemand &demand);

private:
    std::optional<ActiveNavigationThumbnailDemand> m_acceptedDemand;
};
}

#endif

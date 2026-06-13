// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "activenavigationthumbnaildemand.h"

namespace {
bool sameDemandState(const kiriview::ActiveNavigationThumbnailDemand &left,
    const kiriview::ActiveNavigationThumbnailDemand &right)
{
    return left.number == right.number && left.url == right.url && left.bucket == right.bucket
        && left.priority == right.priority
        && left.navigationGeneration == right.navigationGeneration;
}
}

namespace kiriview {
ActiveNavigationThumbnailDemandBucket activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(
    int physicalMaxEdge)
{
    if (physicalMaxEdge <= 0) {
        return ActiveNavigationThumbnailDemandBucket::None;
    }
    if (physicalMaxEdge <= 128) {
        return ActiveNavigationThumbnailDemandBucket::Normal;
    }
    if (physicalMaxEdge <= 256) {
        return ActiveNavigationThumbnailDemandBucket::Large;
    }
    if (physicalMaxEdge <= 512) {
        return ActiveNavigationThumbnailDemandBucket::XLarge;
    }

    return ActiveNavigationThumbnailDemandBucket::XXLarge;
}

bool ActiveNavigationThumbnailDemandTracker::record(const ActiveNavigationThumbnailDemand &demand)
{
    if (demand.number <= 0 || demand.url.isEmpty()
        || demand.bucket == ActiveNavigationThumbnailDemandBucket::None
        || demand.navigationGeneration == 0) {
        return false;
    }

    if (m_acceptedDemand.has_value() && sameDemandState(*m_acceptedDemand, demand)) {
        return false;
    }

    m_acceptedDemand = demand;
    return true;
}
}

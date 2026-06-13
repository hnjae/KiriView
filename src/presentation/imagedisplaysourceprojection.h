// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDISPLAYSOURCEPROJECTION_H
#define KIRIVIEW_IMAGEDISPLAYSOURCEPROJECTION_H

#include "presentation/imagepresentationscope.h"
#include "rendering/displayimagequality.h"
#include "rendering/imagerendercontext.h"

#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>

namespace kiriview {
enum class ImageDisplaySourceStatus {
    Missing,
    Ready,
    Error,
    Unsupported,
};

enum class ImageDisplaySourceRetentionStatus {
    None,
    StaleRetained,
};

enum class ImageDisplayLoadOutcome {
    Loaded,
    Error,
    Missing,
};

struct ImageDisplaySourceSlot {
    QUrl providerUrl;
    quint64 revision = 0;
    QString sourceIdentity;
    QSize originalSize;
    QSize rasterSize;
    QSize sourceSizeHint;
    DisplayImageQuality quality = DisplayImageQuality::Exact;
    ImageDisplaySourceStatus status = ImageDisplaySourceStatus::Missing;
    bool cacheEnabled = false;
    bool loadAcknowledgmentRequired = false;
    ImageDisplaySourceRetentionStatus retentionStatus = ImageDisplaySourceRetentionStatus::None;
    bool retainWhileLoadingEligible = false;
};

struct ImageDisplaySourceProjection {
    bool visible = false;
    DisplayedPageRole pageRole = DisplayedPageRole::Primary;
    QUrl providerUrl;
    quint64 revision = 0;
    QString revisionToken;
    QString sourceIdentity;
    ImagePresentationScopeKey selectedSourceScope;
    QSize originalSize;
    QSize rasterSize;
    QSize sourceSizeHint;
    DisplayImageQuality quality = DisplayImageQuality::Exact;
    ImageDisplaySourceStatus status = ImageDisplaySourceStatus::Missing;
    bool cacheEnabled = false;
    bool loadAcknowledgmentRequired = false;
    QSizeF displaySize;
    QRectF visibleItemRect;
    int rotationDegrees = 0;
    ImageDisplaySourceRetentionStatus retentionStatus = ImageDisplaySourceRetentionStatus::None;
    bool retainWhileLoadingEligible = false;
};

QString imageDisplaySourceRevisionToken(quint64 revision);
}

#endif

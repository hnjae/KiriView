// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGENAVIGATIONPLAN_H
#define KIRIVIEW_IMAGENAVIGATIONPLAN_H

#include "imagecontaineropenplan.h"
#include "imagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <variant>
#include <vector>

namespace KiriView {
using ContainerNavigationError = ImageContainerOpenError;

struct OpenImageNavigationUrlEffect {
    ImageNavigationTarget target;
};

struct OpenContainerImageNavigationEffect {
    ImageNavigationTarget target;
    QUrl containerUrl;
};

struct ReportContainerNavigationErrorEffect {
    QUrl containerUrl;
    ContainerNavigationError error = ContainerNavigationError::Generic;
    QString errorString;
};

struct ClearCurrentImageNavigationEffect {
};

using ImageNavigationEffect
    = std::variant<OpenImageNavigationUrlEffect, OpenContainerImageNavigationEffect,
        ReportContainerNavigationErrorEffect, ClearCurrentImageNavigationEffect>;

using ImageNavigationPlan = std::vector<ImageNavigationEffect>;
}

#endif

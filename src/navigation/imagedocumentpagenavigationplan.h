// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATIONPLAN_H
#define KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATIONPLAN_H

#include "imagecontaineropenplan.h"
#include "imagedocumentpagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <variant>
#include <vector>

namespace KiriView {
using ContainerNavigationError = ImageContainerOpenError;

struct OpenImageDocumentPageUrlEffect {
    ImageDocumentPageTarget target;
};

struct OpenContainerImageDocumentPageNavigationEffect {
    ImageDocumentPageTarget target;
    QUrl containerUrl;
};

struct ReportContainerNavigationErrorEffect {
    QUrl containerUrl;
    ContainerNavigationError error = ContainerNavigationError::Generic;
    QString errorString;
};

struct ClearCurrentImageDocumentPageNavigationEffect {
};

using ImageDocumentPageNavigationEffect
    = std::variant<OpenImageDocumentPageUrlEffect, OpenContainerImageDocumentPageNavigationEffect,
        ReportContainerNavigationErrorEffect, ClearCurrentImageDocumentPageNavigationEffect>;

using ImageDocumentPageNavigationPlan = std::vector<ImageDocumentPageNavigationEffect>;
}

#endif

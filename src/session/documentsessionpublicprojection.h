// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONPUBLICPROJECTION_H
#define KIRIVIEW_DOCUMENTSESSIONPUBLICPROJECTION_H

#include "session/activenavigationprojection.h"
#include "session/documentsessionstate.h"

#include <QSize>
#include <QString>

namespace KiriView {
struct DocumentSessionPublicProjectionInput {
    DocumentSessionKind documentKind = DocumentSessionKind::Empty;
    bool directImageLoadMayUseMediaScope = false;
    bool imageSourceMayRepresentDocument = false;
    bool fileDeletionInProgress = false;
    MediaActiveNavigationInput mediaNavigation;
    ImageDocumentActiveNavigationInput imageDocumentNavigation;
    QString imageWindowTitleFileName;
    QSize imageDirectMediaSize;
    QString videoWindowTitleFileName;
    QSize videoDirectMediaSize;
    bool imageReadyForDeletion = false;
    bool directImageReplacementPending = false;
    bool videoSourcePresent = false;
    bool videoError = false;
};

DocumentSessionPublicProjection projectDocumentSessionPublicState(
    DocumentSessionPublicProjectionInput input);
}

#endif

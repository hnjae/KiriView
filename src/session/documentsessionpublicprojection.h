// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONPUBLICPROJECTION_H
#define KIRIVIEW_DOCUMENTSESSIONPUBLICPROJECTION_H

#include "metadata/embeddedmetadata.h"
#include "session/activenavigationprojection.h"
#include "session/documentsessiontypes.h"

#include <QSize>
#include <QString>
#include <QUrl>

namespace KiriView {
struct DocumentSessionPublicProjectionInput {
    DocumentSessionKind documentKind = DocumentSessionKind::Empty;
    bool directImageLoadMayUseImageDocumentSourceScope = false;
    bool imageSourceMayRepresentDocument = false;
    bool fileDeletionInProgress = false;
    DirectMediaActiveNavigationInput directMediaNavigation;
    ImageDocumentPageActiveNavigationSnapshot imageDocumentPageNavigation;
    QString imageWindowTitleFileName;
    QSize imageDirectMediaSize;
    QString videoWindowTitleFileName;
    QSize videoDirectMediaSize;
    bool imageReadyForDeletion = false;
    bool directImageReplacementPending = false;
    bool videoSourcePresent = false;
    bool videoError = false;
    bool displayedMediaOpenWithAvailable = false;
};

struct DocumentSessionPublicSessionLeafSnapshot {
    QUrl sourceUrl;
    DocumentSessionKind documentKind = DocumentSessionKind::Empty;
    QString sessionErrorString;
    bool fileDeletionInProgress = false;
    bool directImageLoadMayUseImageDocumentSourceScope = false;
    DirectMediaActiveNavigationInput directMediaNavigation;
    ActiveNavigationRevealIntent activeNavigationRevealIntent = ActiveNavigationRevealIntent::None;
    ActiveNavigationRevealDirection activeNavigationRevealDirection
        = ActiveNavigationRevealDirection::None;
};

struct DocumentSessionPublicImageLeafSnapshot {
    QUrl sourceUrl;
    bool sourceMayRepresentDocument = false;
    ImageDocumentPageActiveNavigationSnapshot pageNavigation;
    ImageDocumentPageNavigationSnapshot pageNavigationRows;
    QUrl displayedUrl;
    OpenedCollectionScopeLocation displayedOpenedCollectionScope;
    QString windowTitleFileName;
    QSize directMediaSize;
    EmbeddedMetadata embeddedMetadata;
    bool readyForDeletion = false;
    bool readyForInformation = false;
    bool error = false;
    bool fileDeletionInProgress = false;
    bool openedCollectionScopeActive = false;
    bool ordinaryDirectMediaScopeActive = false;
    bool unsupportedOpenedCollectionVideo = false;
    bool directImageReplacementPending = false;
    bool containerNavigationAvailable = false;
    bool twoPageModeEnabled = false;
    bool twoPageModeAvailable = false;
    bool rightToLeftReadingEnabled = false;
    bool rightToLeftReadingAvailable = false;
    bool fitModeSelected = false;
    bool fitHeightModeSelected = false;
    bool fitWidthModeSelected = false;
    bool zoomPercentKnown = false;
    qreal zoomPercent = 0.0;
    QString errorString;
};

struct DocumentSessionPublicVideoLeafSnapshot {
    QUrl sourceUrl;
    QString windowTitleFileName;
    QSize directMediaSize;
    EmbeddedMetadata embeddedMetadata;
    bool ready = false;
    bool hasVideo = false;
    bool sourcePresent = false;
    bool error = false;
    bool zoomPercentKnown = false;
    int zoomPercent = 0;
    QString errorString;
};

struct DocumentSessionPublicOperationAvailabilitySnapshot {
    bool displayedMediaOpenWithAvailable = false;
};

struct DocumentSessionPublicSnapshotInput {
    quint64 inputRevision = 0;
    DocumentSessionPublicSessionLeafSnapshot session;
    DocumentSessionPublicImageLeafSnapshot image;
    DocumentSessionPublicVideoLeafSnapshot video;
    DocumentSessionPublicOperationAvailabilitySnapshot operations;
    MediaInformationProjectionInput mediaInformation;
};

DocumentSessionPublicProjection projectDocumentSessionPublicState(
    DocumentSessionPublicProjectionInput input);

DocumentSessionPublicSnapshot projectDocumentSessionPublicSnapshot(
    const DocumentSessionPublicSnapshotInput &input, quint64 revision);
}

#endif

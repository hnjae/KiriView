// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionpublicprojection.h"

#include "session/windowtitleprojection.h"

namespace {
bool imageDocumentPagesArePresent(const KiriView::ImageDocumentPageActiveNavigationSnapshot &input)
{
    return input.currentNumber > 0 || input.count > 0;
}

KiriView::ActiveNavigationSourceKind sourceKindForInput(
    const KiriView::DocumentSessionPublicProjectionInput &input)
{
    switch (input.documentKind) {
    case KiriView::DocumentSessionKind::Video:
        return KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia;
    case KiriView::DocumentSessionKind::Image:
        if (input.directImageLoadMayUseImageDocumentSourceScope) {
            return KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia;
        }
        if (imageDocumentPagesArePresent(input.imageDocumentPageNavigation)
            || input.imageSourceMayRepresentDocument) {
            return KiriView::ActiveNavigationSourceKind::ImageDocumentPages;
        }
        return KiriView::ActiveNavigationSourceKind::None;
    case KiriView::DocumentSessionKind::Empty:
        return KiriView::ActiveNavigationSourceKind::None;
    }

    return KiriView::ActiveNavigationSourceKind::None;
}

QSize directMediaSizeForInput(const KiriView::DocumentSessionPublicProjectionInput &input,
    KiriView::ActiveNavigationSourceKind sourceKind)
{
    if (sourceKind != KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia) {
        return {};
    }

    switch (input.documentKind) {
    case KiriView::DocumentSessionKind::Image:
        return input.imageDirectMediaSize;
    case KiriView::DocumentSessionKind::Video:
        return input.videoDirectMediaSize;
    case KiriView::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

QString windowTitleFileNameForInput(const KiriView::DocumentSessionPublicProjectionInput &input)
{
    switch (input.documentKind) {
    case KiriView::DocumentSessionKind::Image:
        return input.imageWindowTitleFileName;
    case KiriView::DocumentSessionKind::Video:
        return input.videoWindowTitleFileName;
    case KiriView::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

bool displayedFileDeletionAvailableForInput(
    const KiriView::DocumentSessionPublicProjectionInput &input)
{
    if (input.fileDeletionInProgress) {
        return false;
    }

    switch (input.documentKind) {
    case KiriView::DocumentSessionKind::Image:
        if (input.directImageLoadMayUseImageDocumentSourceScope
            && input.directImageReplacementPending) {
            return false;
        }
        return input.imageReadyForDeletion;
    case KiriView::DocumentSessionKind::Video:
        return input.videoSourcePresent && !input.videoError;
    case KiriView::DocumentSessionKind::Empty:
        return false;
    }

    return false;
}

KiriView::ActiveZoomSnapshot activeZoomForInput(
    const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    switch (input.session.documentKind) {
    case KiriView::DocumentSessionKind::Image:
        if (!input.image.zoomPercentKnown) {
            return {};
        }
        return KiriView::ActiveZoomSnapshot { true, true, input.image.zoomPercent, true };
    case KiriView::DocumentSessionKind::Video:
        return KiriView::ActiveZoomSnapshot {
            true,
            input.video.zoomPercentKnown,
            input.video.zoomPercentKnown ? qreal(input.video.zoomPercent) : 0.0,
            false,
        };
    case KiriView::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

bool activeImageReadyForInput(const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    return input.session.documentKind == KiriView::DocumentSessionKind::Image
        && input.image.readyForInformation && !input.image.unsupportedOpenedCollectionVideo;
}

bool activeImageOpenedCollectionScopeActiveForInput(
    const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    return input.session.documentKind == KiriView::DocumentSessionKind::Image
        && input.image.openedCollectionScopeActive;
}

bool activeImageRightToLeftReadingActiveForInput(
    const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    return input.session.documentKind == KiriView::DocumentSessionKind::Image
        && input.image.rightToLeftReadingEnabled && input.image.rightToLeftReadingAvailable;
}

bool activeVideoReadyForInput(const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    return input.session.documentKind == KiriView::DocumentSessionKind::Video && input.video.ready;
}

bool activeVideoControlsReadyForInput(const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    return activeVideoReadyForInput(input) && input.video.hasVideo;
}

KiriView::DocumentSessionActionAvailabilityFacts actionAvailabilityFactsForInput(
    const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    if (input.session.documentKind != KiriView::DocumentSessionKind::Image) {
        return {};
    }

    return KiriView::DocumentSessionActionAvailabilityFacts {
        activeImageReadyForInput(input),
        input.image.containerNavigationAvailable,
        input.image.twoPageModeEnabled,
        input.image.twoPageModeAvailable,
        input.image.rightToLeftReadingEnabled,
        input.image.rightToLeftReadingAvailable,
        input.image.fitModeSelected,
        input.image.fitHeightModeSelected,
        input.image.fitWidthModeSelected,
    };
}

QString errorStringForInput(const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    if (!input.session.sessionErrorString.isEmpty()) {
        return input.session.sessionErrorString;
    }

    switch (input.session.documentKind) {
    case KiriView::DocumentSessionKind::Image:
        return input.image.errorString;
    case KiriView::DocumentSessionKind::Video:
        return input.video.errorString;
    case KiriView::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

KiriView::DocumentSessionPublicProjectionInput projectionInputForSnapshotInput(
    const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    return KiriView::DocumentSessionPublicProjectionInput {
        input.session.documentKind,
        input.session.directImageLoadMayUseImageDocumentSourceScope,
        input.image.sourceMayRepresentDocument,
        input.session.fileDeletionInProgress,
        input.session.directMediaNavigation,
        input.image.pageNavigation,
        input.image.windowTitleFileName,
        input.image.directMediaSize,
        input.video.windowTitleFileName,
        input.video.directMediaSize,
        input.image.readyForDeletion,
        input.image.directImageReplacementPending,
        input.video.sourcePresent,
        input.video.error,
        input.operations.displayedMediaOpenWithAvailable,
    };
}

KiriView::MediaInformationProjectionInput mediaInformationInputForSnapshotInput(
    const KiriView::DocumentSessionPublicSnapshotInput &input)
{
    if (input.mediaInformation.inputRevision != 0) {
        return input.mediaInformation;
    }

    KiriView::MediaInformationProjectionInput mediaInformationInput;
    mediaInformationInput.inputRevision = input.inputRevision;
    mediaInformationInput.documentKind = input.session.documentKind;
    mediaInformationInput.imageReady = input.image.readyForInformation;
    mediaInformationInput.imageDisplayedUrl = input.image.displayedUrl;
    mediaInformationInput.imageDisplayedOpenedCollectionScope
        = input.image.displayedOpenedCollectionScope;
    mediaInformationInput.imageSize = input.image.directMediaSize;
    mediaInformationInput.imageEmbeddedMetadata = input.image.embeddedMetadata;
    mediaInformationInput.videoSourceUrl = input.video.sourceUrl;
    mediaInformationInput.videoSize = input.video.directMediaSize;
    mediaInformationInput.videoEmbeddedMetadata = input.video.embeddedMetadata;
    return mediaInformationInput;
}
}

namespace KiriView {
DocumentSessionPublicProjection projectDocumentSessionPublicState(
    DocumentSessionPublicProjectionInput input)
{
    DocumentSessionPublicProjection projection;
    projection.sourceKind = sourceKindForInput(input);
    projection.boundaryScope = activeNavigationBoundaryScopeForSource(projection.sourceKind);
    projection.activeNavigation
        = projectActiveNavigation(projection.sourceKind, input.directMediaNavigation,
            input.imageDocumentPageNavigation, input.fileDeletionInProgress);
    projection.windowTitleSubject = projectWindowTitleSubject(WindowTitleSubjectInput {
        windowTitleFileNameForInput(input),
        projection.sourceKind,
        directMediaSizeForInput(input, projection.sourceKind),
        projection.activeNavigation,
    });
    projection.displayedMediaOpenWithAvailable = input.displayedMediaOpenWithAvailable;
    projection.displayedFileDeletionAvailable = displayedFileDeletionAvailableForInput(input);
    return projection;
}

DocumentSessionPublicSnapshot projectDocumentSessionPublicSnapshot(
    const DocumentSessionPublicSnapshotInput &input, quint64 revision)
{
    DocumentSessionPublicSnapshot snapshot;
    snapshot.revision = revision;
    snapshot.inputRevision = input.inputRevision;
    snapshot.sourceUrl = input.session.sourceUrl;
    snapshot.documentKind = input.session.documentKind;
    snapshot.errorString = errorStringForInput(input);
    snapshot.fileDeletionInProgress = input.session.fileDeletionInProgress;
    snapshot.activeZoom = activeZoomForInput(input);
    snapshot.activeImageReady = activeImageReadyForInput(input);
    snapshot.activeImageUnsupportedOpenedCollectionVideo
        = input.session.documentKind == DocumentSessionKind::Image
        && input.image.unsupportedOpenedCollectionVideo;
    snapshot.activeImageOpenedCollectionScopeActive
        = activeImageOpenedCollectionScopeActiveForInput(input);
    snapshot.activeImageRightToLeftReadingActive
        = activeImageRightToLeftReadingActiveForInput(input);
    snapshot.activeVideoReady = activeVideoReadyForInput(input);
    snapshot.activeVideoControlsReady = activeVideoControlsReadyForInput(input);
    snapshot.actionAvailability = actionAvailabilityFactsForInput(input);
    snapshot.projection = projectDocumentSessionPublicState(projectionInputForSnapshotInput(input));
    snapshot.mediaInformation
        = projectMediaInformation(mediaInformationInputForSnapshotInput(input), revision);
    snapshot.activeNavigationRevealIntent = input.session.activeNavigationRevealIntent;
    snapshot.activeNavigationRevealDirection = input.session.activeNavigationRevealDirection;
    return snapshot;
}
}

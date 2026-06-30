// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionpublicprojection.h"

#include "session/documentsessionactivezoom.h"
#include "session/windowtitleprojection.h"

namespace {
bool imageDocumentPagesArePresent(kiriview::ImageDocumentPageActiveNavigationSnapshot input)
{
    return input.currentNumber > 0 || input.count > 0;
}

kiriview::ActiveNavigationSourceKind sourceKindForInput(
    const kiriview::DocumentSessionPublicProjectionInput& input)
{
    switch (input.documentKind) {
    case kiriview::DocumentSessionKind::Video:
        if (input.openedCollectionVideoActive
            && imageDocumentPagesArePresent(input.imageDocumentPageNavigation)) {
            return kiriview::ActiveNavigationSourceKind::ImageDocumentPages;
        }
        return kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia;
    case kiriview::DocumentSessionKind::Image:
        if (input.directImageLoadMayUseImageDocumentSourceScope) {
            return kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia;
        }
        if (imageDocumentPagesArePresent(input.imageDocumentPageNavigation)
            || input.imageSourceMayRepresentDocument) {
            return kiriview::ActiveNavigationSourceKind::ImageDocumentPages;
        }
        return kiriview::ActiveNavigationSourceKind::None;
    case kiriview::DocumentSessionKind::Empty:
        return kiriview::ActiveNavigationSourceKind::None;
    }

    return kiriview::ActiveNavigationSourceKind::None;
}

QSize directMediaSizeForInput(const kiriview::DocumentSessionPublicProjectionInput& input,
    kiriview::ActiveNavigationSourceKind sourceKind)
{
    if (sourceKind != kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia) {
        return {};
    }

    switch (input.documentKind) {
    case kiriview::DocumentSessionKind::Image:
        return input.imageDirectMediaSize;
    case kiriview::DocumentSessionKind::Video:
        return input.videoDirectMediaSize;
    case kiriview::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

QString windowTitleFileNameForInput(const kiriview::DocumentSessionPublicProjectionInput& input)
{
    switch (input.documentKind) {
    case kiriview::DocumentSessionKind::Image:
        return input.imageWindowTitleFileName;
    case kiriview::DocumentSessionKind::Video:
        return input.videoWindowTitleFileName;
    case kiriview::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

bool displayedFileDeletionAvailableForInput(
    const kiriview::DocumentSessionPublicProjectionInput& input)
{
    if (input.fileDeletionInProgress) {
        return false;
    }

    switch (input.documentKind) {
    case kiriview::DocumentSessionKind::Image:
        if (input.directImageLoadMayUseImageDocumentSourceScope
            && input.directImageReplacementPending) {
            return false;
        }
        return input.imageReadyForDeletion;
    case kiriview::DocumentSessionKind::Video:
        return input.videoSourcePresent && !input.videoError;
    case kiriview::DocumentSessionKind::Empty:
        return false;
    }

    return false;
}

bool activeImageReadyForInput(const kiriview::DocumentSessionPublicSnapshotInput& input)
{
    return input.session.documentKind == kiriview::DocumentSessionKind::Image
        && input.image.readyForInformation && !input.image.unsupportedOpenedCollectionVideo;
}

bool activeImageOpenedCollectionScopeActiveForInput(
    const kiriview::DocumentSessionPublicSnapshotInput& input)
{
    return input.session.documentKind == kiriview::DocumentSessionKind::Image
        && input.image.openedCollectionScopeActive;
}

bool activeImageRightToLeftReadingActiveForInput(
    const kiriview::DocumentSessionPublicSnapshotInput& input)
{
    return input.session.documentKind == kiriview::DocumentSessionKind::Image
        && input.image.rightToLeftReadingEnabled && input.image.rightToLeftReadingAvailable;
}

bool activeVideoReadyForInput(const kiriview::DocumentSessionPublicSnapshotInput& input)
{
    return input.session.documentKind == kiriview::DocumentSessionKind::Video && input.video.ready;
}

bool activeVideoControlsReadyForInput(const kiriview::DocumentSessionPublicSnapshotInput& input)
{
    return activeVideoReadyForInput(input) && input.video.hasVideo;
}

kiriview::DocumentSessionActionAvailabilityFacts actionAvailabilityFactsForInput(
    const kiriview::DocumentSessionPublicSnapshotInput& input)
{
    if (input.session.documentKind != kiriview::DocumentSessionKind::Image) {
        return {};
    }

    return kiriview::DocumentSessionActionAvailabilityFacts {
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

QString errorStringForInput(const kiriview::DocumentSessionPublicSnapshotInput& input)
{
    if (!input.session.sessionErrorString.isEmpty()) {
        return input.session.sessionErrorString;
    }

    switch (input.session.documentKind) {
    case kiriview::DocumentSessionKind::Image:
        return input.image.errorString;
    case kiriview::DocumentSessionKind::Video:
        return input.video.errorString;
    case kiriview::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

kiriview::DocumentSessionPublicProjectionInput projectionInputForSnapshotInput(
    const kiriview::DocumentSessionPublicSnapshotInput& input)
{
    return kiriview::DocumentSessionPublicProjectionInput {
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
        input.session.openedCollectionVideoActive,
    };
}

kiriview::MediaInformationProjectionInput mediaInformationInputForSnapshotInput(
    const kiriview::DocumentSessionPublicSnapshotInput& input)
{
    if (input.mediaInformation.inputRevision != 0) {
        return input.mediaInformation;
    }

    kiriview::MediaInformationProjectionInput mediaInformationInput;
    mediaInformationInput.inputRevision = input.inputRevision;
    mediaInformationInput.documentKind = input.session.documentKind;
    mediaInformationInput.imageReady = input.image.readyForInformation;
    mediaInformationInput.imageUnsupportedOpenedCollectionVideo
        = input.image.unsupportedOpenedCollectionVideo;
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

namespace kiriview {
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
    const DocumentSessionPublicSnapshotInput& input, quint64 revision)
{
    DocumentSessionPublicSnapshot snapshot;
    snapshot.revision = revision;
    snapshot.inputRevision = input.inputRevision;
    snapshot.sourceUrl = input.session.sourceUrl;
    snapshot.documentKind = input.session.documentKind;
    snapshot.errorString = errorStringForInput(input);
    snapshot.fileDeletionInProgress = input.session.fileDeletionInProgress;
    snapshot.activeZoom
        = documentSessionActiveZoomSnapshot(input.session.documentKind, input.image, input.video);
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

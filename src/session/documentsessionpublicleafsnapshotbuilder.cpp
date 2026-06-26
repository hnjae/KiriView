// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionpublicleafsnapshotbuilder.h"

#include "navigation/mediaformatregistry.h"

namespace kiriview {
DocumentSessionPublicImageLeafSnapshot buildDocumentSessionPublicImageLeafSnapshot(
    const DocumentSessionImageDocumentSnapshot& leafSnapshot)
{
    DocumentSessionPublicImageLeafSnapshot snapshot;
    const QUrl sourceUrl = leafSnapshot.sourceUrl;
    snapshot.sourceUrl = leafSnapshot.sourceUrl;
    snapshot.sourceMayRepresentDocument
        = !sourceUrl.isEmpty() && !isSupportedDirectImageUrl(sourceUrl);
    snapshot.pageNavigation = leafSnapshot.activeNavigationSnapshot;
    snapshot.pageNavigationRows = leafSnapshot.pageNavigationSnapshot;
    snapshot.displayedUrl = leafSnapshot.displayedUrl;
    snapshot.displayedOpenedCollectionScope = leafSnapshot.displayedOpenedCollectionScope;
    snapshot.windowTitleFileName = leafSnapshot.windowTitleFileName;
    snapshot.directMediaSize = leafSnapshot.primaryImageSize;
    snapshot.embeddedMetadata = leafSnapshot.embeddedMetadata;
    snapshot.readyForDeletion = leafSnapshot.ready;
    snapshot.readyForInformation = leafSnapshot.ready;
    snapshot.error = leafSnapshot.error;
    snapshot.fileDeletionInProgress = leafSnapshot.fileDeletionInProgress;
    snapshot.openedCollectionScopeActive = leafSnapshot.openedCollectionScopeActive;
    snapshot.ordinaryDirectMediaScopeActive = leafSnapshot.ordinaryDirectMediaScopeActive;
    snapshot.unsupportedOpenedCollectionVideo = leafSnapshot.unsupportedOpenedCollectionVideo;
    snapshot.containerNavigationAvailable = leafSnapshot.containerNavigationAvailable;
    snapshot.twoPageModeEnabled = leafSnapshot.twoPageModeEnabled;
    snapshot.twoPageModeAvailable = leafSnapshot.twoPageModeAvailable;
    snapshot.rightToLeftReadingEnabled = leafSnapshot.rightToLeftReadingEnabled;
    snapshot.rightToLeftReadingAvailable = leafSnapshot.rightToLeftReadingAvailable;
    snapshot.fitModeSelected = leafSnapshot.fitModeSelected;
    snapshot.fitHeightModeSelected = leafSnapshot.fitHeightModeSelected;
    snapshot.fitWidthModeSelected = leafSnapshot.fitWidthModeSelected;
    snapshot.zoomPercentKnown = leafSnapshot.zoomPercentKnown;
    snapshot.zoomPercent = leafSnapshot.zoomPercent;
    snapshot.errorString = leafSnapshot.errorString;
    snapshot.primaryDisplayedPredecodeImage = leafSnapshot.primaryDisplayedPredecodeImage;
    snapshot.firstDisplayDecodeContext = leafSnapshot.firstDisplayDecodeContext;
    return snapshot;
}

DocumentSessionPublicVideoLeafSnapshot buildDocumentSessionPublicVideoLeafSnapshot(
    const DocumentSessionVideoDocumentSnapshot& leafSnapshot)
{
    DocumentSessionPublicVideoLeafSnapshot snapshot;
    snapshot.sourceUrl = leafSnapshot.sourceUrl;
    snapshot.windowTitleFileName = leafSnapshot.windowTitleFileName;
    snapshot.directMediaSize = leafSnapshot.videoSize;
    snapshot.embeddedMetadata = leafSnapshot.embeddedMetadata;
    snapshot.ready = leafSnapshot.ready;
    snapshot.hasVideo = leafSnapshot.hasVideo;
    snapshot.sourcePresent = !snapshot.sourceUrl.isEmpty();
    snapshot.error = leafSnapshot.error;
    snapshot.zoomPercentKnown = leafSnapshot.zoomPercentKnown;
    snapshot.zoomPercent = leafSnapshot.zoomPercent;
    snapshot.errorString = leafSnapshot.errorString;
    return snapshot;
}
}

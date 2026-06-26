// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionpublicsnapshotinputbuilder.h"

#include "session/mediaopenwithplan.h"

namespace kiriview {
DocumentSessionPublicSnapshotInput buildDocumentSessionPublicSnapshotInput(
    const DocumentSessionPublicSnapshotInputBuilderInput& builderInput)
{
    DocumentSessionPublicSnapshotInput input;
    input.inputRevision = builderInput.inputRevision;
    input.session = builderInput.session;
    input.image = builderInput.image;
    input.image.directImageReplacementPending
        = !builderInput.directMediaCursor.pendingUrl.isEmpty();
    input.video = builderInput.video;
    input.operations.displayedMediaOpenWithAvailable
        = mediaOpenWithPlan(MediaOpenWithPlanInput {
                                builderInput.session.documentKind,
                                builderInput.image.readyForInformation,
                                builderInput.image.displayedUrl,
                                builderInput.image.displayedOpenedCollectionScope,
                                builderInput.video.ready,
                                builderInput.video.sourceUrl,
                            })
              .hasRequest();
    return input;
}
}

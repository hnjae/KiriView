// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONPUBLICSNAPSHOTINPUTBUILDER_H
#define KIRIVIEW_DOCUMENTSESSIONPUBLICSNAPSHOTINPUTBUILDER_H

#include "session/directmediacursor.h"
#include "session/documentsessionpublicprojection.h"

namespace kiriview {
struct DocumentSessionPublicSnapshotInputBuilderInput {
    quint64 inputRevision = 0;
    DocumentSessionPublicSessionLeafSnapshot session;
    DocumentSessionPublicImageLeafSnapshot image;
    DocumentSessionPublicVideoLeafSnapshot video;
    DirectMediaCursor directMediaCursor;
};

DocumentSessionPublicSnapshotInput buildDocumentSessionPublicSnapshotInput(
    const DocumentSessionPublicSnapshotInputBuilderInput &input);
}

#endif

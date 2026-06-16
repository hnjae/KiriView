// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONPUBLICLEAFSNAPSHOTBUILDER_H
#define KIRIVIEW_DOCUMENTSESSIONPUBLICLEAFSNAPSHOTBUILDER_H

#include "session/documentsessiondocumentports.h"
#include "session/documentsessionpublicprojection.h"

namespace kiriview {
DocumentSessionPublicImageLeafSnapshot buildDocumentSessionPublicImageLeafSnapshot(
    const DocumentSessionImageDocumentSnapshot &leafSnapshot);

DocumentSessionPublicVideoLeafSnapshot buildDocumentSessionPublicVideoLeafSnapshot(
    const DocumentSessionVideoDocumentSnapshot &leafSnapshot);
}

#endif

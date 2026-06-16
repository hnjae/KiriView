// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONACTIVEZOOM_H
#define KIRIVIEW_DOCUMENTSESSIONACTIVEZOOM_H

#include "session/documentsessiontypes.h"

namespace kiriview {
struct DocumentSessionPublicImageLeafSnapshot;
struct DocumentSessionPublicVideoLeafSnapshot;

ActiveZoomSnapshot documentSessionActiveZoomSnapshot(DocumentSessionKind documentKind,
    const DocumentSessionPublicImageLeafSnapshot &image,
    const DocumentSessionPublicVideoLeafSnapshot &video);
}

#endif

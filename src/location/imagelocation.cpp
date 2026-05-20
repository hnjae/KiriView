// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "location/imagelocation.h"

#include "location/imageurl.h"

namespace KiriView {
bool sameArchiveDocumentLocation(
    const ArchiveDocumentLocation &left, const ArchiveDocumentLocation &right)
{
    return sameNormalizedUrl(left.fileUrl(), right.fileUrl())
        && sameNormalizedUrl(left.rootUrl(), right.rootUrl()) && left.kind() == right.kind();
}
}

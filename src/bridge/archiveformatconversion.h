// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEFORMATCONVERSION_H
#define KIRIVIEW_ARCHIVEFORMATCONVERSION_H

#include "archive/archiveformat.h"
#include "kiriview/src/policy/archiveformat.cxx.h"

#include <optional>

namespace kiriview {
ArchiveStorageBackend archiveStorageBackendFromBridge(RustArchiveStorageBackend backend);
std::optional<ArchiveOpenMatch> archiveOpenMatchFromBridge(const RustArchiveOpenMatch& match);
}

#endif

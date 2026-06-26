// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformatconversion.h"

#include "bridge/rustqtconversion.h"

namespace {
kiriview::ArchiveOpenMatchKind archiveOpenMatchKindFromBridge(
    kiriview::RustArchiveOpenMatchKind kind)
{
    switch (kind) {
    case kiriview::RustArchiveOpenMatchKind::ComicBook:
        return kiriview::ArchiveOpenMatchKind::ComicBook;
    case kiriview::RustArchiveOpenMatchKind::GeneralArchive:
        return kiriview::ArchiveOpenMatchKind::GeneralArchive;
    }

    return kiriview::ArchiveOpenMatchKind::GeneralArchive;
}
}

namespace kiriview {
ArchiveStorageBackend archiveStorageBackendFromBridge(RustArchiveStorageBackend backend)
{
    switch (backend) {
    case RustArchiveStorageBackend::KZip:
        return ArchiveStorageBackend::KZip;
    case RustArchiveStorageBackend::KTar:
        return ArchiveStorageBackend::KTar;
    case RustArchiveStorageBackend::K7Zip:
        return ArchiveStorageBackend::K7Zip;
    case RustArchiveStorageBackend::LibArchive:
        return ArchiveStorageBackend::LibArchive;
    case RustArchiveStorageBackend::None:
        return ArchiveStorageBackend::None;
    }

    return ArchiveStorageBackend::None;
}

std::optional<ArchiveOpenMatch> archiveOpenMatchFromBridge(const RustArchiveOpenMatch& match)
{
    if (!match.found) {
        return std::nullopt;
    }

    return ArchiveOpenMatch {
        Bridge::qtString(match.scheme),
        archiveOpenMatchKindFromBridge(match.kind),
    };
}
}

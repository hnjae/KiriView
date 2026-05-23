// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archiveformatconversion.h"

#include "bridge/rustqtconversion.h"

namespace {
KiriView::ArchiveOpenMatchKind archiveOpenMatchKindFromBridge(
    KiriView::RustArchiveOpenMatchKind kind)
{
    switch (kind) {
    case KiriView::RustArchiveOpenMatchKind::ComicBook:
        return KiriView::ArchiveOpenMatchKind::ComicBook;
    case KiriView::RustArchiveOpenMatchKind::GeneralArchive:
        return KiriView::ArchiveOpenMatchKind::GeneralArchive;
    }

    return KiriView::ArchiveOpenMatchKind::GeneralArchive;
}
}

namespace KiriView {
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

std::optional<ArchiveOpenMatch> archiveOpenMatchFromBridge(const RustArchiveOpenMatch &match)
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

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "metadata/embeddedmetadata.h"

#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/embeddedmetadata.cxx.h"

#include <utility>

namespace {
kiriview::EmbeddedMetadataRow embeddedMetadataRowFromRust(
    const kiriview::RustEmbeddedMetadataRow &row)
{
    return kiriview::EmbeddedMetadataRow {
        kiriview::Bridge::qtString(row.label),
        kiriview::Bridge::qtString(row.value),
    };
}

kiriview::EmbeddedMetadata embeddedMetadataFromRust(const kiriview::RustEmbeddedMetadata &metadata)
{
    kiriview::EmbeddedMetadata converted;
    converted.cameraMake = kiriview::Bridge::qtString(metadata.camera_make);
    converted.cameraModel = kiriview::Bridge::qtString(metadata.camera_model);
    converted.taken = kiriview::Bridge::qtString(metadata.taken);
    converted.location = kiriview::Bridge::qtString(metadata.location);
    converted.lens = kiriview::Bridge::qtString(metadata.lens);
    converted.exposure = kiriview::Bridge::qtString(metadata.exposure);
    converted.iso = kiriview::Bridge::qtString(metadata.iso);
    converted.focalLength = kiriview::Bridge::qtString(metadata.focal_length);
    converted.software = kiriview::Bridge::qtString(metadata.software);
    converted.duration = kiriview::Bridge::qtString(metadata.duration);
    converted.frameSize = kiriview::Bridge::qtString(metadata.frame_size);
    converted.advancedRows.reserve(metadata.advanced_rows.size());
    for (const kiriview::RustEmbeddedMetadataRow &row : metadata.advanced_rows) {
        converted.advancedRows.push_back(embeddedMetadataRowFromRust(row));
    }
    return converted;
}
}

namespace kiriview {
bool EmbeddedMetadata::isEmpty() const
{
    return cameraMake.isEmpty() && cameraModel.isEmpty() && taken.isEmpty() && location.isEmpty()
        && lens.isEmpty() && exposure.isEmpty() && iso.isEmpty() && focalLength.isEmpty()
        && software.isEmpty() && duration.isEmpty() && frameSize.isEmpty() && advancedRows.empty();
}

EmbeddedMetadata parseImageEmbeddedMetadata(const QByteArray &data)
{
    return embeddedMetadataFromRust(rustParseImageEmbeddedMetadata(Bridge::rustBytes(data)));
}

EmbeddedMetadata parsePathEmbeddedMetadata(const QString &path)
{
    const QByteArray bytes = path.toUtf8();
    return embeddedMetadataFromRust(rustParsePathEmbeddedMetadata(Bridge::rustStr(bytes)));
}
}

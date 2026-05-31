// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "metadata/embeddedmetadata.h"

#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/embeddedmetadata.cxx.h"

#include <utility>

namespace {
KiriView::EmbeddedMetadataRow embeddedMetadataRowFromRust(
    const KiriView::RustEmbeddedMetadataRow &row)
{
    return KiriView::EmbeddedMetadataRow {
        KiriView::Bridge::qtString(row.label),
        KiriView::Bridge::qtString(row.value),
    };
}

KiriView::EmbeddedMetadata embeddedMetadataFromRust(const KiriView::RustEmbeddedMetadata &metadata)
{
    KiriView::EmbeddedMetadata converted;
    converted.cameraMake = KiriView::Bridge::qtString(metadata.camera_make);
    converted.cameraModel = KiriView::Bridge::qtString(metadata.camera_model);
    converted.taken = KiriView::Bridge::qtString(metadata.taken);
    converted.location = KiriView::Bridge::qtString(metadata.location);
    converted.lens = KiriView::Bridge::qtString(metadata.lens);
    converted.exposure = KiriView::Bridge::qtString(metadata.exposure);
    converted.iso = KiriView::Bridge::qtString(metadata.iso);
    converted.focalLength = KiriView::Bridge::qtString(metadata.focal_length);
    converted.software = KiriView::Bridge::qtString(metadata.software);
    converted.duration = KiriView::Bridge::qtString(metadata.duration);
    converted.frameSize = KiriView::Bridge::qtString(metadata.frame_size);
    converted.advancedRows.reserve(metadata.advanced_rows.size());
    for (const KiriView::RustEmbeddedMetadataRow &row : metadata.advanced_rows) {
        converted.advancedRows.push_back(embeddedMetadataRowFromRust(row));
    }
    return converted;
}
}

namespace KiriView {
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

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heifcontainer.h"

#include "kiriview/src/heifcontainer.cxx.h"

#include <cstdint>

namespace {
rust::Slice<const std::uint8_t> rustBrandBytes(std::string_view brand)
{
    return rust::Slice<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t *>(brand.data()), brand.size());
}

KiriView::HeifBrandKind heifBrandKindFromRust(KiriView::RustHeifBrandKind brandKind)
{
    switch (brandKind) {
    case KiriView::RustHeifBrandKind::StillImage:
        return KiriView::HeifBrandKind::StillImage;
    case KiriView::RustHeifBrandKind::ImageSequence:
        return KiriView::HeifBrandKind::ImageSequence;
    case KiriView::RustHeifBrandKind::Unknown:
        return KiriView::HeifBrandKind::Unknown;
    }
    return KiriView::HeifBrandKind::Unknown;
}
}

namespace KiriView {
HeifBrandKind heifBrandKind(std::string_view brand)
{
    return heifBrandKindFromRust(rustHeifBrandKind(rustBrandBytes(brand)));
}

bool isLikelyHeifContainer(const QByteArray &data) { return heifContainerInfo(data).isHeif(); }

bool isLikelyHeifStillImageContainer(const QByteArray &data)
{
    return heifContainerInfo(data).stillImage;
}

bool isLikelyHeifSequenceContainer(const QByteArray &data)
{
    return heifContainerInfo(data).imageSequence;
}

HeifContainerInfo heifContainerInfo(const QByteArray &data)
{
    const RustHeifContainerInfo rustInfo = rustHeifContainerInfo(data);
    return HeifContainerInfo {
        rustInfo.still_image,
        rustInfo.image_sequence,
    };
}
}

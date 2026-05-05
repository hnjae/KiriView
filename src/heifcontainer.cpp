// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heifcontainer.h"

#include <QtGlobal>
#include <algorithm>
#include <array>
#include <cstring>

namespace {
quint32 readBigEndianUint32(const char *data)
{
    return (static_cast<quint32>(static_cast<unsigned char>(data[0])) << 24)
        | (static_cast<quint32>(static_cast<unsigned char>(data[1])) << 16)
        | (static_cast<quint32>(static_cast<unsigned char>(data[2])) << 8)
        | static_cast<quint32>(static_cast<unsigned char>(data[3]));
}

void recordBrand(KiriView::HeifContainerInfo &info, std::string_view brand)
{
    switch (KiriView::heifBrandKind(brand)) {
    case KiriView::HeifBrandKind::StillImage:
        info.stillImage = true;
        break;
    case KiriView::HeifBrandKind::ImageSequence:
        info.imageSequence = true;
        break;
    case KiriView::HeifBrandKind::Unknown:
        break;
    }
}

KiriView::HeifContainerInfo scanHeifBrands(const QByteArray &data)
{
    if (data.size() < 16 || std::memcmp(data.constData() + 4, "ftyp", 4) != 0) {
        return {};
    }

    const quint32 boxSize = readBigEndianUint32(data.constData());
    if (boxSize < 16 || boxSize > static_cast<quint32>(data.size())) {
        return {};
    }

    KiriView::HeifContainerInfo info;
    recordBrand(info, std::string_view(data.constData() + 8, 4));
    for (quint32 offset = 16; offset + 4 <= boxSize; offset += 4) {
        recordBrand(info, std::string_view(data.constData() + offset, 4));
    }
    return info;
}
}

namespace KiriView {
HeifBrandKind heifBrandKind(std::string_view brand)
{
    if (brand.size() != 4) {
        return HeifBrandKind::Unknown;
    }

    static constexpr std::array<std::string_view, 13> stillImageBrands {
        "avci",
        "avif",
        "heic",
        "heif",
        "heim",
        "heis",
        "heix",
        "hej2",
        "j2ki",
        "jpeg",
        "mif1",
        "mif2",
        "vvic",
    };
    static constexpr std::array<std::string_view, 10> sequenceBrands {
        "avcs",
        "avis",
        "hevc",
        "hevm",
        "hevs",
        "hevx",
        "j2is",
        "jpgs",
        "msf1",
        "vvis",
    };

    if (std::find(stillImageBrands.cbegin(), stillImageBrands.cend(), brand)
        != stillImageBrands.cend()) {
        return HeifBrandKind::StillImage;
    }
    if (std::find(sequenceBrands.cbegin(), sequenceBrands.cend(), brand) != sequenceBrands.cend()) {
        return HeifBrandKind::ImageSequence;
    }
    return HeifBrandKind::Unknown;
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

HeifContainerInfo heifContainerInfo(const QByteArray &data) { return scanHeifBrands(data); }
}

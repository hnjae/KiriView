// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_HEIFCONTAINER_H
#define KIRIVIEW_HEIFCONTAINER_H

#include <QByteArray>
#include <string_view>

namespace KiriView {
enum class HeifBrandKind {
    Unknown,
    StillImage,
    ImageSequence,
};

HeifBrandKind heifBrandKind(std::string_view brand);
bool isLikelyHeifContainer(const QByteArray &data);
bool isLikelyHeifStillImageContainer(const QByteArray &data);
bool isLikelyHeifSequenceContainer(const QByteArray &data);
}

#endif

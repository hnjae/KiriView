// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloader.h"

#include "imageasyncdependencies.h"

namespace KiriView {
// clang-format off
ImageLoader::ImageLoader(QObject *parent)
    : ImageLoader(parent, defaultImageAsyncDependencies().candidateProvider,
        defaultImageAsyncDependencies().imageDataLoader,
        defaultImageAsyncDependencies().imageDataDecoder)
{
}
// clang-format on
}

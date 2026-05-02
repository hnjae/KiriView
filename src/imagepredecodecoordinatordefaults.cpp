// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "imageasyncdependencies.h"

namespace KiriView {
// clang-format off
ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent)
    : ImagePredecodeCoordinator(parent, defaultImageAsyncDependencies().candidateProvider,
        defaultImageAsyncDependencies().imageDataLoader,
        defaultImageAsyncDependencies().imageDataDecoder)
{
}
// clang-format on
}

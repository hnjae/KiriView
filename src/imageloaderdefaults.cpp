// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloader.h"

namespace KiriView {
ImageLoader::ImageLoader(QObject *parent)
    : ImageLoader(parent, ImageNavigationCandidateProvider {}, ImageDecodeDependencies {})
{
}
}

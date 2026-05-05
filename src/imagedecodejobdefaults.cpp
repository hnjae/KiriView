// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejob.h"

#include "imageasyncdependencies.h"

#include <utility>

namespace KiriView {
ImageDecodeJob::ImageDecodeJob(QObject *parent)
    : QObject(parent)
{
    ImageAsyncDependencies dependencies = defaultImageAsyncDependencies();
    m_dataLoader = std::move(dependencies.imageDataLoader);
    m_dataDecoder = std::move(dependencies.imageDataDecoder);
}
}

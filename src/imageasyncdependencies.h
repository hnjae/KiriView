// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCDEPENDENCIES_H
#define KIRIVIEW_IMAGEASYNCDEPENDENCIES_H

#include "imagecandidaterepository.h"
#include "imagedecodejob.h"

namespace KiriView {
struct ImageAsyncDependencies {
    ImageNavigationCandidateProvider candidateProvider;
    ImageDecodeJob::DataLoader imageDataLoader;
    ImageDecodeJob::DataDecoder imageDataDecoder;
};

ImageAsyncDependencies defaultImageAsyncDependencies();
}

#endif

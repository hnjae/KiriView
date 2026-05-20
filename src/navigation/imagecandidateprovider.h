// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATEPROVIDER_H
#define KIRIVIEW_IMAGECANDIDATEPROVIDER_H

#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "imagecandidatecallbacks.h"
#include "location/imagelocation.h"

#include <QUrl>
#include <functional>

class QObject;

namespace KiriView {
struct ImageNavigationCandidateProvider {
    using ImageCandidateLoader
        = std::function<ImageIoJob(QObject *, QUrl, ImageCandidatesCallback, ErrorCallback)>;
    using ArchiveImageCandidateLoader = std::function<ImageIoJob(
        QObject *, ArchiveDocumentLocation, ImageCandidatesCallback, ErrorCallback)>;
    using ContainerCandidateLoader
        = std::function<ImageIoJob(QObject *, QUrl, ContainerCandidatesCallback, ErrorCallback)>;
    using ImageCandidateChangeSubscriber
        = std::function<ImageIoJob(QObject *, QUrl, ImageCandidatesCallback, ErrorCallback)>;

    ImageCandidateLoader directoryImages;
    ContainerCandidateLoader directoryContainers;
    ArchiveImageCandidateLoader archiveImages;
    ImageCandidateChangeSubscriber directoryImageChanges;
};

ImageNavigationCandidateProvider defaultImageNavigationCandidateProvider();
ImageNavigationCandidateProvider imageNavigationCandidateProviderWithDefaults(
    ImageNavigationCandidateProvider provider);
}

#endif

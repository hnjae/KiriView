// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCDEPENDENCIES_H
#define KIRIVIEW_IMAGEASYNCDEPENDENCIES_H

#include "archivebackend.h"
#include "decodedimageresult.h"
#include "filedeletion.h"
#include "imagedecoderequest.h"
#include "imageiojob.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <vector>

class QObject;

namespace KiriView {
using ImageCandidatesCallback = std::function<void(std::vector<ImageNavigationCandidate>)>;
using ContainerCandidatesCallback = std::function<void(std::vector<ContainerNavigationCandidate>)>;
using ImageDataCallback = std::function<void(QByteArray)>;
using ErrorCallback = std::function<void(const QString &)>;

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

using ImageDataLoader
    = std::function<ImageIoJob(QObject *, ImageDecodeRequest, ImageDataCallback, ErrorCallback)>;
using ImageDataDecoder
    = std::function<DecodedImageResult(const QByteArray &, const ImageDecodeRequest &)>;

struct ImageDecodeDependencies {
    ImageDataLoader dataLoader;
    ImageDataDecoder dataDecoder;
};

class PowerSaverStateMonitor
{
public:
    virtual ~PowerSaverStateMonitor() = default;
    virtual bool powerSaverEnabled() const = 0;
};

using PowerSaverChangedCallback = std::function<void(bool)>;
using PowerSaverMonitorFactory
    = std::function<std::unique_ptr<PowerSaverStateMonitor>(QObject *, PowerSaverChangedCallback)>;

struct PowerSaverProvider {
    PowerSaverMonitorFactory monitor;
};

struct ImageAsyncDependencies {
    ImageNavigationCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
    FileOperationProvider fileOperations;
    ArchiveDocumentSessionFactory archiveDocumentSessions;
    PowerSaverProvider powerSaver;
};

ImageNavigationCandidateProvider defaultImageNavigationCandidateProvider();
ImageNavigationCandidateProvider imageNavigationCandidateProviderWithDefaults(
    ImageNavigationCandidateProvider provider);
ImageDecodeDependencies defaultImageDecodeDependencies();
ImageDecodeDependencies imageDecodeDependenciesWithDefaults(ImageDecodeDependencies dependencies);
PowerSaverProvider defaultPowerSaverProvider();
PowerSaverProvider powerSaverProviderWithDefault(PowerSaverProvider provider);
FileOperationProvider fileOperationProviderWithDefault(FileOperationProvider provider);
ImageAsyncDependencies defaultImageAsyncDependencies();
ImageAsyncDependencies imageAsyncDependenciesWithDefaults(ImageAsyncDependencies dependencies);
}

#endif

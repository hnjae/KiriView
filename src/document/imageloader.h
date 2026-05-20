// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADER_H
#define KIRIVIEW_IMAGELOADER_H

#include "async/imageiojob.h"
#include "decoding/decodedimageresult.h"
#include "decoding/imagedecodedependencies.h"
#include "decoding/imagedecodejob.h"
#include "imageloadsessiontracker.h"
#include "imageloadtypes.h"
#include "navigation/imagecandidateprovider.h"
#include "navigation/imagecandidaterepository.h"
#include "predecode/predecodedimage.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <optional>

namespace KiriView {
class ImageLoader final : public QObject
{
public:
    using SourceResolvedCallback = std::function<void(const QUrl &)>;
    using ErrorCallback = std::function<void(ImageLoadSession, ImageLoadError, const QString &)>;
    using DecodedImageCallback = std::function<void(ImageLoadSession, DecodedImage)>;
    using PredecodedImageCallback = std::function<void(ImageLoadSession, PredecodedImage)>;
    using FindPredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl &)>;

    struct Callbacks {
        SourceResolvedCallback sourceResolved;
        ErrorCallback error;
        DecodedImageCallback decodedImage;
        PredecodedImageCallback predecodedImage;
        FindPredecodedImageCallback findPredecodedImage;
    };

    explicit ImageLoader(QObject *parent = nullptr);
    ImageLoader(QObject *parent, Callbacks callbacks);
    ImageLoader(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies);
    ImageLoader(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies, Callbacks callbacks);

    void start(ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext = {});
    void cancel();

private:
    void finishDecodeResult(ImageDecodeRequest request, DecodedImageResult result);
    void finishImageLoadError(const ImageDecodeRequest &request, const QString &errorString);
    void startImageLoad(ImageLoadSession session);
    void startArchiveLoad(ImageLoadSession session);
    bool tryDisplayPredecodedImage(ImageLoadSession session);
    void finishDecodeRequestWithError(
        const ImageDecodeRequest &request, ImageLoadError error, const QString &errorString);
    void finishDecodedImage(ImageLoadSession session, DecodedImage image);
    void finishPredecodedImage(ImageLoadSession session, PredecodedImage image);

    Callbacks m_callbacks;
    ImageDecodeJob m_decodeJob;
    ImageCandidateRepository m_candidateRepository;
    ImageIoJob m_archiveListJob;
    ImageLoadSessionTracker m_sessionTracker;
};
}

#endif

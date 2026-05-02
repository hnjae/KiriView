// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADER_H
#define KIRIVIEW_IMAGELOADER_H

#include "imagecandidaterepository.h"
#include "imagedecodejob.h"
#include "imageiojob.h"
#include "imagelocation.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

namespace KiriView {
struct ImageLoadSession {
    quint64 id = 0;
    ImageLoadRequest request;
    DisplayedImageLocation location;
};

enum class ImageLoadError {
    Generic,
    EmptyComicBookArchive,
};

class ImageLoader final : public QObject
{
public:
    using SourceResolvedCallback = std::function<void(const QUrl &)>;
    using ErrorCallback
        = std::function<void(const ImageLoadSession &, ImageLoadError, const QString &)>;
    using DecodedImageCallback
        = std::function<void(ImageLoadSession, std::shared_ptr<DecodedImageResult>)>;
    using PredecodedImageCallback = std::function<void(ImageLoadSession, const QImage &)>;
    using TakePredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl &)>;
    using DataLoader = ImageDecodeJob::DataLoader;
    using DataDecoder = ImageDecodeJob::DataDecoder;

    explicit ImageLoader(QObject *parent = nullptr);
    ImageLoader(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
        DataLoader dataLoader, DataDecoder dataDecoder);

    void setSourceResolvedCallback(SourceResolvedCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setDecodedImageCallback(DecodedImageCallback callback);
    void setPredecodedImageCallback(PredecodedImageCallback callback);
    void setTakePredecodedImageCallback(TakePredecodedImageCallback callback);

    void start(ImageLoadRequest request);
    void cancel();

private:
    void startImageLoad(ImageLoadSession session);
    void startComicBookLoad(ImageLoadSession session);
    std::optional<ImageLoadSession> currentLoadSessionForDecodeRequest(
        const ImageDecodeRequest &request) const;
    bool isCurrentLoadSession(const ImageLoadSession &session) const;
    void clearLoadSession(const ImageLoadSession &session);
    bool tryDisplayPredecodedImage(ImageLoadSession session);
    void finishLoadWithError(
        const ImageLoadSession &session, ImageLoadError error, const QString &errorString);
    void finishDecodedImage(ImageLoadSession session, std::shared_ptr<DecodedImageResult> result);
    void finishPredecodedImage(ImageLoadSession session, const QImage &image);

    SourceResolvedCallback m_sourceResolved;
    ErrorCallback m_error;
    DecodedImageCallback m_decodedImage;
    PredecodedImageCallback m_predecodedImage;
    TakePredecodedImageCallback m_takePredecodedImage;
    ImageDecodeJob m_decodeJob;
    ImageCandidateRepository m_candidateRepository;
    ImageIoJob m_archiveListJob;
    quint64 m_nextLoadSessionId = 0;
    std::optional<ImageLoadSession> m_loadSession;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADER_H
#define KIRIVIEW_IMAGELOADER_H

#include "imageasyncdependencies.h"
#include "imageasyncticket.h"
#include "imagedecodejob.h"
#include "imageiojob.h"
#include "imageloadtypes.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

namespace KiriView {
class ImageLoader final : public QObject
{
public:
    using SourceResolvedCallback = std::function<void(const QUrl &)>;
    using ErrorCallback = std::function<void(ImageLoadSession, ImageLoadError, const QString &)>;
    using DecodedImageCallback
        = std::function<void(ImageLoadSession, std::shared_ptr<DecodedImage>)>;
    using PredecodedImageCallback = std::function<void(ImageLoadSession, PredecodedImage)>;
    using TakePredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl &)>;

    struct Callbacks {
        SourceResolvedCallback sourceResolved;
        ErrorCallback error;
        DecodedImageCallback decodedImage;
        PredecodedImageCallback predecodedImage;
        TakePredecodedImageCallback takePredecodedImage;
    };

    explicit ImageLoader(QObject *parent = nullptr);
    ImageLoader(QObject *parent, Callbacks callbacks);
    ImageLoader(QObject *parent, const ImageAsyncDependencies &dependencies);
    ImageLoader(QObject *parent, const ImageAsyncDependencies &dependencies, Callbacks callbacks);

    void start(ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext = {});
    void cancel();

private:
    void startImageLoad(ImageLoadSession session);
    void startArchiveLoad(ImageLoadSession session);
    std::optional<ImageLoadSession> currentLoadSessionForDecodeRequest(
        const ImageDecodeRequest &request) const;
    bool isCurrentLoadSession(const ImageLoadSession &session) const;
    std::optional<ImageLoadSession> takeCurrentLoadSession(const ImageLoadSession &session);
    bool tryDisplayPredecodedImage(ImageLoadSession session);
    void finishLoadWithError(
        const ImageLoadSession &session, ImageLoadError error, const QString &errorString);
    void finishDecodedImage(ImageLoadSession session, std::shared_ptr<DecodedImage> image);
    void finishPredecodedImage(ImageLoadSession session, PredecodedImage image);
    template <typename Callback, typename... Args>
    void finishCurrentLoadSession(
        const ImageLoadSession &session, Callback &callback, Args &&...args);

    Callbacks m_callbacks;
    ImageAsyncTicket m_loadTickets;
    ImageDecodeJob m_decodeJob;
    ImageCandidateRepository m_candidateRepository;
    ImageIoJob m_archiveListJob;
    std::optional<ImageLoadSession> m_loadSession;
    ImageFirstDisplayDecodeContext m_firstDisplayContext;
};
}

#endif

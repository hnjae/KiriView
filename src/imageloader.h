// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADER_H
#define KIRIVIEW_IMAGELOADER_H

#include "asyncobjectslot.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

namespace KiriView {
struct DecodedImageResult;

struct ImageLoadSession {
    quint64 id = 0;
    QUrl requestedSourceUrl;
    QUrl imageUrl;
    QUrl comicBookRootUrl;
    QUrl containerNavigationUrl;
};

enum class ImageLoadError {
    Generic,
    EmptyComicBookArchive,
};

struct PredecodedImage {
    QImage image;
    QUrl comicBookRootUrl;
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

    explicit ImageLoader(QObject *parent = nullptr);

    void setSourceResolvedCallback(SourceResolvedCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setDecodedImageCallback(DecodedImageCallback callback);
    void setPredecodedImageCallback(PredecodedImageCallback callback);
    void setTakePredecodedImageCallback(TakePredecodedImageCallback callback);

    void start(const QUrl &sourceUrl, const QUrl &displayedComicBookRootUrl,
        const QUrl &containerNavigationUrl);
    void cancel();

private:
    void startImageLoad(ImageLoadSession session);
    void startImageDecode(QByteArray data, ImageLoadSession session);
    void startComicBookLoad(ImageLoadSession session);
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
    AsyncObjectSlot m_imageLoadSlot;
    AsyncObjectSlot m_archiveListSlot;
    quint64 m_nextLoadSessionId = 0;
    std::optional<ImageLoadSession> m_loadSession;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEJOB_H
#define KIRIVIEW_IMAGEDECODEJOB_H

#include "imageiojob.h"
#include "kiriimagedecoder.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

namespace KiriView {
struct ImageDecodeRequest {
    quint64 id = 0;
    QUrl imageUrl;
};

class ImageDecodeJob final : public QObject
{
public:
    using DataCallback = std::function<void(QByteArray)>;
    using ErrorCallback = std::function<void(const QString &)>;
    using DataLoader = std::function<ImageIoJob(QObject *, QUrl, DataCallback, ErrorCallback)>;
    using DataDecoder = std::function<DecodedImageResult(const QByteArray &)>;
    using DecodedCallback
        = std::function<void(ImageDecodeRequest, std::shared_ptr<DecodedImageResult>)>;
    using LoadErrorCallback = std::function<void(const ImageDecodeRequest &, const QString &)>;

    ImageDecodeJob(QObject *parent, DataLoader dataLoader, DataDecoder dataDecoder);

    void setDecodedCallback(DecodedCallback callback);
    void setLoadErrorCallback(LoadErrorCallback callback);

    void start(ImageDecodeRequest request);
    void cancel();
    bool hasActiveRequest() const;

private:
    void startDecode(QByteArray data, ImageDecodeRequest request);
    bool isCurrentRequest(const ImageDecodeRequest &request) const;
    void clearRequest(const ImageDecodeRequest &request);

    DataLoader m_dataLoader;
    DataDecoder m_dataDecoder;
    DecodedCallback m_decoded;
    LoadErrorCallback m_loadError;
    ImageIoJob m_dataLoadJob;
    std::optional<ImageDecodeRequest> m_request;
};
}

#endif

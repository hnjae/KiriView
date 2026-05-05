// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEJOB_H
#define KIRIVIEW_IMAGEDECODEJOB_H

#include "decodedimageresult.h"
#include "imagedecoderequest.h"
#include "imageiojob.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <functional>
#include <optional>

namespace KiriView {
class ImageDecodeJob final : public QObject
{
public:
    using DataCallback = std::function<void(QByteArray)>;
    using ErrorCallback = std::function<void(const QString &)>;
    using DataLoader
        = std::function<ImageIoJob(QObject *, ImageDecodeRequest, DataCallback, ErrorCallback)>;
    using DataDecoder
        = std::function<DecodedImageResult(const QByteArray &, const ImageDecodeRequest &)>;
    using DecodedCallback = std::function<void(ImageDecodeRequest, DecodedImageResult)>;
    using LoadErrorCallback = std::function<void(const ImageDecodeRequest &, const QString &)>;

    explicit ImageDecodeJob(QObject *parent = nullptr);
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

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEJOB_H
#define KIRIVIEW_IMAGEDECODEJOB_H

#include "decodedimageresult.h"
#include "imageasyncdependencies.h"
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
    using DecodedCallback = std::function<void(ImageDecodeRequest, DecodedImageResult)>;
    using LoadErrorCallback = std::function<void(const ImageDecodeRequest &, const QString &)>;

    explicit ImageDecodeJob(QObject *parent = nullptr);
    ImageDecodeJob(QObject *parent, ImageDataLoader dataLoader, ImageDataDecoder dataDecoder);

    void setDecodedCallback(DecodedCallback callback);
    void setLoadErrorCallback(LoadErrorCallback callback);

    void start(ImageDecodeRequest request);
    void cancel();
    bool hasActiveRequest() const;

private:
    void startDecode(QByteArray data, ImageDecodeRequest request);
    bool isCurrentRequest(const ImageDecodeRequest &request) const;
    void clearRequest(const ImageDecodeRequest &request);

    ImageDataLoader m_dataLoader;
    ImageDataDecoder m_dataDecoder;
    DecodedCallback m_decoded;
    LoadErrorCallback m_loadError;
    ImageIoJob m_dataLoadJob;
    std::optional<ImageDecodeRequest> m_request;
};
}

#endif

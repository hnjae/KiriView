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

    struct Callbacks {
        DecodedCallback decoded;
        LoadErrorCallback loadError;
    };

    explicit ImageDecodeJob(QObject *parent = nullptr);
    ImageDecodeJob(QObject *parent, Callbacks callbacks);
    ImageDecodeJob(QObject *parent, ImageDecodeDependencies dependencies);
    ImageDecodeJob(QObject *parent, ImageDecodeDependencies dependencies, Callbacks callbacks);

    void start(ImageDecodeRequest request);
    void cancel();
    bool hasActiveRequest() const;

private:
    void startDecode(QByteArray data, ImageDecodeRequest request);
    bool isCurrentRequest(const ImageDecodeRequest &request) const;
    std::optional<ImageDecodeRequest> takeCurrentRequest(const ImageDecodeRequest &request);

    ImageDataLoader m_dataLoader;
    ImageDataDecoder m_dataDecoder;
    Callbacks m_callbacks;
    ImageIoJob m_dataLoadJob;
    std::optional<ImageDecodeRequest> m_request;
};
}

#endif

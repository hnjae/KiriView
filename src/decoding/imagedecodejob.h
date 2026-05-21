// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEJOB_H
#define KIRIVIEW_IMAGEDECODEJOB_H

#include "async/imageiojob.h"
#include "decodedimageresult.h"
#include "imagedecodedependencies.h"
#include "imagedecodejobstate.h"
#include "imagedecoderequest.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <functional>

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
    void startDecode(QByteArray data, ImageDecodeJobTicket ticket, ImageDecodeRequest request);

    ImageDecodeDependencies m_dependencies;
    Callbacks m_callbacks;
    ImageIoJob m_dataLoadJob;
    ImageDecodeJobState m_state;
};
}

#endif

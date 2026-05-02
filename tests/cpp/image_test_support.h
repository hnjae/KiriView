// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_TESTS_IMAGE_TEST_SUPPORT_H
#define KIRIVIEW_TESTS_IMAGE_TEST_SUPPORT_H

#include "imagecandidaterepository.h"
#include "imagedecodejob.h"
#include "imageiojob.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>
#include <QUrl>
#include <memory>
#include <utility>
#include <vector>

namespace KiriView::TestSupport {
struct ManualImageDataLoad {
    QObject *object = nullptr;
    QUrl url;
    ImageDecodeJob::DataCallback dataCallback;
    ImageDecodeJob::ErrorCallback errorCallback;
    bool canceled = false;
};

class ManualImageDataLoader
{
public:
    ImageIoJob start(QObject *receiver, QUrl url, ImageDecodeJob::DataCallback callback,
        ImageDecodeJob::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualImageDataLoad>();
        load->object = new QObject(receiver);
        load->url = std::move(url);
        load->dataCallback = std::move(callback);
        load->errorCallback = std::move(errorCallback);
        loads.push_back(load);

        return ImageIoJob(load->object, [load](QObject *object) {
            load->canceled = true;
            if (object != nullptr) {
                object->deleteLater();
            }
        });
    }

    std::vector<std::shared_ptr<ManualImageDataLoad>> loads;
};

inline ImageDecodeJob::DataLoader dataLoaderFor(ManualImageDataLoader &dataLoader)
{
    using DataCb = ImageDecodeJob::DataCallback;
    using ErrorCb = ImageDecodeJob::ErrorCallback;

    return [&dataLoader](QObject *receiver, QUrl url, DataCb callback, ErrorCb errorCallback) {
        return dataLoader.start(
            receiver, std::move(url), std::move(callback), std::move(errorCallback));
    };
}

inline QString keyForUrl(const QUrl &url)
{
    return url.adjusted(QUrl::NormalizePathSegments).toString();
}

inline QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

inline ImageNavigationCandidate imageCandidate(const QUrl &url)
{
    return ImageNavigationCandidate { url, url.fileName() };
}

inline QImage testImage(const QSize &size = QSize(1, 1))
{
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

inline QImage testImage(int width, int height = 1) { return testImage(QSize(width, height)); }

inline DecodedImageResult decodeStaticTestImageData(const QByteArray &)
{
    return StaticDecodedImage { testImage() };
}
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_TESTS_IMAGE_TEST_SUPPORT_H
#define KIRIVIEW_TESTS_IMAGE_TEST_SUPPORT_H

#include "imagecandidaterepository.h"
#include "imagedecodejob.h"
#include "imageiojob.h"
#include "imagesurface.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView::TestSupport {
struct ManualImageDataLoad {
    QObject *object = nullptr;
    QUrl url;
    ArchiveDocumentLocation archiveDocument;
    ImageDecodeJob::DataCallback dataCallback;
    ImageDecodeJob::ErrorCallback errorCallback;
    bool canceled = false;
};

class ManualImageDataLoader
{
public:
    ImageIoJob start(QObject *receiver, ImageDecodeRequest request,
        ImageDecodeJob::DataCallback callback, ImageDecodeJob::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualImageDataLoad>();
        load->object = new QObject(receiver);
        load->url = std::move(request.imageUrl);
        load->archiveDocument = std::move(request.archiveDocument);
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

class ManualImageDataLoaderAdapter
{
public:
    explicit ManualImageDataLoaderAdapter(ManualImageDataLoader &dataLoader)
        : m_dataLoader(&dataLoader)
    {
    }

    ImageIoJob operator()(QObject *receiver, ImageDecodeRequest request,
        ImageDecodeJob::DataCallback callback, ImageDecodeJob::ErrorCallback errorCallback) const
    {
        return m_dataLoader->start(
            receiver, std::move(request), std::move(callback), std::move(errorCallback));
    }

private:
    ManualImageDataLoader *m_dataLoader = nullptr;
};

inline ImageDecodeJob::DataLoader dataLoaderFor(ManualImageDataLoader &dataLoader)
{
    return ManualImageDataLoaderAdapter(dataLoader);
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

class TestImageTileSource final : public ImageTileSource
{
public:
    explicit TestImageTileSource(QImage image)
        : m_image(std::move(image))
        , m_pyramid(m_image.size())
    {
    }

    QSize imageSize() const override { return m_image.size(); }
    int levelCount() const override { return m_pyramid.levelCount(); }
    QSize levelSize(int level) const override { return m_pyramid.levelSize(level); }
    qsizetype byteCost() const override { return m_image.sizeInBytes(); }

    QImage decodePreview(int, QString *) const override { return m_image; }

    std::optional<DecodedTile> decodeTile(const TileRequest &request, QString *) const override
    {
        if (request.textureLevelRect.isEmpty()) {
            return std::nullopt;
        }

        QImage levelImage
            = m_image.scaled(request.levelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        return DecodedTile { request.key, request.levelSize, request.levelRect,
            request.textureLevelRect, levelImage.copy(request.textureLevelRect) };
    }

private:
    QImage m_image;
    TilePyramid m_pyramid;
};

inline StaticDecodedImage staticDecodedTestImage(const QImage &image = testImage())
{
    return StaticDecodedImage { std::make_shared<TestImageTileSource>(image), image };
}

inline DecodedImageResult decodeStaticTestImageData(const QByteArray &)
{
    return staticDecodedTestImage();
}
}

#endif

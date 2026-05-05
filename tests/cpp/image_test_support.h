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
#include <QtGlobal>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace KiriView::TestSupport {
struct ManualImageDataLoad {
    QObject *object = nullptr;
    QUrl url;
    ArchiveDocumentLocation archiveDocument;
    ImageFirstDisplayDecodeContext firstDisplay;
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
        load->firstDisplay = request.firstDisplay;
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

inline QUrl archivePageUrl(const QUrl &archiveRootUrl, const QString &pageName)
{
    QUrl pageUrl = archiveRootUrl;
    pageUrl.setPath(archiveRootUrl.path() + pageName);
    return pageUrl;
}

namespace Detail {
    inline void appendFourCc(QByteArray &data, std::string_view fourCc)
    {
        Q_ASSERT(fourCc.size() == 4);

        if (fourCc.size() >= 4) {
            data.append(fourCc.data(), 4);
            return;
        }

        data.append(fourCc.data(), static_cast<qsizetype>(fourCc.size()));
        data.append(4 - static_cast<qsizetype>(fourCc.size()), '\0');
    }
}

inline QByteArray heifFtypBox(
    std::string_view majorBrand, std::initializer_list<std::string_view> compatibleBrands)
{
    const quint32 boxSize = 16 + static_cast<quint32>(compatibleBrands.size() * 4);
    QByteArray data;
    data.append(static_cast<char>((boxSize >> 24) & 0xff));
    data.append(static_cast<char>((boxSize >> 16) & 0xff));
    data.append(static_cast<char>((boxSize >> 8) & 0xff));
    data.append(static_cast<char>(boxSize & 0xff));
    Detail::appendFourCc(data, "ftyp");
    Detail::appendFourCc(data, majorBrand);
    data.append(4, '\0');
    for (std::string_view brand : compatibleBrands) {
        Detail::appendFourCc(data, brand);
    }
    return data;
}

inline ImageNavigationCandidate imageCandidate(const QUrl &url)
{
    return ImageNavigationCandidate { url, url.fileName() };
}

inline ContainerNavigationCandidate containerCandidate(
    const QUrl &url, ContainerNavigationCandidateType type)
{
    return ContainerNavigationCandidate { url, url.fileName(), type };
}

inline ContainerNavigationCandidate comicBookContainerCandidate(const QUrl &url)
{
    return containerCandidate(url, ContainerNavigationCandidateType::ComicBookArchive);
}

class FakeImageNavigationCandidateProvider
{
public:
    void setDirectoryImages(
        const QUrl &directoryUrl, std::vector<ImageNavigationCandidate> candidates)
    {
        m_directoryImagesByUrl[keyForUrl(directoryUrl)] = std::move(candidates);
    }

    void setArchiveImages(
        const QUrl &archiveRootUrl, std::vector<ImageNavigationCandidate> candidates)
    {
        m_archiveImagesByUrl[keyForUrl(archiveRootUrl)] = std::move(candidates);
    }

    void setContainerCandidates(
        const QUrl &directoryUrl, std::vector<ContainerNavigationCandidate> candidates)
    {
        m_containerCandidatesByUrl[keyForUrl(directoryUrl)] = std::move(candidates);
    }

    void setDirectoryImageError(const QUrl &directoryUrl, QString errorString)
    {
        m_directoryImageErrorsByUrl[keyForUrl(directoryUrl)] = std::move(errorString);
    }

    void setArchiveImageError(const QUrl &archiveRootUrl, QString errorString)
    {
        m_archiveImageErrorsByUrl[keyForUrl(archiveRootUrl)] = std::move(errorString);
    }

    void setContainerError(const QUrl &directoryUrl, QString errorString)
    {
        m_containerErrorsByUrl[keyForUrl(directoryUrl)] = std::move(errorString);
    }

    ImageNavigationCandidateProvider provider()
    {
        return ImageNavigationCandidateProvider {
            [this](QObject *, QUrl directoryUrl, ImageCandidatesCallback callback,
                ErrorCallback errorCallback) {
                loadImages(m_directoryImagesByUrl, m_directoryImageErrorsByUrl,
                    std::move(directoryUrl), std::move(callback), std::move(errorCallback));
                return ImageIoJob();
            },
            [this](QObject *, QUrl directoryUrl, ContainerCandidatesCallback callback,
                ErrorCallback errorCallback) {
                loadContainers(
                    std::move(directoryUrl), std::move(callback), std::move(errorCallback));
                return ImageIoJob();
            },
            [this](QObject *, ArchiveDocumentLocation archiveDocument,
                ImageCandidatesCallback callback, ErrorCallback errorCallback) {
                loadImages(m_archiveImagesByUrl, m_archiveImageErrorsByUrl,
                    archiveDocument.rootUrl(), std::move(callback), std::move(errorCallback));
                return ImageIoJob();
            },
        };
    }

private:
    void loadImages(std::map<QString, std::vector<ImageNavigationCandidate>> &imagesByUrl,
        const std::map<QString, QString> &errorsByUrl, QUrl url, ImageCandidatesCallback callback,
        ErrorCallback errorCallback)
    {
        const QString key = keyForUrl(url);
        const auto error = errorsByUrl.find(key);
        if (error != errorsByUrl.cend()) {
            if (errorCallback) {
                errorCallback(error->second);
            }
            return;
        }

        if (callback) {
            callback(imagesByUrl[key]);
        }
    }

    void loadContainers(
        QUrl directoryUrl, ContainerCandidatesCallback callback, ErrorCallback errorCallback)
    {
        const QString key = keyForUrl(directoryUrl);
        const auto error = m_containerErrorsByUrl.find(key);
        if (error != m_containerErrorsByUrl.cend()) {
            if (errorCallback) {
                errorCallback(error->second);
            }
            return;
        }

        if (callback) {
            callback(m_containerCandidatesByUrl[key]);
        }
    }

    std::map<QString, std::vector<ImageNavigationCandidate>> m_directoryImagesByUrl;
    std::map<QString, std::vector<ImageNavigationCandidate>> m_archiveImagesByUrl;
    std::map<QString, std::vector<ContainerNavigationCandidate>> m_containerCandidatesByUrl;
    std::map<QString, QString> m_directoryImageErrorsByUrl;
    std::map<QString, QString> m_archiveImageErrorsByUrl;
    std::map<QString, QString> m_containerErrorsByUrl;
};

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
    {
    }

    QSize imageSize() const override { return m_image.size(); }
    qsizetype byteCost() const override { return m_image.sizeInBytes(); }

    FirstDisplayImageDecodeResult decodeFirstDisplayImage(
        const ImageFirstDisplayDecodeContext &, QString *) const override
    {
        return {};
    }

    QImage decodeBlockingDisplayImage(int, QString *) const override { return m_image; }

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
};

inline StaticDecodedImage staticDecodedTestImage(const QImage &image = testImage())
{
    return StaticDecodedImage { std::make_shared<TestImageTileSource>(image), image, {} };
}

inline DecodedImageResult decodeStaticTestImageData(const QByteArray &, const ImageDecodeRequest &)
{
    return staticDecodedTestImage();
}
}

#endif

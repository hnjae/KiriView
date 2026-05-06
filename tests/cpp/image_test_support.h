// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_TESTS_IMAGE_TEST_SUPPORT_H
#define KIRIVIEW_TESTS_IMAGE_TEST_SUPPORT_H

#include "imageasyncdependencies.h"
#include "imageiojob.h"
#include "staticimage.h"

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
    ImageDataCallback dataCallback;
    ErrorCallback errorCallback;
    bool canceled = false;
};

class ManualImageDataLoader
{
public:
    ImageIoJob start(QObject *receiver, ImageDecodeRequest request, ImageDataCallback callback,
        ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualImageDataLoad>();
        load->object = new QObject(receiver);
        load->url = request.imageUrl();
        load->archiveDocument = request.archiveDocument();
        load->firstDisplay = request.firstDisplay();
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

    ImageIoJob operator()(QObject *receiver, ImageDecodeRequest request, ImageDataCallback callback,
        ErrorCallback errorCallback) const
    {
        return m_dataLoader->start(
            receiver, std::move(request), std::move(callback), std::move(errorCallback));
    }

private:
    ManualImageDataLoader *m_dataLoader = nullptr;
};

inline ImageDataLoader dataLoaderFor(ManualImageDataLoader &dataLoader)
{
    return ManualImageDataLoaderAdapter(dataLoader);
}

inline QString keyForUrl(const QUrl &url)
{
    return url.adjusted(QUrl::NormalizePathSegments).toString();
}

inline QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

inline QString indexedImageFileName(int index)
{
    return QStringLiteral("%1.png").arg(index, 2, 10, QLatin1Char('0'));
}

inline QUrl indexedImageUrl(int index)
{
    return localUrl(QStringLiteral("/images/") + indexedImageFileName(index));
}

inline QUrl imagesDirectoryUrl() { return localUrl(QStringLiteral("/images/")); }

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

template <typename Candidates> class FakeCandidateListing
{
public:
    void setItems(const QUrl &url, Candidates candidates)
    {
        m_itemsByUrl[keyForUrl(url)] = std::move(candidates);
    }

    void setError(const QUrl &url, QString errorString)
    {
        m_errorsByUrl[keyForUrl(url)] = std::move(errorString);
    }

    template <typename Callback>
    void load(QUrl url, Callback callback, ErrorCallback errorCallback) const
    {
        const QString key = keyForUrl(url);
        const auto error = m_errorsByUrl.find(key);
        if (error != m_errorsByUrl.cend()) {
            if (errorCallback) {
                errorCallback(error->second);
            }
            return;
        }

        const auto items = m_itemsByUrl.find(key);
        if (items == m_itemsByUrl.cend()) {
            if (errorCallback) {
                errorCallback(
                    QStringLiteral("missing fake candidate listing for %1").arg(url.toString()));
            }
            return;
        }

        if (callback) {
            callback(items->second);
        }
    }

private:
    std::map<QString, Candidates> m_itemsByUrl;
    std::map<QString, QString> m_errorsByUrl;
};

class FakeImageNavigationCandidateProvider
{
public:
    void setDirectoryImages(
        const QUrl &directoryUrl, std::vector<ImageNavigationCandidate> candidates)
    {
        m_directoryImages.setItems(directoryUrl, std::move(candidates));
    }

    void setArchiveImages(
        const QUrl &archiveRootUrl, std::vector<ImageNavigationCandidate> candidates)
    {
        m_archiveImages.setItems(archiveRootUrl, std::move(candidates));
    }

    void setContainerCandidates(
        const QUrl &directoryUrl, std::vector<ContainerNavigationCandidate> candidates)
    {
        m_containerCandidates.setItems(directoryUrl, std::move(candidates));
    }

    void setDirectoryImageError(const QUrl &directoryUrl, QString errorString)
    {
        m_directoryImages.setError(directoryUrl, std::move(errorString));
    }

    void setArchiveImageError(const QUrl &archiveRootUrl, QString errorString)
    {
        m_archiveImages.setError(archiveRootUrl, std::move(errorString));
    }

    void setContainerError(const QUrl &directoryUrl, QString errorString)
    {
        m_containerCandidates.setError(directoryUrl, std::move(errorString));
    }

    ImageNavigationCandidateProvider provider()
    {
        return ImageNavigationCandidateProvider {
            [this](QObject *, QUrl directoryUrl, ImageCandidatesCallback callback,
                ErrorCallback errorCallback) {
                m_directoryImages.load(
                    std::move(directoryUrl), std::move(callback), std::move(errorCallback));
                return ImageIoJob();
            },
            [this](QObject *, QUrl directoryUrl, ContainerCandidatesCallback callback,
                ErrorCallback errorCallback) {
                m_containerCandidates.load(
                    std::move(directoryUrl), std::move(callback), std::move(errorCallback));
                return ImageIoJob();
            },
            [this](QObject *, ArchiveDocumentLocation archiveDocument,
                ImageCandidatesCallback callback, ErrorCallback errorCallback) {
                m_archiveImages.load(
                    archiveDocument.rootUrl(), std::move(callback), std::move(errorCallback));
                return ImageIoJob();
            },
        };
    }

private:
    FakeCandidateListing<std::vector<ImageNavigationCandidate>> m_directoryImages;
    FakeCandidateListing<std::vector<ImageNavigationCandidate>> m_archiveImages;
    FakeCandidateListing<std::vector<ContainerNavigationCandidate>> m_containerCandidates;
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

inline StaticImagePayload staticTestImagePayload(
    const QImage &sourceImage, const QImage &preview, StaticImageDisplayHints displayHints = {})
{
    return StaticImagePayload {
        std::make_shared<TestImageTileSource>(sourceImage),
        preview,
        displayHints,
    };
}

inline StaticImagePayload staticTestImagePayload(
    const QImage &image = testImage(), StaticImageDisplayHints displayHints = {})
{
    return staticTestImagePayload(image, image, displayHints);
}

inline StaticDecodedImage staticDecodedTestImage(const QImage &image = testImage())
{
    return StaticDecodedImage { staticTestImagePayload(image) };
}

inline QString testImageDecodeFailureString() { return QStringLiteral("decode failed"); }

inline DecodedImageResult failedTestImageDecodeResult()
{
    return failedDecodedImageResult(testImageDecodeFailureString());
}

inline ImageDataDecoder staticImageDataDecoder(QImage image = testImage())
{
    return [image = std::move(image)](const QByteArray &, const ImageDecodeRequest &) {
        return successfulDecodedImageResult(staticDecodedTestImage(image));
    };
}

inline ImageDataDecoder staticImageDataDecoderRejectingBadData(QImage image = testImage())
{
    return [decoder = staticImageDataDecoder(std::move(image))](
               const QByteArray &data, const ImageDecodeRequest &request) {
        if (data == QByteArrayLiteral("bad")) {
            return failedTestImageDecodeResult();
        }

        return decoder(data, request);
    };
}

inline ImageDecodeDependencies imageDecodeDependenciesFor(
    ManualImageDataLoader &dataLoader, ImageDataDecoder dataDecoder)
{
    return ImageDecodeDependencies {
        dataLoaderFor(dataLoader),
        std::move(dataDecoder),
    };
}

inline ImageAsyncDependencies imageAsyncDependenciesFor(
    FakeImageNavigationCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader,
    ImageDataDecoder dataDecoder)
{
    return ImageAsyncDependencies {
        candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, std::move(dataDecoder)),
    };
}

}

#endif

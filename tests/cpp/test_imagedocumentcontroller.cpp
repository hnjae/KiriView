// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagerendering.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace {
using KiriView::TestSupport::dataLoaderFor;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::keyForUrl;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::testImage;

QUrl archivePageUrl(const QUrl &archiveRootUrl, const QString &pageName)
{
    QUrl pageUrl = archiveRootUrl;
    pageUrl.setPath(archiveRootUrl.path() + pageName);
    return pageUrl;
}

KiriView::ContainerNavigationCandidate comicBookContainerCandidate(const QUrl &url)
{
    return KiriView::ContainerNavigationCandidate {
        url,
        url.fileName(),
        KiriView::ContainerNavigationCandidateType::ComicBookArchive,
    };
}

KiriView::DecodedImageResult decodeTestImageData(const QByteArray &)
{
    return KiriView::StaticDecodedImage { testImage(2) };
}

class FakeCandidateProvider
{
public:
    KiriView::ImageNavigationCandidateProvider provider()
    {
        return KiriView::ImageNavigationCandidateProvider {
            [this](QObject *, QUrl directoryUrl, KiriView::ImageCandidatesCallback callback,
                KiriView::ErrorCallback) {
                if (callback) {
                    callback(directoryImagesByUrl[keyForUrl(directoryUrl)]);
                }
                return KiriView::ImageIoJob();
            },
            [this](QObject *, QUrl directoryUrl, KiriView::ContainerCandidatesCallback callback,
                KiriView::ErrorCallback) {
                if (callback) {
                    callback(containerCandidatesByUrl[keyForUrl(directoryUrl)]);
                }
                return KiriView::ImageIoJob();
            },
            [this](QObject *, KiriView::ArchiveDocumentLocation archiveDocument,
                KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback) {
                if (callback) {
                    callback(archiveImagesByUrl[keyForUrl(archiveDocument.rootUrl())]);
                }
                return KiriView::ImageIoJob();
            },
        };
    }

    std::map<QString, std::vector<KiriView::ImageNavigationCandidate>> directoryImagesByUrl;
    std::map<QString, std::vector<KiriView::ImageNavigationCandidate>> archiveImagesByUrl;
    std::map<QString, std::vector<KiriView::ContainerNavigationCandidate>> containerCandidatesByUrl;
};

KiriView::ImageAsyncDependencies dependenciesFor(
    FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    return KiriView::ImageAsyncDependencies {
        candidateProvider.provider(),
        dataLoaderFor(dataLoader),
        decodeTestImageData,
    };
}

std::unique_ptr<KiriView::ImageDocumentController> createController(
    QObject *parent, FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    return std::make_unique<KiriView::ImageDocumentController>(
        parent,
        []() {
            return KiriView::ImageDocumentRenderContext {
                1.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        KiriView::ImageDocumentController::ChangeCallback {},
        dependenciesFor(candidateProvider, dataLoader));
}

void finishLoad(ManualImageDataLoader &dataLoader)
{
    dataLoader.loads.back()->dataCallback(QByteArrayLiteral("ok"));
}
}

class TestImageDocumentController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialLoadSuccessUpdatesDocumentState();
    void replacementLoadFailureKeepsDisplayedImage();
    void emptyContainerNavigationClearsImageAndSelectsContainer();
};

void TestImageDocumentController::initialLoadSuccessUpdatesDocumentState()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.directoryImagesByUrl[keyForUrl(localUrl(QStringLiteral("/images/")))] = {
        imageCandidate(imageUrl),
    };

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);

    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Loading);
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->sourceUrl(), imageUrl);
    QCOMPARE(controller->displayedUrl(), imageUrl);
    QCOMPARE(controller->imageSize(), QSize(2, 1));
    QCOMPARE(controller->currentPageNumber(), 1);
    QCOMPARE(controller->imageCount(), 1);
    QVERIFY(!controller->containerNavigationAvailable());
    QVERIFY(!controller->image().isNull());
}

void TestImageDocumentController::replacementLoadFailureKeepsDisplayedImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl missingUrl = localUrl(QStringLiteral("/images/missing.png"));
    candidateProvider.directoryImagesByUrl[keyForUrl(localUrl(QStringLiteral("/images/")))] = {
        imageCandidate(imageUrl),
        imageCandidate(missingUrl),
    };

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    const quint64 displayedRevision = controller->imageRevision();
    const std::size_t loadCountBeforeReplacement = dataLoader.loads.size();

    controller->setSourceUrl(missingUrl);
    QCOMPARE(dataLoader.loads.size(), loadCountBeforeReplacement + 1);
    QCOMPARE(dataLoader.loads.back()->url, missingUrl);
    dataLoader.loads.back()->errorCallback(QStringLiteral("missing"));

    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->sourceUrl(), imageUrl);
    QCOMPARE(controller->displayedUrl(), imageUrl);
    QCOMPARE(controller->errorString(), QStringLiteral("missing"));
    QCOMPARE(controller->imageSize(), QSize(2, 1));
    QCOMPARE(controller->imageRevision(), displayedRevision);
    QVERIFY(!controller->image().isNull());
}

void TestImageDocumentController::emptyContainerNavigationClearsImageAndSelectsContainer()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> currentArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(currentContainerUrl);
    const std::optional<KiriView::ArchiveDocumentLocation> targetArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(currentArchiveDocument.has_value());
    QVERIFY(targetArchiveDocument.has_value());
    const QUrl currentImageUrl
        = archivePageUrl(currentArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.archiveImagesByUrl[keyForUrl(currentArchiveDocument->rootUrl())] = {
        imageCandidate(currentImageUrl),
    };
    candidateProvider.containerCandidatesByUrl[keyForUrl(localUrl(QStringLiteral("/books/")))] = {
        comicBookContainerCandidate(currentContainerUrl),
        comicBookContainerCandidate(targetContainerUrl),
    };
    candidateProvider.archiveImagesByUrl[keyForUrl(targetArchiveDocument->rootUrl())] = {};

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(currentContainerUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->openNextContainer();

    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Error);
    QCOMPARE(controller->sourceUrl(), targetContainerUrl);
    QCOMPARE(controller->displayedUrl(), QUrl());
    QVERIFY(controller->containerNavigationAvailable());
    QVERIFY(controller->image().isNull());
    QCOMPARE(controller->imageSize(), QSize());
    QVERIFY(!controller->errorString().isEmpty());
}

QTEST_GUILESS_MAIN(TestImageDocumentController)

#include "test_imagedocumentcontroller.moc"

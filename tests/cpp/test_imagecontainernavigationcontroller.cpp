// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imagecallback.h"
#include "candidate_test_support.h"
#include "image_async_test_support.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagecandidaterepository.h"
#include "navigation/imagecontainernavigationcontroller.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <functional>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace {
using KiriView::ContainerNavigationCandidate;
using KiriView::ContainerNavigationCandidateType;
using KiriView::ImageContainerOpenError;
using KiriView::ImageNavigationCandidate;
using KiriView::NavigationDirection;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::containerCandidate;
using KiriView::TestSupport::FakeImageNavigationCandidateProvider;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;

template <typename Candidates, typename Callback> struct ManualCandidateLoad {
    QObject *object = nullptr;
    QUrl url;
    Callback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

using ManualContainerList = ManualCandidateLoad<std::vector<ContainerNavigationCandidate>,
    KiriView::ContainerCandidatesCallback>;
using ManualImageList
    = ManualCandidateLoad<std::vector<ImageNavigationCandidate>, KiriView::ImageCandidatesCallback>;

class ManualContainerNavigationProvider
{
public:
    KiriView::ImageIoJob startContainerList(QObject *receiver, QUrl directoryUrl,
        KiriView::ContainerCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualContainerList>();
        load->url = std::move(directoryUrl);
        load->callback = std::move(callback);
        load->errorCallback = std::move(errorCallback);

        KiriView::ImageIoJob job = KiriView::TestSupport::Detail::startManualIoJob(receiver, load);
        m_containerLists.push_back(load);
        return job;
    }

    KiriView::ImageIoJob startImageList(QObject *receiver, QUrl directoryUrl,
        KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualImageList>();
        load->url = std::move(directoryUrl);
        load->callback = std::move(callback);
        load->errorCallback = std::move(errorCallback);

        KiriView::ImageIoJob job = KiriView::TestSupport::Detail::startManualIoJob(receiver, load);
        m_imageLists.push_back(load);
        return job;
    }

    std::size_t containerListCount() const { return m_containerLists.size(); }
    std::size_t imageListCount() const { return m_imageLists.size(); }

    ManualContainerList &containerListAt(std::size_t index) { return *m_containerLists.at(index); }
    ManualImageList &imageListAt(std::size_t index) { return *m_imageLists.at(index); }

    void finishContainerListAt(
        std::size_t index, std::vector<ContainerNavigationCandidate> candidates)
    {
        KiriView::TestSupport::Detail::finishManualIoJob(m_containerLists.at(index),
            [candidates = std::move(candidates)](ManualContainerList &load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    void deliverContainerListAtIgnoringCancellation(
        std::size_t index, std::vector<ContainerNavigationCandidate> candidates)
    {
        ManualContainerList &load = *m_containerLists.at(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

    void finishImageListAt(std::size_t index, std::vector<ImageNavigationCandidate> candidates)
    {
        KiriView::TestSupport::Detail::finishManualIoJob(m_imageLists.at(index),
            [candidates = std::move(candidates)](ManualImageList &load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    void deliverImageListAtIgnoringCancellation(
        std::size_t index, std::vector<ImageNavigationCandidate> candidates)
    {
        ManualImageList &load = *m_imageLists.at(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

    KiriView::ImageNavigationCandidateProvider provider()
    {
        return KiriView::ImageNavigationCandidateProvider {
            [this](QObject *receiver, QUrl directoryUrl, KiriView::ImageCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                return startImageList(receiver, std::move(directoryUrl), std::move(callback),
                    std::move(errorCallback));
            },
            [this](QObject *receiver, QUrl directoryUrl,
                KiriView::ContainerCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                return startContainerList(receiver, std::move(directoryUrl), std::move(callback),
                    std::move(errorCallback));
            },
            [](QObject *, KiriView::ImagePageScopeLocation, KiriView::ImageCandidatesCallback,
                KiriView::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected archive image listing"));
                }
                return KiriView::ImageIoJob();
            },
        };
    }

private:
    std::vector<std::shared_ptr<ManualContainerList>> m_containerLists;
    std::vector<std::shared_ptr<ManualImageList>> m_imageLists;
};

KiriView::ImageContainerNavigationController::Callbacks controllerCallbacks(
    std::function<void(const QUrl &, const QUrl &)> openContainerImage = {},
    std::function<void(const QUrl &, KiriView::ContainerNavigationError, const QString &)>
        containerNavigationError
    = {})
{
    return KiriView::ImageContainerNavigationController::Callbacks {
        [openContainerImage = std::move(openContainerImage),
            containerNavigationError = std::move(containerNavigationError)](
            KiriView::ImageNavigationPlan plan) mutable {
            for (const KiriView::ImageNavigationEffect &effect : plan) {
                if (const auto *openEffect
                    = std::get_if<KiriView::OpenContainerImageNavigationEffect>(&effect)) {
                    KiriView::invokeIfSet(
                        openContainerImage, openEffect->imageUrl, openEffect->containerUrl);
                } else if (const auto *errorEffect
                    = std::get_if<KiriView::ReportContainerNavigationErrorEffect>(&effect)) {
                    KiriView::invokeIfSet(containerNavigationError, errorEffect->containerUrl,
                        errorEffect->error, errorEffect->errorString);
                }
            }
        },
    };
}
}

class TestImageContainerNavigationController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void opensFirstImageFromAdjacentContainer();
    void opensFirstImageFromAdjacentArchiveContainer();
    void reportsEmptyAdjacentContainer();
    void reportsInvalidAdjacentArchiveContainer();
    void forwardsAdjacentContainerImageListingError();
    void cancelRejectsPendingContainerListing();
    void newRequestCancelsPendingFirstImageLoad();
    void canceledContainerListingCompletionIsIgnored();
    void canceledFirstImageCompletionIsIgnored();
};

void TestImageContainerNavigationController::opensFirstImageFromAdjacentContainer()
{
    ManualContainerNavigationProvider provider;
    KiriView::ImageCandidateRepository repository(provider.provider());
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    const QUrl targetImageUrl = localUrl(QStringLiteral("/books/b/01.png"));

    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    KiriView::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks(
            [&openedImageUrl, &openedContainerUrl](const QUrl &imageUrl, const QUrl &containerUrl) {
                openedImageUrl = imageUrl;
                openedContainerUrl = containerUrl;
            }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);
    QCOMPARE(provider.containerListCount(), std::size_t(1));
    provider.finishContainerListAt(0,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::Directory),
        });

    QCOMPARE(provider.imageListCount(), std::size_t(1));
    provider.finishImageListAt(0, { imageCandidate(targetImageUrl) });

    QCOMPARE(openedImageUrl, targetImageUrl);
    QCOMPARE(openedContainerUrl, targetContainerUrl);
}

void TestImageContainerNavigationController::opensFirstImageFromAdjacentArchiveContainer()
{
    FakeImageNavigationCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl targetImageUrl
        = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(
                targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        });
    fakeProvider.setArchiveImages(archiveDocument->rootUrl(), { imageCandidate(targetImageUrl) });

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    KiriView::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks(
            [&openedImageUrl, &openedContainerUrl](const QUrl &imageUrl, const QUrl &containerUrl) {
                openedImageUrl = imageUrl;
                openedContainerUrl = containerUrl;
            }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(openedImageUrl, targetImageUrl);
    QCOMPARE(openedContainerUrl, targetContainerUrl);
}

void TestImageContainerNavigationController::reportsEmptyAdjacentContainer()
{
    FakeImageNavigationCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::Directory),
        });
    fakeProvider.setDirectoryImages(targetContainerUrl, {});

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl errorContainerUrl;
    ImageContainerOpenError error = ImageContainerOpenError::Generic;
    KiriView::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks({},
            [&errorContainerUrl, &error](
                const QUrl &containerUrl, ImageContainerOpenError type, const QString &) {
                errorContainerUrl = containerUrl;
                error = type;
            }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(errorContainerUrl, targetContainerUrl);
    QCOMPARE(error, ImageContainerOpenError::EmptyContainer);
}

void TestImageContainerNavigationController::reportsInvalidAdjacentArchiveContainer()
{
    FakeImageNavigationCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/not-an-archive.png"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(
                targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        });

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    bool openedImage = false;
    QUrl errorContainerUrl;
    ImageContainerOpenError error = ImageContainerOpenError::Generic;
    KiriView::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks([&openedImage](const QUrl &, const QUrl &) { openedImage = true; },
            [&errorContainerUrl, &error](
                const QUrl &containerUrl, ImageContainerOpenError type, const QString &) {
                errorContainerUrl = containerUrl;
                error = type;
            }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QVERIFY(!openedImage);
    QCOMPARE(errorContainerUrl, targetContainerUrl);
    QCOMPARE(error, ImageContainerOpenError::InvalidComicBookArchive);
}

void TestImageContainerNavigationController::forwardsAdjacentContainerImageListingError()
{
    FakeImageNavigationCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::Directory),
        });
    fakeProvider.setDirectoryImageError(targetContainerUrl, QStringLiteral("No access"));

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl errorContainerUrl;
    ImageContainerOpenError error = ImageContainerOpenError::EmptyContainer;
    QString errorString;
    KiriView::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks({},
            [&errorContainerUrl, &error, &errorString](
                const QUrl &containerUrl, ImageContainerOpenError type, const QString &message) {
                errorContainerUrl = containerUrl;
                error = type;
                errorString = message;
            }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(errorContainerUrl, targetContainerUrl);
    QCOMPARE(error, ImageContainerOpenError::Generic);
    QCOMPARE(errorString, QStringLiteral("No access"));
}

void TestImageContainerNavigationController::cancelRejectsPendingContainerListing()
{
    ManualContainerNavigationProvider provider;
    KiriView::ImageCandidateRepository repository(provider.provider());
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));

    QUrl openedImageUrl;
    KiriView::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks(
            [&openedImageUrl](const QUrl &imageUrl, const QUrl &) { openedImageUrl = imageUrl; },
            {}));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);
    QCOMPARE(provider.containerListCount(), std::size_t(1));

    controller.cancel();
    QVERIFY(provider.containerListAt(0).canceled);
    provider.finishContainerListAt(0,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::Directory),
        });

    QVERIFY(openedImageUrl.isEmpty());
    QCOMPARE(provider.imageListCount(), std::size_t(0));
}

void TestImageContainerNavigationController::newRequestCancelsPendingFirstImageLoad()
{
    ManualContainerNavigationProvider provider;
    KiriView::ImageCandidateRepository repository(provider.provider());
    const QUrl firstCurrentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl firstTargetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    const QUrl firstTargetImageUrl = localUrl(QStringLiteral("/books/b/01.png"));
    const QUrl secondCurrentContainerUrl = localUrl(QStringLiteral("/books/c/"));
    const QUrl secondTargetContainerUrl = localUrl(QStringLiteral("/books/d/"));
    const QUrl secondTargetImageUrl = localUrl(QStringLiteral("/books/d/01.png"));

    std::vector<QUrl> openedImageUrls;
    KiriView::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks([&openedImageUrls](const QUrl &imageUrl,
                                const QUrl &) { openedImageUrls.push_back(imageUrl); },
            {}));

    controller.openAdjacentContainer(firstCurrentContainerUrl, NavigationDirection::Next);
    provider.finishContainerListAt(0,
        {
            containerCandidate(
                firstCurrentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(
                firstTargetContainerUrl, ContainerNavigationCandidateType::Directory),
        });
    QCOMPARE(provider.imageListCount(), std::size_t(1));

    controller.openAdjacentContainer(secondCurrentContainerUrl, NavigationDirection::Next);
    QVERIFY(provider.imageListAt(0).canceled);
    provider.deliverImageListAtIgnoringCancellation(0, { imageCandidate(firstTargetImageUrl) });

    QCOMPARE(provider.containerListCount(), std::size_t(2));
    provider.finishContainerListAt(1,
        {
            containerCandidate(
                secondCurrentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(
                secondTargetContainerUrl, ContainerNavigationCandidateType::Directory),
        });
    QCOMPARE(provider.imageListCount(), std::size_t(2));
    provider.finishImageListAt(1, { imageCandidate(secondTargetImageUrl) });

    QCOMPARE(openedImageUrls.size(), std::size_t(1));
    QCOMPARE(openedImageUrls.front(), secondTargetImageUrl);
}

void TestImageContainerNavigationController::canceledContainerListingCompletionIsIgnored()
{
    ManualContainerNavigationProvider provider;
    KiriView::ImageCandidateRepository repository(provider.provider());
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));

    QUrl openedImageUrl;
    KiriView::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks(
            [&openedImageUrl](const QUrl &imageUrl, const QUrl &) { openedImageUrl = imageUrl; },
            {}));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);
    QCOMPARE(provider.containerListCount(), std::size_t(1));

    controller.cancel();
    provider.deliverContainerListAtIgnoringCancellation(0,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::Directory),
        });

    QVERIFY(openedImageUrl.isEmpty());
    QCOMPARE(provider.imageListCount(), std::size_t(0));
}

void TestImageContainerNavigationController::canceledFirstImageCompletionIsIgnored()
{
    ManualContainerNavigationProvider provider;
    KiriView::ImageCandidateRepository repository(provider.provider());
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    const QUrl targetImageUrl = localUrl(QStringLiteral("/books/b/01.png"));

    QUrl openedImageUrl;
    KiriView::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks(
            [&openedImageUrl](const QUrl &imageUrl, const QUrl &) { openedImageUrl = imageUrl; },
            {}));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);
    provider.finishContainerListAt(0,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::Directory),
        });
    QCOMPARE(provider.imageListCount(), std::size_t(1));

    controller.cancel();
    provider.deliverImageListAtIgnoringCancellation(0, { imageCandidate(targetImageUrl) });

    QVERIFY(openedImageUrl.isEmpty());
}

QTEST_GUILESS_MAIN(TestImageContainerNavigationController)

#include "test_imagecontainernavigationcontroller.moc"

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imagecallback.h"
#include "candidate_test_support.h"
#include "image_async_test_support.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagecontainernavigationcontroller.h"
#include "navigation/imagedocumentpagecandidaterepository.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <functional>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace {
using kiriview::ContainerNavigationCandidate;
using kiriview::ContainerNavigationCandidateType;
using kiriview::ImageContainerOpenError;
using kiriview::ImageDocumentPageCandidate;
using kiriview::NavigationDirection;
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::containerCandidate;
using kiriview::TestSupport::FakeImageDocumentPageCandidateProvider;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::localUrl;

template <typename Candidates, typename Callback> struct ManualCandidateLoad {
    QObject *object = nullptr;
    QUrl url;
    Callback callback;
    kiriview::ErrorCallback errorCallback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

using ManualContainerList = ManualCandidateLoad<std::vector<ContainerNavigationCandidate>,
    kiriview::ContainerCandidatesCallback>;
using ManualImageList = ManualCandidateLoad<std::vector<ImageDocumentPageCandidate>,
    kiriview::ImageDocumentPageCandidatesCallback>;

class ManualContainerNavigationProvider
{
public:
    kiriview::ImageIoJob startContainerList(QObject *receiver, QUrl directoryUrl,
        kiriview::ContainerCandidatesCallback callback, kiriview::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualContainerList>();
        load->url = std::move(directoryUrl);
        load->callback = std::move(callback);
        load->errorCallback = std::move(errorCallback);

        kiriview::ImageIoJob job = kiriview::TestSupport::Detail::startManualIoJob(receiver, load);
        m_containerLists.push_back(load);
        return job;
    }

    kiriview::ImageIoJob startImageList(QObject *receiver, QUrl directoryUrl,
        kiriview::ImageDocumentPageCandidatesCallback callback,
        kiriview::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualImageList>();
        load->url = std::move(directoryUrl);
        load->callback = std::move(callback);
        load->errorCallback = std::move(errorCallback);

        kiriview::ImageIoJob job = kiriview::TestSupport::Detail::startManualIoJob(receiver, load);
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
        kiriview::TestSupport::Detail::finishManualIoJob(m_containerLists.at(index),
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

    void finishImageListAt(std::size_t index, std::vector<ImageDocumentPageCandidate> candidates)
    {
        kiriview::TestSupport::Detail::finishManualIoJob(m_imageLists.at(index),
            [candidates = std::move(candidates)](ManualImageList &load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    void deliverImageListAtIgnoringCancellation(
        std::size_t index, std::vector<ImageDocumentPageCandidate> candidates)
    {
        ManualImageList &load = *m_imageLists.at(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

    kiriview::ImageDocumentPageCandidateProvider provider()
    {
        return kiriview::ImageDocumentPageCandidateProvider {
            [this](QObject *receiver, QUrl directoryUrl,
                kiriview::ImageDocumentPageCandidatesCallback callback,
                kiriview::ErrorCallback errorCallback) {
                return startImageList(receiver, std::move(directoryUrl), std::move(callback),
                    std::move(errorCallback));
            },
            [this](QObject *receiver, QUrl directoryUrl,
                kiriview::ContainerCandidatesCallback callback,
                kiriview::ErrorCallback errorCallback) {
                return startContainerList(receiver, std::move(directoryUrl), std::move(callback),
                    std::move(errorCallback));
            },
            [](QObject *, kiriview::OpenedCollectionScopeLocation,
                kiriview::ImageDocumentPageCandidatesCallback,
                kiriview::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected archive image listing"));
                }
                return kiriview::ImageIoJob();
            },
        };
    }

private:
    std::vector<std::shared_ptr<ManualContainerList>> m_containerLists;
    std::vector<std::shared_ptr<ManualImageList>> m_imageLists;
};

kiriview::ImageContainerNavigationController::Callbacks controllerCallbacks(
    std::function<void(const QUrl &, const QUrl &)> openContainerImage = {},
    std::function<void(const QUrl &, kiriview::ContainerNavigationError, const QString &)>
        containerNavigationError
    = {},
    std::function<void(kiriview::NavigationDirection)> containerNavigationBoundary = {},
    std::function<void(const kiriview::ContainerNavigationListFailure &)>
        containerNavigationListFailure
    = {})
{
    return kiriview::ImageContainerNavigationController::Callbacks {
        [openContainerImage = std::move(openContainerImage),
            containerNavigationError = std::move(containerNavigationError),
            containerNavigationBoundary = std::move(containerNavigationBoundary),
            containerNavigationListFailure = std::move(containerNavigationListFailure)](
            kiriview::ImageDocumentPageNavigationPlan plan) mutable {
            for (const kiriview::ImageDocumentPageNavigationEffect &effect : plan) {
                if (const auto *openEffect
                    = std::get_if<kiriview::OpenContainerImageDocumentPageNavigationEffect>(
                        &effect)) {
                    kiriview::invokeIfSet(
                        openContainerImage, openEffect->target.url, openEffect->containerUrl);
                } else if (const auto *errorEffect
                    = std::get_if<kiriview::ReportContainerNavigationErrorEffect>(&effect)) {
                    kiriview::invokeIfSet(containerNavigationError, errorEffect->containerUrl,
                        errorEffect->error, errorEffect->errorString);
                } else if (const auto *boundaryEffect
                    = std::get_if<kiriview::ReportContainerNavigationBoundaryEffect>(&effect)) {
                    kiriview::invokeIfSet(containerNavigationBoundary, boundaryEffect->direction);
                } else if (const auto *listErrorEffect
                    = std::get_if<kiriview::ReportContainerNavigationListErrorEffect>(&effect)) {
                    kiriview::invokeIfSet(containerNavigationListFailure, listErrorEffect->failure);
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
    void reportsPreviousBoundaryWhenNoAdjacentContainerExists();
    void reportsNextBoundaryWhenNoAdjacentContainerExists();
    void missingCurrentContainerDoesNotReportBoundary();
    void forwardsAdjacentContainerListingErrorAsDiagnostic();
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
    kiriview::ImageDocumentPageCandidateRepository repository(provider.provider());
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    const QUrl targetImageUrl = localUrl(QStringLiteral("/books/b/01.png"));

    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
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
    provider.finishImageListAt(0, { imageDocumentPageCandidate(targetImageUrl) });

    QCOMPARE(openedImageUrl, targetImageUrl);
    QCOMPARE(openedContainerUrl, targetContainerUrl);
}

void TestImageContainerNavigationController::opensFirstImageFromAdjacentArchiveContainer()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl targetImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(
                targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        });
    fakeProvider.setOpenedCollectionCandidates(
        archiveCollection->rootUrl(), { imageDocumentPageCandidate(targetImageUrl) });

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks(
            [&openedImageUrl, &openedContainerUrl](const QUrl &imageUrl, const QUrl &containerUrl) {
                openedImageUrl = imageUrl;
                openedContainerUrl = containerUrl;
            }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(openedImageUrl, targetImageUrl);
    QCOMPARE(openedContainerUrl, targetContainerUrl);
}

void TestImageContainerNavigationController::reportsPreviousBoundaryWhenNoAdjacentContainerExists()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(
                localUrl(QStringLiteral("/books/b/")), ContainerNavigationCandidateType::Directory),
        });

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    int boundaryCount = 0;
    NavigationDirection boundaryDirection = NavigationDirection::Next;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks(
            {}, {}, [&boundaryCount, &boundaryDirection](NavigationDirection direction) {
                ++boundaryCount;
                boundaryDirection = direction;
            }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Previous);

    QCOMPARE(boundaryCount, 1);
    QCOMPARE(boundaryDirection, NavigationDirection::Previous);
}

void TestImageContainerNavigationController::reportsNextBoundaryWhenNoAdjacentContainerExists()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/b/"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(
                localUrl(QStringLiteral("/books/a/")), ContainerNavigationCandidateType::Directory),
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
        });

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    int boundaryCount = 0;
    NavigationDirection boundaryDirection = NavigationDirection::Previous;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks(
            {}, {}, [&boundaryCount, &boundaryDirection](NavigationDirection direction) {
                ++boundaryCount;
                boundaryDirection = direction;
            }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(boundaryCount, 1);
    QCOMPARE(boundaryDirection, NavigationDirection::Next);
}

void TestImageContainerNavigationController::missingCurrentContainerDoesNotReportBoundary()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/missing/"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(
                localUrl(QStringLiteral("/books/a/")), ContainerNavigationCandidateType::Directory),
            containerCandidate(
                localUrl(QStringLiteral("/books/b/")), ContainerNavigationCandidateType::Directory),
        });

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    int boundaryCount = 0;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks({}, {}, [&boundaryCount](NavigationDirection) { ++boundaryCount; }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(boundaryCount, 0);
}

void TestImageContainerNavigationController::forwardsAdjacentContainerListingErrorAsDiagnostic()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    fakeProvider.setContainerError(parentUrl, QStringLiteral("No parent access"));

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    bool openedImage = false;
    int openErrorCount = 0;
    QUrl reportedCurrentContainerUrl;
    QUrl reportedParentUrl;
    NavigationDirection reportedDirection = NavigationDirection::Previous;
    QString diagnostic;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
        controllerCallbacks([&openedImage](const QUrl &, const QUrl &) { openedImage = true; },
            [&openErrorCount](
                const QUrl &, ImageContainerOpenError, const QString &) { ++openErrorCount; },
            {},
            [&reportedCurrentContainerUrl, &reportedParentUrl, &reportedDirection, &diagnostic](
                const kiriview::ContainerNavigationListFailure &failure) {
                reportedCurrentContainerUrl = failure.currentContainerUrl;
                reportedParentUrl = failure.parentUrl;
                reportedDirection = failure.direction;
                diagnostic = failure.diagnosticDetail;
                QCOMPARE(
                    failure.kind, kiriview::ContainerNavigationListFailureKind::DirectoryListing);
                QCOMPARE(
                    failure.severity, kiriview::ContainerNavigationListFailureSeverity::Diagnostic);
            }));

    controller.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QVERIFY(!openedImage);
    QCOMPARE(openErrorCount, 0);
    QCOMPARE(reportedCurrentContainerUrl, currentContainerUrl);
    QCOMPARE(reportedParentUrl, parentUrl);
    QCOMPARE(reportedDirection, NavigationDirection::Next);
    QCOMPARE(diagnostic, QStringLiteral("No parent access"));
}

void TestImageContainerNavigationController::reportsEmptyAdjacentContainer()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::Directory),
        });
    fakeProvider.setDirectoryImages(targetContainerUrl, {});

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    QUrl errorContainerUrl;
    ImageContainerOpenError error = ImageContainerOpenError::Generic;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
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
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/not-an-archive.png"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(
                targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        });

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    bool openedImage = false;
    QUrl errorContainerUrl;
    ImageContainerOpenError error = ImageContainerOpenError::Generic;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
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
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::Directory),
        });
    fakeProvider.setDirectoryImageError(targetContainerUrl, QStringLiteral("No access"));

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    QUrl errorContainerUrl;
    ImageContainerOpenError error = ImageContainerOpenError::EmptyContainer;
    QString errorString;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
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
    kiriview::ImageDocumentPageCandidateRepository repository(provider.provider());
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));

    QUrl openedImageUrl;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
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
    kiriview::ImageDocumentPageCandidateRepository repository(provider.provider());
    const QUrl firstCurrentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl firstTargetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    const QUrl firstTargetImageUrl = localUrl(QStringLiteral("/books/b/01.png"));
    const QUrl secondCurrentContainerUrl = localUrl(QStringLiteral("/books/c/"));
    const QUrl secondTargetContainerUrl = localUrl(QStringLiteral("/books/d/"));
    const QUrl secondTargetImageUrl = localUrl(QStringLiteral("/books/d/01.png"));

    std::vector<QUrl> openedImageUrls;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
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
    provider.deliverImageListAtIgnoringCancellation(
        0, { imageDocumentPageCandidate(firstTargetImageUrl) });

    QCOMPARE(provider.containerListCount(), std::size_t(2));
    provider.finishContainerListAt(1,
        {
            containerCandidate(
                secondCurrentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(
                secondTargetContainerUrl, ContainerNavigationCandidateType::Directory),
        });
    QCOMPARE(provider.imageListCount(), std::size_t(2));
    provider.finishImageListAt(1, { imageDocumentPageCandidate(secondTargetImageUrl) });

    QCOMPARE(openedImageUrls.size(), std::size_t(1));
    QCOMPARE(openedImageUrls.front(), secondTargetImageUrl);
}

void TestImageContainerNavigationController::canceledContainerListingCompletionIsIgnored()
{
    ManualContainerNavigationProvider provider;
    kiriview::ImageDocumentPageCandidateRepository repository(provider.provider());
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));

    QUrl openedImageUrl;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
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
    kiriview::ImageDocumentPageCandidateRepository repository(provider.provider());
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    const QUrl targetImageUrl = localUrl(QStringLiteral("/books/b/01.png"));

    QUrl openedImageUrl;
    kiriview::ImageContainerNavigationController controller(nullptr, repository,
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
    provider.deliverImageListAtIgnoringCancellation(
        0, { imageDocumentPageCandidate(targetImageUrl) });

    QVERIFY(openedImageUrl.isEmpty());
}

QTEST_GUILESS_MAIN(TestImageContainerNavigationController)

#include "test_imagecontainernavigationcontroller.moc"

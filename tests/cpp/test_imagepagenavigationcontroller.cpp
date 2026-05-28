// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imagecallback.h"
#include "candidate_test_support.h"
#include "navigation/imagepagenavigationcontroller.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <functional>
#include <utility>
#include <variant>
#include <vector>

namespace {
using KiriView::ImageCandidateListContext;
using KiriView::ImageCandidateRepository;
using KiriView::ImageNavigationCandidate;
using KiriView::ImagePageNavigationController;
using KiriView::NavigationDirection;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

class DelayedDirectoryImageProvider
{
public:
    KiriView::ImageNavigationCandidateProvider provider()
    {
        return KiriView::ImageNavigationCandidateProvider {
            [this](QObject *, QUrl directoryUrl, KiriView::ImageCandidatesCallback callback,
                KiriView::ErrorCallback) {
                m_loads.push_back(Load { std::move(directoryUrl), std::move(callback) });
                return KiriView::ImageIoJob();
            },
            [](QObject *, QUrl, KiriView::ContainerCandidatesCallback, KiriView::ErrorCallback) {
                return KiriView::ImageIoJob();
            },
            [](QObject *, KiriView::OpenedCollectionScopeLocation,
                KiriView::ImageCandidatesCallback,
                KiriView::ErrorCallback) { return KiriView::ImageIoJob(); },
            [](QObject *, QUrl, KiriView::ImageCandidatesCallback, KiriView::ErrorCallback) {
                return KiriView::ImageIoJob();
            },
        };
    }

    std::size_t loadCount() const { return m_loads.size(); }

    const QUrl &loadDirectory(std::size_t index) const { return m_loads.at(index).directoryUrl; }

    void finishLoad(std::size_t index, std::vector<ImageNavigationCandidate> candidates)
    {
        KiriView::ImageCandidatesCallback callback = m_loads.at(index).callback;
        if (callback) {
            callback(std::move(candidates));
        }
    }

private:
    struct Load {
        QUrl directoryUrl;
        KiriView::ImageCandidatesCallback callback;
    };

    std::vector<Load> m_loads;
};

ImagePageNavigationController::Callbacks controllerCallbacks(
    std::function<void(const QUrl &)> openUrl = {},
    ImagePageNavigationController::PageNavigationChangedCallback pageNavigationChanged = {},
    std::function<void()> clearCurrentImage = {},
    ImagePageNavigationController::DeletionInProgressCallback deletionInProgress = {})
{
    return ImagePageNavigationController::Callbacks {
        [openUrl = std::move(openUrl), clearCurrentImage = std::move(clearCurrentImage)](
            KiriView::ImageNavigationPlan plan) mutable {
            for (const KiriView::ImageNavigationEffect &effect : plan) {
                if (const auto *openEffect
                    = std::get_if<KiriView::OpenImageNavigationUrlEffect>(&effect)) {
                    KiriView::invokeIfSet(openUrl, openEffect->target.url);
                } else if (std::holds_alternative<KiriView::ClearCurrentImageNavigationEffect>(
                               effect)) {
                    KiriView::invokeIfSet(clearCurrentImage);
                }
            }
        },
        std::move(pageNavigationChanged),
        std::move(deletionInProgress),
    };
}

ImageCandidateListContext directoryContext(const QUrl &currentUrl, const QUrl &directoryUrl)
{
    return ImageCandidateListContext::forDirectory(currentUrl, directoryUrl);
}
}

class TestImagePageNavigationController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateOwnsRefreshStateAndReusesWatcher();
    void changedCandidatesRecoverRemovedImageAfterStateUpdate();
    void deletionInProgressSuppressesRemovedCurrentImageRecovery();
    void fallbackAdjacentNavigationPublishesTargetBeforeOpening();
    void staleUpdateCompletionCannotReplaceCurrentRefreshContext();
    void staleSameSourceUpdateCompletionIsRejected();
    void cancelUpdateStopsCandidateChangeWatcher();
};

void TestImagePageNavigationController::updateOwnsRefreshStateAndReusesWatcher()
{
    FakeCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    fakeProvider.setDirectoryImages(directoryUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });

    ImageCandidateRepository repository(fakeProvider.provider());
    int changeCount = 0;
    ImagePageNavigationController controller(
        nullptr, repository, controllerCallbacks({}, [&changeCount]() { ++changeCount; }));

    controller.update(directoryContext(firstUrl, directoryUrl));
    QCOMPARE(controller.currentPageNumber(), 1);
    QCOMPARE(controller.imageCount(), 2);
    QCOMPARE(changeCount, 1);
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(directoryUrl), 1);

    controller.update(directoryContext(secondUrl, directoryUrl));
    QCOMPARE(controller.currentPageNumber(), 2);
    QCOMPARE(controller.imageCount(), 2);
    QCOMPARE(changeCount, 2);
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(directoryUrl), 1);
}

void TestImagePageNavigationController::changedCandidatesRecoverRemovedImageAfterStateUpdate()
{
    FakeCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(directoryUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    ImageCandidateRepository repository(fakeProvider.provider());
    QUrl openedUrl;
    int clearCount = 0;
    ImagePageNavigationController controller(nullptr, repository,
        controllerCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }, {},
            [&clearCount]() { ++clearCount; }));
    controller.update(directoryContext(secondUrl, directoryUrl));

    fakeProvider.emitDirectoryImageChanges(directoryUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(thirdUrl),
        });

    QCOMPARE(controller.currentPageNumber(), 0);
    QCOMPARE(controller.imageCount(), 2);
    QCOMPARE(clearCount, 1);
    QCOMPARE(openedUrl, thirdUrl);
}

void TestImagePageNavigationController::deletionInProgressSuppressesRemovedCurrentImageRecovery()
{
    FakeCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(directoryUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    ImageCandidateRepository repository(fakeProvider.provider());
    QUrl openedUrl;
    int clearCount = 0;
    bool deletionInProgress = true;
    ImagePageNavigationController controller(nullptr, repository,
        controllerCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }, {},
            [&clearCount]() { ++clearCount; },
            [&deletionInProgress]() { return deletionInProgress; }));
    controller.update(directoryContext(secondUrl, directoryUrl));

    fakeProvider.emitDirectoryImageChanges(directoryUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(thirdUrl),
        });

    QCOMPARE(controller.currentPageNumber(), 0);
    QCOMPARE(controller.imageCount(), 2);
    QCOMPARE(clearCount, 0);
    QVERIFY(openedUrl.isEmpty());
}

void TestImagePageNavigationController::fallbackAdjacentNavigationPublishesTargetBeforeOpening()
{
    FakeCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(directoryUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    ImageCandidateRepository repository(fakeProvider.provider());
    QUrl openedUrl;
    int currentPageAtOpen = 0;
    int imageCountAtOpen = 0;
    ImagePageNavigationController *controllerPtr = nullptr;
    ImagePageNavigationController controller(
        nullptr, repository, controllerCallbacks([&](const QUrl &url) {
            openedUrl = url;
            currentPageAtOpen = controllerPtr->currentPageNumber();
            imageCountAtOpen = controllerPtr->imageCount();
        }));
    controllerPtr = &controller;

    controller.openAdjacentImage(
        directoryContext(firstUrl, directoryUrl), NavigationDirection::Next);

    QCOMPARE(openedUrl, secondUrl);
    QCOMPARE(currentPageAtOpen, 2);
    QCOMPARE(imageCountAtOpen, 3);
    QCOMPARE(controller.currentPageNumber(), 2);
    QCOMPARE(controller.imageCount(), 3);
}

void TestImagePageNavigationController::staleUpdateCompletionCannotReplaceCurrentRefreshContext()
{
    DelayedDirectoryImageProvider delayedProvider;
    ImageCandidateRepository repository(delayedProvider.provider());
    int changeCount = 0;
    ImagePageNavigationController controller(
        nullptr, repository, controllerCallbacks({}, [&changeCount]() { ++changeCount; }));
    const QUrl firstDirectoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl secondDirectoryUrl = localUrl(QStringLiteral("/other/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/other/02.png"));

    controller.update(directoryContext(firstUrl, firstDirectoryUrl));
    controller.update(directoryContext(secondUrl, secondDirectoryUrl));
    QCOMPARE(delayedProvider.loadCount(), std::size_t(2));
    QCOMPARE(delayedProvider.loadDirectory(0), firstDirectoryUrl);
    QCOMPARE(delayedProvider.loadDirectory(1), secondDirectoryUrl);

    delayedProvider.finishLoad(0, { imageCandidate(firstUrl) });
    QCOMPARE(controller.currentPageNumber(), 0);
    QCOMPARE(controller.imageCount(), 0);
    QCOMPARE(changeCount, 0);

    delayedProvider.finishLoad(1, { imageCandidate(secondUrl) });
    QCOMPARE(controller.currentPageNumber(), 1);
    QCOMPARE(controller.imageCount(), 1);
    QCOMPARE(changeCount, 1);
}

void TestImagePageNavigationController::staleSameSourceUpdateCompletionIsRejected()
{
    DelayedDirectoryImageProvider delayedProvider;
    ImageCandidateRepository repository(delayedProvider.provider());
    int changeCount = 0;
    ImagePageNavigationController controller(
        nullptr, repository, controllerCallbacks({}, [&changeCount]() { ++changeCount; }));
    const QUrl directoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));

    controller.update(directoryContext(firstUrl, directoryUrl));
    controller.update(directoryContext(secondUrl, directoryUrl));
    QCOMPARE(delayedProvider.loadCount(), std::size_t(2));
    QCOMPARE(delayedProvider.loadDirectory(0), directoryUrl);
    QCOMPARE(delayedProvider.loadDirectory(1), directoryUrl);

    delayedProvider.finishLoad(0, { imageCandidate(firstUrl) });
    QCOMPARE(controller.currentPageNumber(), 0);
    QCOMPARE(controller.imageCount(), 0);
    QCOMPARE(changeCount, 0);

    delayedProvider.finishLoad(1, { imageCandidate(firstUrl), imageCandidate(secondUrl) });
    QCOMPARE(controller.currentPageNumber(), 2);
    QCOMPARE(controller.imageCount(), 2);
    QCOMPARE(changeCount, 1);
}

void TestImagePageNavigationController::cancelUpdateStopsCandidateChangeWatcher()
{
    FakeCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    fakeProvider.setDirectoryImages(directoryUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });

    ImageCandidateRepository repository(fakeProvider.provider());
    ImagePageNavigationController controller(nullptr, repository, controllerCallbacks());
    controller.update(directoryContext(firstUrl, directoryUrl));
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(directoryUrl), 1);

    controller.cancelUpdate();
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(directoryUrl), 0);

    fakeProvider.emitDirectoryImageChanges(directoryUrl, { imageCandidate(secondUrl) });
    QCOMPARE(controller.currentPageNumber(), 1);
    QCOMPARE(controller.imageCount(), 2);
}

QTEST_GUILESS_MAIN(TestImagePageNavigationController)

#include "test_imagepagenavigationcontroller.moc"

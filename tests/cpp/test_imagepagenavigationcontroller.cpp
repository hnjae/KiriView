// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagepagenavigationcontroller.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <utility>
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

ImagePageNavigationController::Callbacks controllerCallbacks(
    ImagePageNavigationController::OpenUrlCallback openUrl = {},
    ImagePageNavigationController::PageNavigationChangedCallback pageNavigationChanged = {},
    ImagePageNavigationController::CurrentImageRemovedCallback currentImageRemoved = {})
{
    return ImagePageNavigationController::Callbacks {
        std::move(openUrl),
        std::move(pageNavigationChanged),
        std::move(currentImageRemoved),
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
    void changedCandidatesReportRemovalAfterStateUpdate();
    void fallbackAdjacentNavigationPublishesTargetBeforeOpening();
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

void TestImagePageNavigationController::changedCandidatesReportRemovalAfterStateUpdate()
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
    int removalCount = 0;
    std::optional<ImageCandidateListContext> removedContext;
    std::vector<ImageNavigationCandidate> removalCandidates;
    ImagePageNavigationController controller(nullptr, repository,
        controllerCallbacks({}, {},
            [&](std::vector<ImageNavigationCandidate> candidates,
                ImageCandidateListContext context) {
                ++removalCount;
                removalCandidates = std::move(candidates);
                removedContext = std::move(context);
            }));
    controller.update(directoryContext(secondUrl, directoryUrl));

    fakeProvider.emitDirectoryImageChanges(directoryUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(thirdUrl),
        });

    QCOMPARE(controller.currentPageNumber(), 0);
    QCOMPARE(controller.imageCount(), 2);
    QCOMPARE(removalCount, 1);
    QVERIFY(removedContext.has_value());
    QCOMPARE(removedContext->currentUrl(), secondUrl);
    QCOMPARE(removalCandidates.size(), std::size_t(2));
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

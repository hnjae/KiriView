// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentnavigationcontroller.h"

#include "document/imagedocumentstate.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagedocumentpagenavigationservice.h"
#include "presentation/imagepresentationcontroller.h"
#include "presentation/imagespreadpresentationcontroller.h"
#include "rendering/imagerendering.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::imageDocumentPageCandidate;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageDocumentPageCandidateProvider;

KiriView::ImageDocumentRenderContext renderContext()
{
    return KiriView::ImageDocumentRenderContext {
        1.0,
        KiriView::fallbackTextureSizeMax,
    };
}

KiriView::ImageCacheBudgets testCacheBudgets()
{
    return KiriView::ImageCacheBudgets {
        1024 * 1024,
        512 * 1024,
    };
}

template <typename Operation>
const Operation *findOperation(const KiriView::ImageDocumentRuntimePlan &plan)
{
    for (const KiriView::ImageDocumentRuntimeOperation &operation : plan) {
        if (const auto *payload = std::get_if<Operation>(&operation)) {
            return payload;
        }
    }

    return nullptr;
}

class DocumentNavigationFixture
{
public:
    DocumentNavigationFixture()
        : presentation(&context, renderContext, {}, testCacheBudgets())
        , navigation(&context, candidateProvider.provider(),
              KiriView::ImageDocumentPageNavigationService::Callbacks {
                  [this](KiriView::ImageDocumentPageNavigationPlan plan) {
                      for (const KiriView::ImageDocumentPageNavigationEffect &effect : plan) {
                          if (const auto *openEffect
                              = std::get_if<KiriView::OpenImageDocumentPageUrlEffect>(&effect)) {
                              runtimePlans.push_back(KiriView::ImageDocumentRuntimePlan {
                                  KiriView::LoadUrlOperation { openEffect->target } });
                          }
                      }
                  },
                  {},
              })
        , spread(&context, renderContext, state, presentation,
              KiriView::ImageSpreadPresentationController::Callbacks {
                  {},
                  {},
                  [this]() { return navigation.pageNavigationSnapshot(); },
                  {},
              },
              candidateProvider.provider(),
              imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder(testImage())),
              testCacheBudgets())
        , controller(state, presentation, navigation, spread,
              [this](KiriView::ImageDocumentRuntimePlan plan) {
                  runtimePlans.push_back(std::move(plan));
              })
    {
    }

    void displayImage(const QUrl &url)
    {
        state.setSourceUrl(url);
        state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(url));
        presentation.setStaticImage(staticTestImagePayload(testImage()), false);
    }

    void displayComicPage(
        const QUrl &url, const KiriView::OpenedCollectionScopeLocation &archiveCollection)
    {
        state.setSourceUrl(url);
        state.setDisplayedImageLocation(
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(url, archiveCollection));
        presentation.setStaticImage(staticTestImagePayload(testImage(QSize(800, 1200))), false);
    }

    QObject context;
    KiriView::ImageDocumentState state;
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePresentationController presentation;
    KiriView::ImageDocumentPageNavigationService navigation;
    KiriView::ImageSpreadPresentationController spread;
    KiriView::ImageDocumentNavigationController controller;
    std::vector<KiriView::ImageDocumentRuntimePlan> runtimePlans;
};
}

class TestImageDocumentNavigationController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updatePageNavigationUsesOpenedCollectionContext();
    void updatePageNavigationIgnoresOrdinaryDirectImage();
    void updatePageNavigationRequiresPresentedImage();
    void stalePageNavigationCannotDispatchForOrdinaryDirectImage();
    void adjacentImageDocumentPageNavigationUsesOpenedCollectionContext();
    void pageSelectionDispatchesPageNavigationRuntimePlan();
    void spreadPageSelectionStartsTrackedTransition();
};

void TestImageDocumentNavigationController::updatePageNavigationUsesOpenedCollectionContext()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    fixture.candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });
    fixture.displayComicPage(firstUrl, *archiveCollection);

    fixture.controller.updatePageNavigation();

    QCOMPARE(fixture.controller.currentPageNumber(), 1);
    QCOMPARE(fixture.controller.pageCount(), 2);
}

void TestImageDocumentNavigationController::updatePageNavigationIgnoresOrdinaryDirectImage()
{
    DocumentNavigationFixture fixture;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    fixture.candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });
    fixture.displayImage(firstUrl);

    fixture.controller.updatePageNavigation();

    QCOMPARE(fixture.controller.currentPageNumber(), 0);
    QCOMPARE(fixture.controller.pageCount(), 0);
}

void TestImageDocumentNavigationController::updatePageNavigationRequiresPresentedImage()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    fixture.candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });
    fixture.state.setSourceUrl(firstUrl);
    fixture.state.setDisplayedImageLocation(
        KiriView::DisplayedImageLocation::fromOpenedCollectionScope(firstUrl, *archiveCollection));

    fixture.controller.updatePageNavigation();
    fixture.controller.openAdjacentPage(KiriView::NavigationDirection::Next);

    QCOMPARE(fixture.controller.currentPageNumber(), 0);
    QCOMPARE(fixture.controller.pageCount(), 0);
    QVERIFY(fixture.runtimePlans.empty());
}

void TestImageDocumentNavigationController::
    stalePageNavigationCannotDispatchForOrdinaryDirectImage()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    fixture.candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
        });
    fixture.displayComicPage(firstPageUrl, *archiveCollection);
    fixture.controller.updatePageNavigation();
    QCOMPARE(fixture.controller.pageCount(), 2);

    fixture.runtimePlans.clear();
    fixture.displayImage(localUrl(QStringLiteral("/images/01.png")));

    fixture.controller.openAdjacentPage(KiriView::NavigationDirection::Next);

    QVERIFY(fixture.runtimePlans.empty());
    QCOMPARE(fixture.controller.currentPageNumber(), 0);
    QCOMPARE(fixture.controller.pageCount(), 0);

    fixture.displayComicPage(firstPageUrl, *archiveCollection);
    fixture.controller.updatePageNavigation();
    QCOMPARE(fixture.controller.pageCount(), 2);
    fixture.runtimePlans.clear();
    fixture.displayImage(localUrl(QStringLiteral("/images/02.png")));

    fixture.controller.openImageAtPage(2);

    QVERIFY(fixture.runtimePlans.empty());
    QCOMPARE(fixture.controller.currentPageNumber(), 0);
    QCOMPARE(fixture.controller.pageCount(), 0);
}

void TestImageDocumentNavigationController::
    adjacentImageDocumentPageNavigationUsesOpenedCollectionContext()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    fixture.candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });
    fixture.displayComicPage(firstUrl, *archiveCollection);

    fixture.controller.openAdjacentPage(KiriView::NavigationDirection::Next);

    QCOMPARE(fixture.runtimePlans.size(), std::size_t(1));
    const auto *operation = findOperation<KiriView::LoadUrlOperation>(fixture.runtimePlans.front());
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->target.url, secondUrl);
}

void TestImageDocumentNavigationController::pageSelectionDispatchesPageNavigationRuntimePlan()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.mp4"));
    fixture.candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstUrl),
            KiriView::ImageDocumentPageCandidate {
                secondUrl, QStringLiteral("02.mp4"), KiriView::ImageDocumentPageKind::Video },
        });
    fixture.displayComicPage(firstUrl, *archiveCollection);
    fixture.controller.updatePageNavigation();

    fixture.controller.openImageAtPage(2);

    QCOMPARE(fixture.runtimePlans.size(), std::size_t(1));
    const auto *operation
        = findOperation<KiriView::LoadPageNavigationUrlOperation>(fixture.runtimePlans.front());
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->target.url, secondUrl);
    QCOMPARE(operation->target.kind, KiriView::ImageDocumentPageKind::Video);
    QVERIFY(!operation->preserveTwoPageSpreadTransition);
    QCOMPARE(fixture.controller.currentPageNumber(), 2);
}

void TestImageDocumentNavigationController::spreadPageSelectionStartsTrackedTransition()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    fixture.candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });
    fixture.displayComicPage(firstUrl, *archiveCollection);
    fixture.controller.updatePageNavigation();
    fixture.spread.setTwoPageModeEnabled(true);

    fixture.controller.openImageAtPage(2);

    QCOMPARE(fixture.runtimePlans.size(), std::size_t(1));
    const auto *operation
        = findOperation<KiriView::LoadPageNavigationUrlOperation>(fixture.runtimePlans.front());
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->target.url, secondUrl);
    QVERIFY(operation->preserveTwoPageSpreadTransition);
    QVERIFY(fixture.spread.transitionInProgress());
}

QTEST_GUILESS_MAIN(TestImageDocumentNavigationController)

#include "test_imagedocumentnavigationcontroller.moc"

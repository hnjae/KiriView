// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentnavigationcontroller.h"

#include "document/imagedocumentstate.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagedocumentpagenavigationservice.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagepresentationruntime.h"
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
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::imageDecodeDependenciesFor;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::staticImageDataDecoder;
using kiriview::TestSupport::testImage;

using FakeCandidateProvider = kiriview::TestSupport::FakeImageDocumentPageCandidateProvider;

kiriview::ImageDocumentRenderContext renderContext()
{
    return kiriview::ImageDocumentRenderContext {
        1.0,
        kiriview::fallbackTextureSizeMax,
    };
}

kiriview::ImageCacheBudgets testCacheBudgets()
{
    return kiriview::ImageCacheBudgets {
        1024 * 1024,
        512 * 1024,
    };
}

template <typename Operation>
const Operation *findOperation(const kiriview::ImageDocumentRuntimePlan &plan)
{
    for (const kiriview::ImageDocumentRuntimeOperation &operation : plan) {
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
        : pageSurface(&context, {}, testCacheBudgets())
        , presentationRuntime(renderContext)
        , navigation(&context, candidateProvider.provider(),
              kiriview::ImageDocumentPageNavigationService::Callbacks {
                  [this](kiriview::ImageDocumentPageNavigationPlan plan) {
                      for (const kiriview::ImageDocumentPageNavigationEffect &effect : plan) {
                          if (const auto *openEffect
                              = std::get_if<kiriview::OpenImageDocumentPageUrlEffect>(&effect)) {
                              runtimePlans.push_back(kiriview::ImageDocumentRuntimePlan {
                                  kiriview::LoadUrlOperation { openEffect->target } });
                          }
                      }
                  },
                  {},
              })
        , spread(&context, renderContext, state, pageSurface, presentationRuntime,
              kiriview::ImageSpreadPresentationController::Callbacks {
                  {},
                  {},
                  [this]() { return navigation.pageNavigationSnapshot(); },
                  {},
              },
              candidateProvider.provider(),
              imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder(testImage())),
              testCacheBudgets())
        , controller(state, pageSurface, navigation, spread,
              [this](kiriview::ImageDocumentRuntimePlan plan) {
                  runtimePlans.push_back(std::move(plan));
              })
    {
    }

    void displayImage(const QUrl &url)
    {
        state.setSourceUrl(url);
        state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(url));
        pageSurface.setStaticDisplayImage(
            staticDisplayTestImagePayload(testImage()), false, renderContext());
        spread.commitPrimaryPageSlot(state.displayedImageLocation());
    }

    void displayComicPage(
        const QUrl &url, const kiriview::OpenedCollectionScopeLocation &archiveCollection)
    {
        state.setSourceUrl(url);
        state.setDisplayedImageLocation(
            kiriview::DisplayedImageLocation::fromOpenedCollectionScope(url, archiveCollection));
        pageSurface.setStaticDisplayImage(
            staticDisplayTestImagePayload(testImage(QSize(800, 1200))), false, renderContext());
        spread.commitPrimaryPageSlot(state.displayedImageLocation());
    }

    QObject context;
    kiriview::ImageDocumentState state;
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    kiriview::ImagePageSurfaceController pageSurface;
    kiriview::ImagePresentationRuntime presentationRuntime;
    kiriview::ImageDocumentPageNavigationService navigation;
    kiriview::ImageSpreadPresentationController spread;
    kiriview::ImageDocumentNavigationController controller;
    std::vector<kiriview::ImageDocumentRuntimePlan> runtimePlans;
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
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
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
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
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
        kiriview::DisplayedImageLocation::fromOpenedCollectionScope(firstUrl, *archiveCollection));

    fixture.controller.updatePageNavigation();
    fixture.controller.openAdjacentPage(kiriview::NavigationDirection::Next);

    QCOMPARE(fixture.controller.currentPageNumber(), 0);
    QCOMPARE(fixture.controller.pageCount(), 0);
    QVERIFY(fixture.runtimePlans.empty());
}

void TestImageDocumentNavigationController::
    stalePageNavigationCannotDispatchForOrdinaryDirectImage()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
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

    fixture.controller.openAdjacentPage(kiriview::NavigationDirection::Next);

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
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    fixture.candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });
    fixture.displayComicPage(firstUrl, *archiveCollection);

    fixture.controller.openAdjacentPage(kiriview::NavigationDirection::Next);

    QCOMPARE(fixture.runtimePlans.size(), std::size_t(1));
    const auto *operation = findOperation<kiriview::LoadUrlOperation>(fixture.runtimePlans.front());
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->target.url, secondUrl);
}

void TestImageDocumentNavigationController::pageSelectionDispatchesPageNavigationRuntimePlan()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.mp4"));
    fixture.candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstUrl),
            kiriview::ImageDocumentPageCandidate {
                secondUrl, QStringLiteral("02.mp4"), kiriview::ImageDocumentPageKind::Video },
        });
    fixture.displayComicPage(firstUrl, *archiveCollection);
    fixture.controller.updatePageNavigation();

    fixture.controller.openImageAtPage(2);

    QCOMPARE(fixture.runtimePlans.size(), std::size_t(1));
    const auto *operation
        = findOperation<kiriview::LoadPageNavigationUrlOperation>(fixture.runtimePlans.front());
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->target.url, secondUrl);
    QCOMPARE(operation->target.kind, kiriview::ImageDocumentPageKind::Video);
    QVERIFY(!operation->preserveTwoPageSpreadTransition);
    QCOMPARE(fixture.controller.currentPageNumber(), 2);
}

void TestImageDocumentNavigationController::spreadPageSelectionStartsTrackedTransition()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
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
        = findOperation<kiriview::LoadPageNavigationUrlOperation>(fixture.runtimePlans.front());
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->target.url, secondUrl);
    QVERIFY(operation->preserveTwoPageSpreadTransition);
    QVERIFY(fixture.spread.transitionInProgress());
}

QTEST_GUILESS_MAIN(TestImageDocumentNavigationController)

#include "test_imagedocumentnavigationcontroller.moc"

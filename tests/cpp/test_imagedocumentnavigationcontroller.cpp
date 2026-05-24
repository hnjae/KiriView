// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentnavigationcontroller.h"

#include "document/imagedocumentstate.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagenavigationservice.h"
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
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

KiriView::ImageDocumentRenderContext renderContext()
{
    return KiriView::ImageDocumentRenderContext {
        1.0,
        KiriView::fallbackTextureSizeMax,
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
        : presentation(&context, renderContext, {})
        , navigation(&context, candidateProvider.provider(),
              KiriView::ImageNavigationService::Callbacks {
                  [this](KiriView::ImageNavigationPlan plan) {
                      for (const KiriView::ImageNavigationEffect &effect : plan) {
                          if (const auto *openEffect
                              = std::get_if<KiriView::OpenImageNavigationUrlEffect>(&effect)) {
                              runtimePlans.push_back(KiriView::ImageDocumentRuntimePlan {
                                  KiriView::LoadUrlOperation { openEffect->url } });
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
              imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder(testImage())))
        , controller(state, presentation, navigation, spread,
              [this](KiriView::ImageDocumentRuntimePlan plan) {
                  runtimePlans.push_back(std::move(plan));
              })
    {
    }

    void displayImage(const QUrl &url)
    {
        state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(url));
        presentation.setStaticImage(staticTestImagePayload(testImage()), false);
    }

    void displayComicPage(const QUrl &url, const KiriView::ArchiveDocumentLocation &archiveDocument)
    {
        state.setDisplayedImageLocation(
            KiriView::DisplayedImageLocation::fromArchiveDocument(url, archiveDocument));
        presentation.setStaticImage(staticTestImagePayload(testImage(QSize(800, 1200))), false);
    }

    QObject context;
    KiriView::ImageDocumentState state;
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePresentationController presentation;
    KiriView::ImageNavigationService navigation;
    KiriView::ImageSpreadPresentationController spread;
    KiriView::ImageDocumentNavigationController controller;
    std::vector<KiriView::ImageDocumentRuntimePlan> runtimePlans;
};
}

class TestImageDocumentNavigationController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updatePageNavigationUsesDisplayedImageContext();
    void updatePageNavigationRequiresPresentedImage();
    void adjacentImageNavigationUsesDisplayedImageContext();
    void pageSelectionDispatchesPageNavigationRuntimePlan();
    void spreadPageSelectionStartsTrackedTransition();
};

void TestImageDocumentNavigationController::updatePageNavigationUsesDisplayedImageContext()
{
    DocumentNavigationFixture fixture;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    fixture.candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });
    fixture.displayImage(firstUrl);

    fixture.controller.updatePageNavigation();

    QCOMPARE(fixture.controller.currentPageNumber(), 1);
    QCOMPARE(fixture.controller.imageCount(), 2);
}

void TestImageDocumentNavigationController::updatePageNavigationRequiresPresentedImage()
{
    DocumentNavigationFixture fixture;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    fixture.candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });
    fixture.state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(firstUrl));

    fixture.controller.updatePageNavigation();
    fixture.controller.openAdjacentImage(KiriView::NavigationDirection::Next);

    QCOMPARE(fixture.controller.currentPageNumber(), 0);
    QCOMPARE(fixture.controller.imageCount(), 0);
    QVERIFY(fixture.runtimePlans.empty());
}

void TestImageDocumentNavigationController::adjacentImageNavigationUsesDisplayedImageContext()
{
    DocumentNavigationFixture fixture;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    fixture.candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });
    fixture.displayImage(firstUrl);

    fixture.controller.openAdjacentImage(KiriView::NavigationDirection::Next);

    QCOMPARE(fixture.runtimePlans.size(), std::size_t(1));
    const auto *operation = findOperation<KiriView::LoadUrlOperation>(fixture.runtimePlans.front());
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->url, secondUrl);
}

void TestImageDocumentNavigationController::pageSelectionDispatchesPageNavigationRuntimePlan()
{
    DocumentNavigationFixture fixture;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    fixture.candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });
    fixture.displayImage(firstUrl);
    fixture.controller.updatePageNavigation();

    fixture.controller.openImageAtPage(2);

    QCOMPARE(fixture.runtimePlans.size(), std::size_t(1));
    const auto *operation
        = findOperation<KiriView::LoadPageNavigationUrlOperation>(fixture.runtimePlans.front());
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->url, secondUrl);
    QVERIFY(!operation->preserveTwoPageSpreadTransition);
    QCOMPARE(fixture.controller.currentPageNumber(), 2);
}

void TestImageDocumentNavigationController::spreadPageSelectionStartsTrackedTransition()
{
    DocumentNavigationFixture fixture;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    fixture.candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });
    fixture.displayComicPage(firstUrl, *archiveDocument);
    fixture.controller.updatePageNavigation();
    fixture.spread.setTwoPageModeEnabled(true);

    fixture.controller.openImageAtPage(2);

    QCOMPARE(fixture.runtimePlans.size(), std::size_t(1));
    const auto *operation
        = findOperation<KiriView::LoadPageNavigationUrlOperation>(fixture.runtimePlans.front());
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->url, secondUrl);
    QVERIFY(operation->preserveTwoPageSpreadTransition);
    QVERIFY(fixture.spread.transitionInProgress());
}

QTEST_GUILESS_MAIN(TestImageDocumentNavigationController)

#include "test_imagedocumentnavigationcontroller.moc"

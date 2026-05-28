// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadpresentationcontroller.h"

#include "image_test_support.h"
#include "presentation/imagepresentationcontroller.h"
#include "rendering/imagerendering.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>
#include <map>
#include <optional>
#include <vector>

namespace {
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

KiriView::ImageDocumentRenderContext renderContext()
{
    return KiriView::ImageDocumentRenderContext {
        1.0,
        KiriView::fallbackTextureSizeMax,
    };
}

KiriView::OpenedCollectionScopeLocation openedCollectionScope()
{
    return KiriView::OpenedCollectionScopeLocation::fromUrls(
        localUrl(QStringLiteral("/books/book.cbz")), localUrl(QStringLiteral("/books/book.cbz/")),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
}

KiriView::DisplayedImageLocation displayedLocation(const QUrl &url)
{
    return KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
        url, openedCollectionScope());
}

KiriView::ImageDocumentPageNavigationSnapshot navigationSnapshot(
    const std::vector<QUrl> &urls, int currentPageNumber)
{
    return KiriView::ImageDocumentPageNavigationSnapshot {
        KiriView::PageNavigationState {
            urls,
            currentPageNumber - 1,
        },
    };
}

class SpreadPresentationFixture
{
public:
    SpreadPresentationFixture()
        : primaryPresentation(&context, renderContext, {})
        , controller(&context, renderContext, state, primaryPresentation,
              KiriView::ImageSpreadPresentationController::Callbacks {
                  {},
                  [this](const QUrl &url) { return findPredecodedImage(url); },
                  [this]() { return snapshot; },
                  {},
              },
              {}, {})
    {
        primaryPresentation.setViewportSize(QSizeF(800.0, 600.0));
    }

    void displayPrimaryPage(const QUrl &url, const QSize &imageSize, int pageNumber)
    {
        snapshot = navigationSnapshot(pageUrls, pageNumber);
        state.setDisplayedImageLocation(displayedLocation(url));
        primaryPresentation.setStaticImage(staticTestImagePayload(testImage(imageSize)), false);
    }

    std::optional<KiriView::PredecodedImage> findPredecodedImage(const QUrl &url) const
    {
        const auto imageSize = predecodedImageSizes.find(url);
        if (imageSize == predecodedImageSizes.cend()) {
            return std::nullopt;
        }

        return KiriView::PredecodedImage {
            staticTestImagePayload(testImage(imageSize->second)),
            displayedLocation(url),
        };
    }

    QObject context;
    KiriView::ImageDocumentState state;
    KiriView::ImagePresentationController primaryPresentation;
    std::vector<QUrl> pageUrls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
        localUrl(QStringLiteral("/books/004.png")),
        localUrl(QStringLiteral("/books/005.png")),
        localUrl(QStringLiteral("/books/006.png")),
    };
    std::map<QUrl, QSize> predecodedImageSizes;
    KiriView::ImageDocumentPageNavigationSnapshot snapshot;
    KiriView::ImageSpreadPresentationController controller;
};
}

class TestImageSpreadPresentationController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pageWidthCacheBelongsToSpreadNavigationOwner();
};

void TestImageSpreadPresentationController::pageWidthCacheBelongsToSpreadNavigationOwner()
{
    SpreadPresentationFixture fixture;

    fixture.displayPrimaryPage(fixture.pageUrls.at(3), QSize(1200, 800), 4);
    fixture.controller.setTwoPageModeEnabled(true);
    QVERIFY(fixture.controller.twoPageModeActive());
    QVERIFY(!fixture.controller.secondaryPageVisible());

    fixture.predecodedImageSizes[fixture.pageUrls.at(5)] = QSize(800, 1200);
    fixture.displayPrimaryPage(fixture.pageUrls.at(4), QSize(800, 1200), 5);
    fixture.controller.refreshSecondaryPage();
    QVERIFY(fixture.controller.secondaryPageVisible());

    const KiriView::ImageSpreadPageNavigationTarget target
        = fixture.controller.imageDocumentPageNavigationTarget(
            KiriView::NavigationDirection::Previous);

    QVERIFY(target.handledBySpread);
    QCOMPARE(target.pageNumber, 4);
}

QTEST_GUILESS_MAIN(TestImageSpreadPresentationController)

#include "test_imagespreadpresentationcontroller.moc"

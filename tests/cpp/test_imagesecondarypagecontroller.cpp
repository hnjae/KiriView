// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagesecondarypagecontroller.h"

#include "image_test_support.h"
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

KiriView::DisplayedImageLocation displayedLocation(const QUrl &url)
{
    return KiriView::DisplayedImageLocation::fromUrl(url);
}

struct SecondaryPageLoadReport {
    KiriView::ImageSecondaryPageLoadResult result = KiriView::ImageSecondaryPageLoadResult::Failed;
    KiriView::DisplayedImageLocation location;
    QSize imageSize;
};

class SecondaryPageFixture
{
public:
    SecondaryPageFixture()
        : controller(&context, renderContext,
              KiriView::ImageSecondaryPageController::Callbacks {
                  [this](KiriView::ImageDocumentChange change) { changes.push_back(change); },
                  [this](KiriView::ImageSecondaryPageLoadResult result,
                      const KiriView::DisplayedImageLocation &location, const QSize &imageSize) {
                      loadReports.push_back(
                          SecondaryPageLoadReport { result, location, imageSize });
                  },
                  [this]() { ++visibilityChangedCount; },
                  [this](const QUrl &url) { return findPredecodedImage(url); },
              },
              {}, {})
    {
        controller.setViewportSize(QSizeF(800.0, 600.0));
    }

    void addPredecodedPage(const QUrl &url, const QSize &imageSize)
    {
        predecodedImageSizes[url] = imageSize;
    }

    void startLoad(const QUrl &url)
    {
        controller.startLoad(url, KiriView::ArchiveDocumentLocation::none(), {});
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
    std::map<QUrl, QSize> predecodedImageSizes;
    std::vector<KiriView::ImageDocumentChange> changes;
    std::vector<SecondaryPageLoadReport> loadReports;
    int visibilityChangedCount = 0;
    KiriView::ImageSecondaryPageController controller;
};
}

class TestImageSecondaryPageController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void visiblePredecodedPageOwnsDisplayedState();
    void widePredecodedPageReportsPrimaryOnlyWithoutVisibleState();
    void clearRemovesDisplayedState();
};

void TestImageSecondaryPageController::visiblePredecodedPageOwnsDisplayedState()
{
    SecondaryPageFixture fixture;
    const QUrl pageUrl = localUrl(QStringLiteral("/book/002.png"));
    const QSize pageSize(800, 1200);
    fixture.addPredecodedPage(pageUrl, pageSize);

    fixture.startLoad(pageUrl);

    QVERIFY(fixture.controller.visible());
    QCOMPARE(fixture.controller.displayedImageLocation().imageUrl(), pageUrl);
    QCOMPARE(fixture.controller.imageSize(), pageSize);
    QVERIFY(fixture.controller.renderSnapshot().isRenderable());
    QCOMPARE(static_cast<int>(fixture.loadReports.size()), 1);
    QCOMPARE(fixture.loadReports.back().result, KiriView::ImageSecondaryPageLoadResult::Visible);
    QCOMPARE(fixture.loadReports.back().location.imageUrl(), pageUrl);
    QCOMPARE(fixture.loadReports.back().imageSize, pageSize);
}

void TestImageSecondaryPageController::widePredecodedPageReportsPrimaryOnlyWithoutVisibleState()
{
    SecondaryPageFixture fixture;
    const QUrl pageUrl = localUrl(QStringLiteral("/book/003.png"));
    const QSize pageSize(1200, 800);
    fixture.addPredecodedPage(pageUrl, pageSize);

    fixture.startLoad(pageUrl);

    QVERIFY(!fixture.controller.visible());
    QVERIFY(fixture.controller.displayedImageLocation().isEmpty());
    QCOMPARE(fixture.controller.imageSize(), QSize());
    QVERIFY(!fixture.controller.renderSnapshot().isRenderable());
    QCOMPARE(static_cast<int>(fixture.loadReports.size()), 1);
    QCOMPARE(
        fixture.loadReports.back().result, KiriView::ImageSecondaryPageLoadResult::PrimaryOnly);
    QCOMPARE(fixture.loadReports.back().location.imageUrl(), pageUrl);
    QCOMPARE(fixture.loadReports.back().imageSize, pageSize);
}

void TestImageSecondaryPageController::clearRemovesDisplayedState()
{
    SecondaryPageFixture fixture;
    const QUrl pageUrl = localUrl(QStringLiteral("/book/004.png"));
    fixture.addPredecodedPage(pageUrl, QSize(800, 1200));

    fixture.startLoad(pageUrl);
    fixture.controller.clear();

    QVERIFY(!fixture.controller.visible());
    QVERIFY(fixture.controller.displayedImageLocation().isEmpty());
    QCOMPARE(fixture.controller.imageSize(), QSize());
    QVERIFY(!fixture.controller.renderSnapshot().isRenderable());
}

QTEST_GUILESS_MAIN(TestImageSecondaryPageController)

#include "test_imagesecondarypagecontroller.moc"

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
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::testImage;

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

kiriview::DisplayedImageLocation displayedLocation(const QUrl &url)
{
    return kiriview::DisplayedImageLocation::fromUrl(url);
}

struct SecondaryPageLoadReport {
    kiriview::ImageSecondaryPageLoadResult result = kiriview::ImageSecondaryPageLoadResult::Failed;
    kiriview::DisplayedImageLocation location;
    QSize imageSize;
};

class SecondaryPageFixture
{
public:
    SecondaryPageFixture()
        : controller(&context, renderContext,
              kiriview::ImageSecondaryPageController::Callbacks {
                  [this](kiriview::ImageDocumentChange change) { changes.push_back(change); },
                  [this](kiriview::ImageSecondaryPageLoadResult result,
                      const kiriview::DisplayedImageLocation &location, const QSize &imageSize) {
                      loadReports.push_back(
                          SecondaryPageLoadReport { result, location, imageSize });
                  },
                  [this]() { ++visibilityChangedCount; },
                  [this](const QUrl &url) { return findPredecodedImage(url); },
              },
              {}, {}, testCacheBudgets())
    {
    }

    void addPredecodedPage(const QUrl &url, const QSize &imageSize)
    {
        predecodedImageSizes[url] = imageSize;
    }

    void startLoad(const QUrl &url)
    {
        controller.startLoad(url, kiriview::OpenedCollectionScopeLocation::none(), {});
    }

    std::optional<kiriview::PredecodedImage> findPredecodedImage(const QUrl &url) const
    {
        const auto imageSize = predecodedImageSizes.find(url);
        if (imageSize == predecodedImageSizes.cend()) {
            return std::nullopt;
        }

        return kiriview::PredecodedImage {
            staticDisplayTestImagePayload(testImage(imageSize->second)),
            displayedLocation(url),
        };
    }

    QObject context;
    std::map<QUrl, QSize> predecodedImageSizes;
    std::vector<kiriview::ImageDocumentChange> changes;
    std::vector<SecondaryPageLoadReport> loadReports;
    int visibilityChangedCount = 0;
    kiriview::ImageSecondaryPageController controller;
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
    QVERIFY(fixture.controller.pageSlotSnapshot().hasImage);
    QCOMPARE(static_cast<int>(fixture.loadReports.size()), 1);
    QCOMPARE(fixture.loadReports.back().result, kiriview::ImageSecondaryPageLoadResult::Visible);
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
    QVERIFY(!fixture.controller.pageSlotSnapshot().hasImage);
    QCOMPARE(static_cast<int>(fixture.loadReports.size()), 1);
    QCOMPARE(
        fixture.loadReports.back().result, kiriview::ImageSecondaryPageLoadResult::PrimaryOnly);
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
    QVERIFY(!fixture.controller.pageSlotSnapshot().hasImage);
}

QTEST_GUILESS_MAIN(TestImageSecondaryPageController)

#include "test_imagesecondarypagecontroller.moc"

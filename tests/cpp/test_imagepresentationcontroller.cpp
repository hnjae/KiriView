// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationcontroller.h"

#include "image_test_support.h"
#include "rendering/imagerendering.h"
#include "rendering/svgtilesource.h"

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

bool containsChange(
    const std::vector<KiriView::ImageDocumentChange> &changes, KiriView::ImageDocumentChange change)
{
    return std::find(changes.cbegin(), changes.cend(), change) != changes.cend();
}

QByteArray smallSvgData()
{
    return QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\">"
                             "<path d=\"M4 4h24v24H4z\" fill=\"red\"/>"
                             "</svg>");
}

KiriView::ImageCacheBudgets testCacheBudgets()
{
    return KiriView::ImageCacheBudgets {
        1024 * 1024,
        4096,
    };
}
}

class TestImagePresentationController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedImageChangesSynchronizeViewportThroughController();
    void smallSvgUsesStaticTileSurfaceInsteadOfLegacyFrameSurface();
};

void TestImagePresentationController::displayedImageChangesSynchronizeViewportThroughController()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImagePresentationController controller(
        this,
        []() {
            return KiriView::ImageDocumentRenderContext {
                1.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        KiriView::ImagePresentationController::Callbacks {
            [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); },
            {},
        },
        testCacheBudgets());
    controller.setViewportSize(QSizeF(200.0, 100.0));

    changes.clear();
    controller.setStaticImage(staticTestImagePayload(testImage(QSize(80, 40))), true);

    QVERIFY(controller.hasImage());
    QCOMPARE(controller.imageRevision(), quint64(1));
    QCOMPARE(controller.imageSize(), QSize(80, 40));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::ImageSize));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::DisplaySize));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Repaint));

    changes.clear();
    controller.clearImage();

    QVERIFY(!controller.hasImage());
    QCOMPARE(controller.imageRevision(), quint64(2));
    QCOMPARE(controller.imageSize(), QSize());
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::ImageSize));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::DisplaySize));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Repaint));

    changes.clear();
    controller.clearImage();

    QVERIFY(changes.empty());
    QCOMPARE(controller.imageRevision(), quint64(2));
}

void TestImagePresentationController::smallSvgUsesStaticTileSurfaceInsteadOfLegacyFrameSurface()
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(smallSvgData(), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const QImage preview = source->decodeBlockingDisplayImage(32, &errorString);
    QVERIFY2(!preview.isNull(), qPrintable(errorString));
    QCOMPARE(preview.size(), source->imageSize());

    KiriView::ImagePresentationController controller(
        this,
        []() {
            return KiriView::ImageDocumentRenderContext {
                1.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        {}, testCacheBudgets());
    controller.setViewportSize(QSizeF(32.0, 32.0));
    controller.setStaticImage(
        KiriView::StaticImagePayload { std::move(source), preview, {} }, true);

    const std::shared_ptr<KiriView::DisplayedImageSurface> surface = controller.imageSurface();
    QVERIFY(surface != nullptr);
    QVERIFY(surface->staticTileSurface() != nullptr);
    QVERIFY(surface->legacyFrameSurface() == nullptr);
    QCOMPARE(surface->staticTileSurface()->tileCacheByteBudget(), qsizetype(4096));
}

QTEST_GUILESS_MAIN(TestImagePresentationController)

#include "test_imagepresentationcontroller.moc"

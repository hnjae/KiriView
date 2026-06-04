// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepagesurfacecontroller.h"

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

KiriView::ImageDocumentRenderContext renderContext()
{
    return KiriView::ImageDocumentRenderContext {
        1.0,
        KiriView::fallbackTextureSizeMax,
    };
}
}

class TestImagePageSurfaceController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedImageChangesUpdateSurfaceResources();
    void smallSvgUsesStaticTileSurfaceInsteadOfLegacyFrameSurface();
};

void TestImagePageSurfaceController::displayedImageChangesUpdateSurfaceResources()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImagePageSurfaceController controller(this,
        KiriView::ImagePageSurfaceController::Callbacks {
            [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); },
            {},
        },
        testCacheBudgets());

    changes.clear();
    controller.setStaticImage(
        staticTestImagePayload(testImage(QSize(80, 40))), true, renderContext());

    QVERIFY(controller.hasImage());
    QCOMPARE(controller.imageRevision(), quint64(1));
    QCOMPARE(controller.imageSize(), QSize(80, 40));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Repaint));

    changes.clear();
    controller.clearImage();

    QVERIFY(!controller.hasImage());
    QCOMPARE(controller.imageRevision(), quint64(2));
    QCOMPARE(controller.imageSize(), QSize());
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Repaint));

    changes.clear();
    controller.clearImage();

    QVERIFY(changes.empty());
    QCOMPARE(controller.imageRevision(), quint64(2));
}

void TestImagePageSurfaceController::smallSvgUsesStaticTileSurfaceInsteadOfLegacyFrameSurface()
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(smallSvgData(), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const QImage preview = source->decodeBlockingDisplayImage(32, &errorString);
    QVERIFY2(!preview.isNull(), qPrintable(errorString));
    QCOMPARE(preview.size(), source->imageSize());

    KiriView::ImagePageSurfaceController controller(this, {}, testCacheBudgets());
    controller.setStaticImage(
        KiriView::StaticImagePayload { std::move(source), preview, {} }, true, renderContext());

    const std::shared_ptr<KiriView::DisplayedImageSurface> surface = controller.imageSurface();
    QVERIFY(surface != nullptr);
    QVERIFY(surface->staticTileSurface() != nullptr);
    QVERIFY(surface->legacyFrameSurface() == nullptr);
    QCOMPARE(surface->staticTileSurface()->tileCacheByteBudget(), qsizetype(4096));
}

QTEST_GUILESS_MAIN(TestImagePageSurfaceController)

#include "test_imagepresentationcontroller.moc"

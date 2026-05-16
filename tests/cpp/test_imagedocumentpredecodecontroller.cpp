// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentstate.h"
#include "imagepresentationcontroller.h"
#include "imagerendering.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::imagesDirectoryUrl;
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

KiriView::ImagePresentationController createPresentationController(QObject *parent)
{
    return KiriView::ImagePresentationController(parent,
        []() {
            return KiriView::ImageDocumentRenderContext {
                2.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        {});
}
}

class TestImageDocumentPredecodeController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scheduleAdjacentImagePredecodeUsesPresentationSnapshot();
    void scheduleAdjacentImagePredecodeWithoutSnapshotCancelsActivePredecode();
};

void TestImageDocumentPredecodeController::scheduleAdjacentImagePredecodeUsesPresentationSnapshot()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageDocumentState state;
    KiriView::ImagePresentationController presentation = createPresentationController(this);
    KiriView::ImageDocumentPredecodeController controller(this, state, presentation,
        candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()));

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    presentation.setViewportSize(QSizeF(320.0, 240.0));
    presentation.setPredecodeCacheable(true);
    presentation.setStaticImage(
        staticTestImagePayload(testImage(QSize(10, 8)), KiriView::StaticImageDisplayHints { 0.5 }));

    controller.scheduleAdjacentImagePredecode();

    const std::optional<KiriView::PredecodedImage> displayed = controller.tryTake(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.5);

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(640, 480));
}

void TestImageDocumentPredecodeController::
    scheduleAdjacentImagePredecodeWithoutSnapshotCancelsActivePredecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageDocumentState state;
    KiriView::ImagePresentationController presentation = createPresentationController(this);
    KiriView::ImageDocumentPredecodeController controller(this, state, presentation,
        candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()));

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    presentation.setStaticImage(staticTestImagePayload(testImage()));
    controller.scheduleAdjacentImagePredecode();
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));

    presentation.clearImage();
    controller.scheduleAdjacentImagePredecode();

    QVERIFY(dataLoader.frontLoad().canceled);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QVERIFY(!controller.tryTake(nextUrl).has_value());
}

QTEST_GUILESS_MAIN(TestImageDocumentPredecodeController)

#include "test_imagedocumentpredecodecontroller.moc"

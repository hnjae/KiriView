// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationcontroller.h"

#include "image_test_support.h"
#include "imagerendering.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <algorithm>
#include <vector>

namespace {
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

bool containsChange(
    const std::vector<KiriView::ImageDocumentChange> &changes, KiriView::ImageDocumentChange change)
{
    return std::find(changes.cbegin(), changes.cend(), change) != changes.cend();
}
}

class TestImagePresentationController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedImageChangesSynchronizeViewportThroughController();
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
        });
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

QTEST_GUILESS_MAIN(TestImagePresentationController)

#include "test_imagepresentationcontroller.moc"

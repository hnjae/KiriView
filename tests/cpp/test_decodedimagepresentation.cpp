// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimagepresentation.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <Qt>

namespace {
KiriView::AnimationFrame frame()
{
    return KiriView::AnimationFrame { KiriView::TestSupport::testImage(), 10 };
}
}

class TestDecodedImagePresentation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staticImagesCarryCacheability();
    void decodedAnimationsRequireFrames();
    void streamedAnimationsUseFirstFrameTargets();
};

void TestDecodedImagePresentation::staticImagesCarryCacheability()
{
    KiriView::StaticDecodedImage decoded {
        KiriView::TestSupport::staticTestImagePayload(KiriView::TestSupport::testImage()),
    };

    KiriView::DecodedImagePresentationPlan plan = KiriView::decodedImagePresentationPlan(decoded);
    QCOMPARE(static_cast<int>(plan.target),
        static_cast<int>(KiriView::DecodedImagePresentationTarget::StaticImage));
    QVERIFY(plan.predecodeCacheable);

    decoded.staticImage = KiriView::StaticImagePayload {};
    plan = KiriView::decodedImagePresentationPlan(decoded);
    QCOMPARE(static_cast<int>(plan.target),
        static_cast<int>(KiriView::DecodedImagePresentationTarget::StaticImage));
    QVERIFY(!plan.predecodeCacheable);
}

void TestDecodedImagePresentation::decodedAnimationsRequireFrames()
{
    KiriView::DecodedAnimationImage decodedAnimation;

    KiriView::DecodedImagePresentationPlan plan
        = KiriView::decodedImagePresentationPlan(decodedAnimation);
    QCOMPARE(static_cast<int>(plan.target),
        static_cast<int>(KiriView::DecodedImagePresentationTarget::DecodeError));

    decodedAnimation.frames.push_back(frame());
    plan = KiriView::decodedImagePresentationPlan(decodedAnimation);
    QCOMPARE(static_cast<int>(plan.target),
        static_cast<int>(KiriView::DecodedImagePresentationTarget::DecodedAnimation));
    QVERIFY(!plan.predecodeCacheable);
}

void TestDecodedImagePresentation::streamedAnimationsUseFirstFrameTargets()
{
    const KiriView::ReaderAnimationImage readerAnimation { KiriView::TestSupport::testImage() };
    KiriView::DecodedImagePresentationPlan plan
        = KiriView::decodedImagePresentationPlan(readerAnimation);
    QCOMPARE(static_cast<int>(plan.target),
        static_cast<int>(KiriView::DecodedImagePresentationTarget::ReaderAnimation));
    QVERIFY(!plan.predecodeCacheable);

    const KiriView::HeifSequenceAnimationImage heifSequence { KiriView::TestSupport::testImage() };
    plan = KiriView::decodedImagePresentationPlan(heifSequence);
    QCOMPARE(static_cast<int>(plan.target),
        static_cast<int>(KiriView::DecodedImagePresentationTarget::HeifSequenceAnimation));
    QVERIFY(!plan.predecodeCacheable);
}

QTEST_GUILESS_MAIN(TestDecodedImagePresentation)

#include "test_decodedimagepresentation.moc"

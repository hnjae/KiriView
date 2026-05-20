// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumenteffectplan.h"

#include "image_document_plan_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>

namespace {
using KiriView::ImageDocumentEffect;
using KiriView::ImageDocumentRuntimePlan;
using KiriView::TestSupport::hasOperationTypes;
using KiriView::TestSupport::operationAt;
using KiriView::TestSupport::operationTypes;

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }
}

class TestImageDocumentEffectPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clearImagePlansOrderedRuntimeOperations();
    void clearDeletedImagePlansDeletionClearAndEmptySourceCompletion();
    void shutdownPlansOrderedRuntimeOperations();
    void payloadEffectsPlanRuntimeOperations();
    void operationTypeAssertionsCoverEveryRuntimeOperation();
};

void TestImageDocumentEffectPlan::clearImagePlansOrderedRuntimeOperations()
{
    const ImageDocumentRuntimePlan plan
        = KiriView::imageDocumentEffectPlan(ImageDocumentEffect::clearImage());

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::ClearArchiveSessionOperation, KiriView::ClearPredecodeOperation,
            KiriView::FinishSpreadTransitionOperation, KiriView::ClearSecondaryPageOperation,
            KiriView::CancelPageNavigationUpdateOperation,
            KiriView::ClearDisplayedImageLocationOperation,
            KiriView::ClearPresentationImageOperation, KiriView::ClearPageNavigationOperation,
            KiriView::NotifyRightToLeftReadingChangedOperation>()));
}

void TestImageDocumentEffectPlan::clearDeletedImagePlansDeletionClearAndEmptySourceCompletion()
{
    const ImageDocumentRuntimePlan plan
        = KiriView::imageDocumentEffectPlan(ImageDocumentEffect::clearDeletedImage());

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::ClearArchiveSessionOperation,
            KiriView::CancelAllNavigationOperation, KiriView::CancelPredecodeOperation,
            KiriView::CancelOpenOperation, KiriView::FinishSpreadTransitionOperation,
            KiriView::ClearSecondaryPageOperation, KiriView::SetSourceUrlOperation,
            KiriView::SetErrorStringOperation, KiriView::FinishEmptySourceLoadOperation>()));
    QVERIFY(operationAt<KiriView::SetSourceUrlOperation>(plan, 6).url.isEmpty());
    QVERIFY(operationAt<KiriView::SetErrorStringOperation>(plan, 7).errorString.isEmpty());
}

void TestImageDocumentEffectPlan::shutdownPlansOrderedRuntimeOperations()
{
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentShutdownPlan();

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::StopPresentationAnimationOperation, KiriView::ShutdownSpreadOperation,
            KiriView::CancelPredecodeOperation, KiriView::CancelAllNavigationOperation,
            KiriView::CancelOpenOperation, KiriView::ClearArchiveSessionOperation>()));
}

void TestImageDocumentEffectPlan::payloadEffectsPlanRuntimeOperations()
{
    ImageDocumentRuntimePlan plan = KiriView::imageDocumentEffectPlan(
        ImageDocumentEffect::openUrl(localUrl(QStringLiteral("/image.png"))));
    QCOMPARE(operationAt<KiriView::LoadUrlOperation>(plan, 0).url,
        localUrl(QStringLiteral("/image.png")));

    plan = KiriView::imageDocumentEffectPlan(ImageDocumentEffect::containerImageSelected(
        localUrl(QStringLiteral("/book/01.png")), localUrl(QStringLiteral("/book.cbz"))));
    QCOMPARE(operationAt<KiriView::LoadContainerImageOperation>(plan, 0).imageUrl,
        localUrl(QStringLiteral("/book/01.png")));
    QCOMPARE(operationAt<KiriView::LoadContainerImageOperation>(plan, 0).containerUrl,
        localUrl(QStringLiteral("/book.cbz")));

    plan = KiriView::imageDocumentEffectPlan(
        ImageDocumentEffect::emptyContainerSelected(localUrl(QStringLiteral("/empty.cbz"))));
    QCOMPARE(operationAt<KiriView::FinishEmptyContainerNavigationOperation>(plan, 0).containerUrl,
        localUrl(QStringLiteral("/empty.cbz")));

    plan = KiriView::imageDocumentEffectPlan(ImageDocumentEffect::containerNavigationFailed(
        localUrl(QStringLiteral("/broken.cbz")), QStringLiteral("broken")));
    QCOMPARE(operationAt<KiriView::FinishContainerNavigationLoadWithErrorOperation>(plan, 0)
                 .containerUrl,
        localUrl(QStringLiteral("/broken.cbz")));
    QCOMPARE(
        operationAt<KiriView::FinishContainerNavigationLoadWithErrorOperation>(plan, 0).errorString,
        QStringLiteral("broken"));

    plan = KiriView::imageDocumentEffectPlan(
        ImageDocumentEffect::pageNavigationSelected(localUrl(QStringLiteral("/next.png")), true));
    QCOMPARE(operationAt<KiriView::LoadPageNavigationUrlOperation>(plan, 0).url,
        localUrl(QStringLiteral("/next.png")));
    QVERIFY(operationAt<KiriView::LoadPageNavigationUrlOperation>(plan, 0)
            .preserveTwoPageSpreadTransition);

    plan = KiriView::imageDocumentEffectPlan(
        ImageDocumentEffect::prepareFailedContainer(localUrl(QStringLiteral("/bad.zip"))));
    QCOMPARE(operationAt<KiriView::PrepareFailedContainerOperation>(plan, 0).containerUrl,
        localUrl(QStringLiteral("/bad.zip")));
}

void TestImageDocumentEffectPlan::operationTypeAssertionsCoverEveryRuntimeOperation()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/book.cbz"));
    const ImageDocumentRuntimePlan plan {
        KiriView::CancelFileDeletionOperation {},
        KiriView::StopPresentationAnimationOperation {},
        KiriView::ShutdownSpreadOperation {},
        KiriView::ClearArchiveSessionOperation {},
        KiriView::ClearPredecodeOperation {},
        KiriView::CancelPredecodeOperation {},
        KiriView::ScheduleAdjacentImagePredecodeOperation {},
        KiriView::FinishSpreadTransitionOperation {},
        KiriView::ResetRightToLeftReadingOperation {},
        KiriView::ClearSecondaryPageOperation {},
        KiriView::NotifyRightToLeftReadingChangedOperation {},
        KiriView::ResetZoomOperation {},
        KiriView::PrepareFailedContainerOperation { containerUrl },
        KiriView::CancelPageNavigationUpdateOperation {},
        KiriView::CancelNavigationOperation {},
        KiriView::CancelContainerNavigationOperation {},
        KiriView::CancelAllNavigationOperation {},
        KiriView::ClearPageNavigationOperation {},
        KiriView::UpdatePageNavigationOperation {},
        KiriView::LoadUrlOperation { sourceUrl },
        KiriView::LoadContainerImageOperation { sourceUrl, containerUrl },
        KiriView::FinishEmptyContainerNavigationOperation { containerUrl },
        KiriView::FinishContainerNavigationLoadWithErrorOperation {
            containerUrl,
            QStringLiteral("broken"),
        },
        KiriView::LoadPageNavigationUrlOperation { sourceUrl, true },
        KiriView::CancelOpenOperation {},
        KiriView::ClearDisplayedImageLocationOperation {},
        KiriView::ClearPresentationImageOperation {},
        KiriView::ClearLoadingContainerNavigationUrlOperation {},
        KiriView::SetLoadingContainerNavigationUrlOperation { containerUrl },
        KiriView::SetContainerNavigationUrlOperation { containerUrl },
        KiriView::PrepareSourceLoadOperation {
            KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl),
        },
        KiriView::SetSourceUrlOperation { sourceUrl },
        KiriView::BeginOpenOperation {},
        KiriView::SetErrorStringOperation { QStringLiteral("failed") },
        KiriView::FinishEmptySourceLoadOperation {},
    };

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::StopPresentationAnimationOperation, KiriView::ShutdownSpreadOperation,
            KiriView::ClearArchiveSessionOperation, KiriView::ClearPredecodeOperation,
            KiriView::CancelPredecodeOperation, KiriView::ScheduleAdjacentImagePredecodeOperation,
            KiriView::FinishSpreadTransitionOperation, KiriView::ResetRightToLeftReadingOperation,
            KiriView::ClearSecondaryPageOperation,
            KiriView::NotifyRightToLeftReadingChangedOperation, KiriView::ResetZoomOperation,
            KiriView::PrepareFailedContainerOperation,
            KiriView::CancelPageNavigationUpdateOperation, KiriView::CancelNavigationOperation,
            KiriView::CancelContainerNavigationOperation, KiriView::CancelAllNavigationOperation,
            KiriView::ClearPageNavigationOperation, KiriView::UpdatePageNavigationOperation,
            KiriView::LoadUrlOperation, KiriView::LoadContainerImageOperation,
            KiriView::FinishEmptyContainerNavigationOperation,
            KiriView::FinishContainerNavigationLoadWithErrorOperation,
            KiriView::LoadPageNavigationUrlOperation, KiriView::CancelOpenOperation,
            KiriView::ClearDisplayedImageLocationOperation,
            KiriView::ClearPresentationImageOperation,
            KiriView::ClearLoadingContainerNavigationUrlOperation,
            KiriView::SetLoadingContainerNavigationUrlOperation,
            KiriView::SetContainerNavigationUrlOperation, KiriView::PrepareSourceLoadOperation,
            KiriView::SetSourceUrlOperation, KiriView::BeginOpenOperation,
            KiriView::SetErrorStringOperation, KiriView::FinishEmptySourceLoadOperation>()));
}

QTEST_GUILESS_MAIN(TestImageDocumentEffectPlan)

#include "test_imagedocumenteffectplan.moc"

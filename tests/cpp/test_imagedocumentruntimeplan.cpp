// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntimeplan.h"

#include "image_document_plan_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>

namespace {
using KiriView::ImageDocumentRuntimePlan;
using KiriView::TestSupport::hasOperationTypes;
using KiriView::TestSupport::operationAt;
using KiriView::TestSupport::operationTypes;

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }
}

class TestImageDocumentRuntimePlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clearImagePlansOrderedRuntimeOperations();
    void clearDeletedImagePlansDeletionClearAndEmptySourceCompletion();
    void shutdownPlansOrderedRuntimeOperations();
    void payloadOperationsCarryRuntimeData();
    void operationTypeAssertionsCoverEveryRuntimeOperation();
};

void TestImageDocumentRuntimePlan::clearImagePlansOrderedRuntimeOperations()
{
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentClearImagePlan();

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::ClearArchiveSessionOperation, KiriView::ClearPredecodeOperation,
            KiriView::FinishSpreadTransitionOperation, KiriView::ClearSecondaryPageOperation,
            KiriView::CancelPageNavigationUpdateOperation,
            KiriView::ClearDisplayedImageLocationOperation,
            KiriView::ClearPresentationImageOperation, KiriView::ClearPageNavigationOperation,
            KiriView::NotifyRightToLeftReadingChangedOperation>()));
}

void TestImageDocumentRuntimePlan::clearDeletedImagePlansDeletionClearAndEmptySourceCompletion()
{
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentClearDeletedImagePlan();

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::ClearArchiveSessionOperation,
            KiriView::CancelAllNavigationOperation, KiriView::CancelPredecodeOperation,
            KiriView::CancelOpenOperation, KiriView::FinishSpreadTransitionOperation,
            KiriView::ClearSecondaryPageOperation, KiriView::SetSourceUrlOperation,
            KiriView::SetErrorStringOperation, KiriView::FinishEmptySourceLoadOperation>()));
    QVERIFY(operationAt<KiriView::SetSourceUrlOperation>(plan, 6).target.url.isEmpty());
    QVERIFY(operationAt<KiriView::SetErrorStringOperation>(plan, 7).errorString.isEmpty());
}

void TestImageDocumentRuntimePlan::shutdownPlansOrderedRuntimeOperations()
{
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentShutdownPlan();

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::StopPresentationAnimationOperation, KiriView::ShutdownSpreadOperation,
            KiriView::CancelPredecodeOperation, KiriView::CancelAllNavigationOperation,
            KiriView::CancelOpenOperation, KiriView::ClearArchiveSessionOperation>()));
}

void TestImageDocumentRuntimePlan::payloadOperationsCarryRuntimeData()
{
    ImageDocumentRuntimePlan plan { KiriView::LoadUrlOperation {
        KiriView::ImageNavigationTarget {
            localUrl(QStringLiteral("/image.png")),
            KiriView::ImageNavigationCandidateKind::Video,
        },
    } };
    QCOMPARE(operationAt<KiriView::LoadUrlOperation>(plan, 0).target.url,
        localUrl(QStringLiteral("/image.png")));
    QCOMPARE(operationAt<KiriView::LoadUrlOperation>(plan, 0).target.kind,
        KiriView::ImageNavigationCandidateKind::Video);

    plan = ImageDocumentRuntimePlan { KiriView::LoadContainerImageOperation {
        KiriView::ImageNavigationTarget {
            localUrl(QStringLiteral("/book/01.png")),
            KiriView::ImageNavigationCandidateKind::Video,
        },
        localUrl(QStringLiteral("/book.cbz")),
    } };
    QCOMPARE(operationAt<KiriView::LoadContainerImageOperation>(plan, 0).target.url,
        localUrl(QStringLiteral("/book/01.png")));
    QCOMPARE(operationAt<KiriView::LoadContainerImageOperation>(plan, 0).target.kind,
        KiriView::ImageNavigationCandidateKind::Video);
    QCOMPARE(operationAt<KiriView::LoadContainerImageOperation>(plan, 0).containerUrl,
        localUrl(QStringLiteral("/book.cbz")));

    plan = ImageDocumentRuntimePlan { KiriView::FinishEmptyContainerNavigationOperation {
        localUrl(QStringLiteral("/empty.cbz")),
    } };
    QCOMPARE(operationAt<KiriView::FinishEmptyContainerNavigationOperation>(plan, 0).containerUrl,
        localUrl(QStringLiteral("/empty.cbz")));

    plan = ImageDocumentRuntimePlan { KiriView::FinishContainerNavigationLoadWithErrorOperation {
        localUrl(QStringLiteral("/broken.cbz")),
        QStringLiteral("broken"),
    } };
    QCOMPARE(operationAt<KiriView::FinishContainerNavigationLoadWithErrorOperation>(plan, 0)
                 .containerUrl,
        localUrl(QStringLiteral("/broken.cbz")));
    QCOMPARE(
        operationAt<KiriView::FinishContainerNavigationLoadWithErrorOperation>(plan, 0).errorString,
        QStringLiteral("broken"));

    plan = ImageDocumentRuntimePlan { KiriView::LoadPageNavigationUrlOperation {
        KiriView::ImageNavigationTarget {
            localUrl(QStringLiteral("/next.png")),
            KiriView::ImageNavigationCandidateKind::Video,
        },
        true,
    } };
    QCOMPARE(operationAt<KiriView::LoadPageNavigationUrlOperation>(plan, 0).target.url,
        localUrl(QStringLiteral("/next.png")));
    QCOMPARE(operationAt<KiriView::LoadPageNavigationUrlOperation>(plan, 0).target.kind,
        KiriView::ImageNavigationCandidateKind::Video);
    QVERIFY(operationAt<KiriView::LoadPageNavigationUrlOperation>(plan, 0)
            .preserveTwoPageSpreadTransition);

    plan = ImageDocumentRuntimePlan { KiriView::PrepareFailedContainerOperation {
        localUrl(QStringLiteral("/bad.zip")),
    } };
    QCOMPARE(operationAt<KiriView::PrepareFailedContainerOperation>(plan, 0).containerUrl,
        localUrl(QStringLiteral("/bad.zip")));
}

void TestImageDocumentRuntimePlan::operationTypeAssertionsCoverEveryRuntimeOperation()
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
        KiriView::LoadUrlOperation { KiriView::ImageNavigationTarget {
            sourceUrl,
            KiriView::ImageNavigationCandidateKind::Image,
        } },
        KiriView::LoadContainerImageOperation {
            KiriView::ImageNavigationTarget {
                sourceUrl,
                KiriView::ImageNavigationCandidateKind::Image,
            },
            containerUrl,
        },
        KiriView::FinishEmptyContainerNavigationOperation { containerUrl },
        KiriView::FinishContainerNavigationLoadWithErrorOperation {
            containerUrl,
            QStringLiteral("broken"),
        },
        KiriView::LoadPageNavigationUrlOperation {
            KiriView::ImageNavigationTarget {
                sourceUrl,
                KiriView::ImageNavigationCandidateKind::Image,
            },
            true,
        },
        KiriView::CancelOpenOperation {},
        KiriView::ClearDisplayedImageLocationOperation {},
        KiriView::ClearPresentationImageOperation {},
        KiriView::ClearLoadingContainerNavigationUrlOperation {},
        KiriView::SetLoadingContainerNavigationUrlOperation { containerUrl },
        KiriView::SetContainerNavigationUrlOperation { containerUrl },
        KiriView::PrepareSourceLoadOperation {
            KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl),
        },
        KiriView::SetSourceUrlOperation {
            KiriView::ImageNavigationTarget {
                sourceUrl,
                KiriView::ImageNavigationCandidateKind::Image,
            },
        },
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

QTEST_GUILESS_MAIN(TestImageDocumentRuntimePlan)

#include "test_imagedocumentruntimeplan.moc"

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntimeplan.h"

#include "image_document_plan_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>

namespace {
using kiriview::ImageDocumentRuntimePlan;
using kiriview::TestSupport::hasOperationTypes;
using kiriview::TestSupport::operationAt;
using kiriview::TestSupport::operationTypes;

QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }
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
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentClearImagePlan();

    QVERIFY(hasOperationTypes(plan,
        operationTypes<kiriview::ClearMediaEntrySourceOperation, kiriview::ClearPredecodeOperation,
            kiriview::FinishSpreadTransitionOperation, kiriview::ClearSecondaryPageOperation,
            kiriview::CancelPageNavigationUpdateOperation,
            kiriview::ClearDisplayedImageLocationOperation,
            kiriview::ClearPresentationImageOperation, kiriview::ClearPageNavigationOperation,
            kiriview::NotifyRightToLeftReadingChangedOperation>()));
}

void TestImageDocumentRuntimePlan::clearDeletedImagePlansDeletionClearAndEmptySourceCompletion()
{
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentClearDeletedImagePlan();

    QVERIFY(hasOperationTypes(plan,
        operationTypes<kiriview::ClearMediaEntrySourceOperation,
            kiriview::CancelAllNavigationOperation, kiriview::CancelPredecodeOperation,
            kiriview::CancelOpenOperation, kiriview::FinishSpreadTransitionOperation,
            kiriview::ClearSecondaryPageOperation, kiriview::SetSourceUrlOperation,
            kiriview::SetErrorStringOperation, kiriview::FinishEmptySourceLoadOperation>()));
    QVERIFY(operationAt<kiriview::SetSourceUrlOperation>(plan, 6).target.url.isEmpty());
    QVERIFY(operationAt<kiriview::SetErrorStringOperation>(plan, 7).errorString.isEmpty());
}

void TestImageDocumentRuntimePlan::shutdownPlansOrderedRuntimeOperations()
{
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentShutdownPlan();

    QVERIFY(hasOperationTypes(plan,
        operationTypes<kiriview::CancelFileDeletionOperation,
            kiriview::StopPresentationAnimationOperation, kiriview::ShutdownSpreadOperation,
            kiriview::CancelPredecodeOperation, kiriview::CancelAllNavigationOperation,
            kiriview::CancelOpenOperation, kiriview::ClearMediaEntrySourceOperation>()));
}

void TestImageDocumentRuntimePlan::payloadOperationsCarryRuntimeData()
{
    ImageDocumentRuntimePlan plan { kiriview::LoadUrlOperation {
        kiriview::ImageDocumentPageTarget {
            localUrl(QStringLiteral("/image.png")),
            kiriview::ImageDocumentPageKind::Video,
        },
    } };
    QCOMPARE(operationAt<kiriview::LoadUrlOperation>(plan, 0).target.url,
        localUrl(QStringLiteral("/image.png")));
    QCOMPARE(operationAt<kiriview::LoadUrlOperation>(plan, 0).target.kind,
        kiriview::ImageDocumentPageKind::Video);

    plan = ImageDocumentRuntimePlan { kiriview::LoadContainerImageOperation {
        kiriview::ImageDocumentPageTarget {
            localUrl(QStringLiteral("/book/01.png")),
            kiriview::ImageDocumentPageKind::Video,
        },
        localUrl(QStringLiteral("/book.cbz")),
    } };
    QCOMPARE(operationAt<kiriview::LoadContainerImageOperation>(plan, 0).target.url,
        localUrl(QStringLiteral("/book/01.png")));
    QCOMPARE(operationAt<kiriview::LoadContainerImageOperation>(plan, 0).target.kind,
        kiriview::ImageDocumentPageKind::Video);
    QCOMPARE(operationAt<kiriview::LoadContainerImageOperation>(plan, 0).containerUrl,
        localUrl(QStringLiteral("/book.cbz")));

    plan = ImageDocumentRuntimePlan { kiriview::FinishEmptyContainerNavigationOperation {
        localUrl(QStringLiteral("/empty.cbz")),
    } };
    QCOMPARE(operationAt<kiriview::FinishEmptyContainerNavigationOperation>(plan, 0).containerUrl,
        localUrl(QStringLiteral("/empty.cbz")));

    plan = ImageDocumentRuntimePlan { kiriview::FinishContainerNavigationLoadWithErrorOperation {
        localUrl(QStringLiteral("/broken.cbz")),
        QStringLiteral("broken"),
    } };
    QCOMPARE(operationAt<kiriview::FinishContainerNavigationLoadWithErrorOperation>(plan, 0)
                 .containerUrl,
        localUrl(QStringLiteral("/broken.cbz")));
    QCOMPARE(
        operationAt<kiriview::FinishContainerNavigationLoadWithErrorOperation>(plan, 0).errorString,
        QStringLiteral("broken"));

    plan = ImageDocumentRuntimePlan { kiriview::LoadPageNavigationUrlOperation {
        kiriview::ImageDocumentPageTarget {
            localUrl(QStringLiteral("/next.png")),
            kiriview::ImageDocumentPageKind::Video,
        },
        true,
    } };
    QCOMPARE(operationAt<kiriview::LoadPageNavigationUrlOperation>(plan, 0).target.url,
        localUrl(QStringLiteral("/next.png")));
    QCOMPARE(operationAt<kiriview::LoadPageNavigationUrlOperation>(plan, 0).target.kind,
        kiriview::ImageDocumentPageKind::Video);
    QVERIFY(operationAt<kiriview::LoadPageNavigationUrlOperation>(plan, 0)
            .preserveTwoPageSpreadTransition);

    plan = ImageDocumentRuntimePlan { kiriview::PrepareFailedContainerOperation {
        localUrl(QStringLiteral("/bad.zip")),
    } };
    QCOMPARE(operationAt<kiriview::PrepareFailedContainerOperation>(plan, 0).containerUrl,
        localUrl(QStringLiteral("/bad.zip")));
}

void TestImageDocumentRuntimePlan::operationTypeAssertionsCoverEveryRuntimeOperation()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/book.cbz"));
    const ImageDocumentRuntimePlan plan {
        kiriview::CancelFileDeletionOperation {},
        kiriview::StopPresentationAnimationOperation {},
        kiriview::ShutdownSpreadOperation {},
        kiriview::ClearMediaEntrySourceOperation {},
        kiriview::ClearPredecodeOperation {},
        kiriview::CancelPredecodeOperation {},
        kiriview::ScheduleAdjacentImagePredecodeOperation {},
        kiriview::FinishSpreadTransitionOperation {},
        kiriview::ResetRightToLeftReadingOperation {},
        kiriview::ClearSecondaryPageOperation {},
        kiriview::NotifyRightToLeftReadingChangedOperation {},
        kiriview::ResetZoomOperation {},
        kiriview::PrepareFailedContainerOperation { containerUrl },
        kiriview::CancelPageNavigationUpdateOperation {},
        kiriview::CancelNavigationOperation {},
        kiriview::CancelContainerNavigationOperation {},
        kiriview::CancelAllNavigationOperation {},
        kiriview::ClearPageNavigationOperation {},
        kiriview::UpdatePageNavigationOperation {},
        kiriview::LoadUrlOperation { kiriview::ImageDocumentPageTarget {
            sourceUrl,
            kiriview::ImageDocumentPageKind::Image,
        } },
        kiriview::LoadContainerImageOperation {
            kiriview::ImageDocumentPageTarget {
                sourceUrl,
                kiriview::ImageDocumentPageKind::Image,
            },
            containerUrl,
        },
        kiriview::FinishEmptyContainerNavigationOperation { containerUrl },
        kiriview::FinishContainerNavigationLoadWithErrorOperation {
            containerUrl,
            QStringLiteral("broken"),
        },
        kiriview::LoadPageNavigationUrlOperation {
            kiriview::ImageDocumentPageTarget {
                sourceUrl,
                kiriview::ImageDocumentPageKind::Image,
            },
            true,
        },
        kiriview::CancelOpenOperation {},
        kiriview::ClearDisplayedImageLocationOperation {},
        kiriview::ClearPresentationImageOperation {},
        kiriview::ClearLoadingContainerNavigationUrlOperation {},
        kiriview::SetLoadingContainerNavigationUrlOperation { containerUrl },
        kiriview::SetContainerNavigationUrlOperation { containerUrl },
        kiriview::PrepareSourceLoadOperation {
            kiriview::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl),
        },
        kiriview::SetSourceUrlOperation {
            kiriview::ImageDocumentPageTarget {
                sourceUrl,
                kiriview::ImageDocumentPageKind::Image,
            },
        },
        kiriview::BeginOpenOperation {},
        kiriview::SetErrorStringOperation { QStringLiteral("failed") },
        kiriview::FinishEmptySourceLoadOperation {},
    };

    QVERIFY(hasOperationTypes(plan,
        operationTypes<kiriview::CancelFileDeletionOperation,
            kiriview::StopPresentationAnimationOperation, kiriview::ShutdownSpreadOperation,
            kiriview::ClearMediaEntrySourceOperation, kiriview::ClearPredecodeOperation,
            kiriview::CancelPredecodeOperation, kiriview::ScheduleAdjacentImagePredecodeOperation,
            kiriview::FinishSpreadTransitionOperation, kiriview::ResetRightToLeftReadingOperation,
            kiriview::ClearSecondaryPageOperation,
            kiriview::NotifyRightToLeftReadingChangedOperation, kiriview::ResetZoomOperation,
            kiriview::PrepareFailedContainerOperation,
            kiriview::CancelPageNavigationUpdateOperation, kiriview::CancelNavigationOperation,
            kiriview::CancelContainerNavigationOperation, kiriview::CancelAllNavigationOperation,
            kiriview::ClearPageNavigationOperation, kiriview::UpdatePageNavigationOperation,
            kiriview::LoadUrlOperation, kiriview::LoadContainerImageOperation,
            kiriview::FinishEmptyContainerNavigationOperation,
            kiriview::FinishContainerNavigationLoadWithErrorOperation,
            kiriview::LoadPageNavigationUrlOperation, kiriview::CancelOpenOperation,
            kiriview::ClearDisplayedImageLocationOperation,
            kiriview::ClearPresentationImageOperation,
            kiriview::ClearLoadingContainerNavigationUrlOperation,
            kiriview::SetLoadingContainerNavigationUrlOperation,
            kiriview::SetContainerNavigationUrlOperation, kiriview::PrepareSourceLoadOperation,
            kiriview::SetSourceUrlOperation, kiriview::BeginOpenOperation,
            kiriview::SetErrorStringOperation, kiriview::FinishEmptySourceLoadOperation>()));
}

QTEST_GUILESS_MAIN(TestImageDocumentRuntimePlan)

#include "test_imagedocumentruntimeplan.moc"

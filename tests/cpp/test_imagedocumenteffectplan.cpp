// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumenteffectplan.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <vector>

namespace {
using KiriView::ImageDocumentEffect;
using KiriView::ImageDocumentRuntimeOperation;
using KiriView::ImageDocumentRuntimeOperationKind;
using KiriView::ImageDocumentRuntimePlan;

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

std::vector<ImageDocumentRuntimeOperationKind> operationKinds(const ImageDocumentRuntimePlan &plan)
{
    std::vector<ImageDocumentRuntimeOperationKind> kinds;
    kinds.reserve(plan.size());
    for (const ImageDocumentRuntimeOperation &operation : plan) {
        kinds.push_back(KiriView::imageDocumentRuntimeOperationKind(operation));
    }
    return kinds;
}

bool hasOperationKinds(const ImageDocumentRuntimePlan &plan,
    const std::vector<ImageDocumentRuntimeOperationKind> &expected)
{
    return operationKinds(plan) == expected;
}

template <typename Operation>
const Operation &operationAt(const ImageDocumentRuntimePlan &plan, std::size_t index)
{
    return std::get<Operation>(plan.at(index));
}
}

class TestImageDocumentEffectPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clearImagePlansOrderedRuntimeOperations();
    void clearDeletedImagePlansDeletionClearAndEmptySourceCompletion();
    void shutdownPlansOrderedRuntimeOperations();
    void payloadEffectsPlanRuntimeOperations();
    void operationKindMapsEveryRuntimeOperationExplicitly();
};

void TestImageDocumentEffectPlan::clearImagePlansOrderedRuntimeOperations()
{
    const ImageDocumentRuntimePlan plan
        = KiriView::imageDocumentEffectPlan(ImageDocumentEffect::clearImage());

    QVERIFY(hasOperationKinds(plan,
        {
            ImageDocumentRuntimeOperationKind::ClearArchiveSession,
            ImageDocumentRuntimeOperationKind::ClearPredecode,
            ImageDocumentRuntimeOperationKind::FinishSpreadTransition,
            ImageDocumentRuntimeOperationKind::ClearSecondaryPage,
            ImageDocumentRuntimeOperationKind::CancelPageNavigationUpdate,
            ImageDocumentRuntimeOperationKind::ClearDisplayedImageLocation,
            ImageDocumentRuntimeOperationKind::ClearPresentationImage,
            ImageDocumentRuntimeOperationKind::ClearPageNavigation,
            ImageDocumentRuntimeOperationKind::NotifyRightToLeftReadingChanged,
        }));
}

void TestImageDocumentEffectPlan::clearDeletedImagePlansDeletionClearAndEmptySourceCompletion()
{
    const ImageDocumentRuntimePlan plan
        = KiriView::imageDocumentEffectPlan(ImageDocumentEffect::clearDeletedImage());

    QVERIFY(hasOperationKinds(plan,
        {
            ImageDocumentRuntimeOperationKind::ClearArchiveSession,
            ImageDocumentRuntimeOperationKind::CancelAllNavigation,
            ImageDocumentRuntimeOperationKind::CancelPredecode,
            ImageDocumentRuntimeOperationKind::CancelOpen,
            ImageDocumentRuntimeOperationKind::FinishSpreadTransition,
            ImageDocumentRuntimeOperationKind::ClearSecondaryPage,
            ImageDocumentRuntimeOperationKind::SetSourceUrl,
            ImageDocumentRuntimeOperationKind::SetErrorString,
            ImageDocumentRuntimeOperationKind::FinishEmptySourceLoad,
        }));
    QVERIFY(operationAt<KiriView::SetSourceUrlOperation>(plan, 6).url.isEmpty());
    QVERIFY(operationAt<KiriView::SetErrorStringOperation>(plan, 7).errorString.isEmpty());
}

void TestImageDocumentEffectPlan::shutdownPlansOrderedRuntimeOperations()
{
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentShutdownPlan();

    QVERIFY(hasOperationKinds(plan,
        {
            ImageDocumentRuntimeOperationKind::CancelFileDeletion,
            ImageDocumentRuntimeOperationKind::StopPresentationAnimation,
            ImageDocumentRuntimeOperationKind::ShutdownSpread,
            ImageDocumentRuntimeOperationKind::CancelPredecode,
            ImageDocumentRuntimeOperationKind::CancelAllNavigation,
            ImageDocumentRuntimeOperationKind::CancelOpen,
            ImageDocumentRuntimeOperationKind::ClearArchiveSession,
        }));
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

void TestImageDocumentEffectPlan::operationKindMapsEveryRuntimeOperationExplicitly()
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

    QVERIFY(hasOperationKinds(plan,
        {
            ImageDocumentRuntimeOperationKind::CancelFileDeletion,
            ImageDocumentRuntimeOperationKind::StopPresentationAnimation,
            ImageDocumentRuntimeOperationKind::ShutdownSpread,
            ImageDocumentRuntimeOperationKind::ClearArchiveSession,
            ImageDocumentRuntimeOperationKind::ClearPredecode,
            ImageDocumentRuntimeOperationKind::CancelPredecode,
            ImageDocumentRuntimeOperationKind::ScheduleAdjacentImagePredecode,
            ImageDocumentRuntimeOperationKind::FinishSpreadTransition,
            ImageDocumentRuntimeOperationKind::ResetRightToLeftReading,
            ImageDocumentRuntimeOperationKind::ClearSecondaryPage,
            ImageDocumentRuntimeOperationKind::NotifyRightToLeftReadingChanged,
            ImageDocumentRuntimeOperationKind::ResetZoom,
            ImageDocumentRuntimeOperationKind::PrepareFailedContainer,
            ImageDocumentRuntimeOperationKind::CancelPageNavigationUpdate,
            ImageDocumentRuntimeOperationKind::CancelNavigation,
            ImageDocumentRuntimeOperationKind::CancelContainerNavigation,
            ImageDocumentRuntimeOperationKind::CancelAllNavigation,
            ImageDocumentRuntimeOperationKind::ClearPageNavigation,
            ImageDocumentRuntimeOperationKind::UpdatePageNavigation,
            ImageDocumentRuntimeOperationKind::LoadUrl,
            ImageDocumentRuntimeOperationKind::LoadContainerImage,
            ImageDocumentRuntimeOperationKind::FinishEmptyContainerNavigation,
            ImageDocumentRuntimeOperationKind::FinishContainerNavigationLoadWithError,
            ImageDocumentRuntimeOperationKind::LoadPageNavigationUrl,
            ImageDocumentRuntimeOperationKind::CancelOpen,
            ImageDocumentRuntimeOperationKind::ClearDisplayedImageLocation,
            ImageDocumentRuntimeOperationKind::ClearPresentationImage,
            ImageDocumentRuntimeOperationKind::ClearLoadingContainerNavigationUrl,
            ImageDocumentRuntimeOperationKind::SetLoadingContainerNavigationUrl,
            ImageDocumentRuntimeOperationKind::SetContainerNavigationUrl,
            ImageDocumentRuntimeOperationKind::PrepareSourceLoad,
            ImageDocumentRuntimeOperationKind::SetSourceUrl,
            ImageDocumentRuntimeOperationKind::BeginOpen,
            ImageDocumentRuntimeOperationKind::SetErrorString,
            ImageDocumentRuntimeOperationKind::FinishEmptySourceLoad,
        }));
}

QTEST_GUILESS_MAIN(TestImageDocumentEffectPlan)

#include "test_imagedocumenteffectplan.moc"

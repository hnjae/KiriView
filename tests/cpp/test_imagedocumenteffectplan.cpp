// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectplan.h"

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

ImageDocumentRuntimeOperation operation(ImageDocumentRuntimeOperationKind kind)
{
    return ImageDocumentRuntimeOperation::simple(kind);
}

std::vector<ImageDocumentRuntimeOperationKind> operationKinds(const ImageDocumentRuntimePlan &plan)
{
    std::vector<ImageDocumentRuntimeOperationKind> kinds;
    kinds.reserve(plan.size());
    for (const ImageDocumentRuntimeOperation &operation : plan) {
        kinds.push_back(operation.kind);
    }
    return kinds;
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
};

void TestImageDocumentEffectPlan::clearImagePlansOrderedRuntimeOperations()
{
    const ImageDocumentRuntimePlan plan
        = KiriView::imageDocumentEffectPlan(ImageDocumentEffect::clearImage());

    QVERIFY(operationKinds(plan)
        == std::vector<ImageDocumentRuntimeOperationKind>({
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

    QVERIFY(plan
        == ImageDocumentRuntimePlan({
            operation(ImageDocumentRuntimeOperationKind::ClearArchiveSession),
            operation(ImageDocumentRuntimeOperationKind::CancelNavigation),
            operation(ImageDocumentRuntimeOperationKind::CancelContainerNavigation),
            operation(ImageDocumentRuntimeOperationKind::CancelPredecode),
            operation(ImageDocumentRuntimeOperationKind::CancelOpen),
            operation(ImageDocumentRuntimeOperationKind::FinishSpreadTransition),
            operation(ImageDocumentRuntimeOperationKind::ClearSecondaryPage),
            ImageDocumentRuntimeOperation::setSourceUrl(QUrl()),
            ImageDocumentRuntimeOperation::setErrorString(QString()),
            operation(ImageDocumentRuntimeOperationKind::FinishEmptySourceLoad),
        }));
}

void TestImageDocumentEffectPlan::shutdownPlansOrderedRuntimeOperations()
{
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentShutdownPlan();

    QVERIFY(operationKinds(plan)
        == std::vector<ImageDocumentRuntimeOperationKind>({
            ImageDocumentRuntimeOperationKind::CancelFileDeletion,
            ImageDocumentRuntimeOperationKind::StopPresentationAnimation,
            ImageDocumentRuntimeOperationKind::ShutdownSpread,
            ImageDocumentRuntimeOperationKind::CancelPredecode,
            ImageDocumentRuntimeOperationKind::CancelPageNavigationUpdate,
            ImageDocumentRuntimeOperationKind::CancelContainerNavigation,
            ImageDocumentRuntimeOperationKind::CancelNavigation,
            ImageDocumentRuntimeOperationKind::CancelOpen,
            ImageDocumentRuntimeOperationKind::ClearArchiveSession,
        }));
}

void TestImageDocumentEffectPlan::payloadEffectsPlanRuntimeOperations()
{
    QVERIFY(KiriView::imageDocumentEffectPlan(
                ImageDocumentEffect::openUrl(localUrl(QStringLiteral("/image.png"))))
        == ImageDocumentRuntimePlan(
            { ImageDocumentRuntimeOperation::loadUrl(localUrl(QStringLiteral("/image.png"))) }));

    QVERIFY(KiriView::imageDocumentEffectPlan(ImageDocumentEffect::containerImageSelected(
                localUrl(QStringLiteral("/book/01.png")), localUrl(QStringLiteral("/book.cbz"))))
        == ImageDocumentRuntimePlan({ ImageDocumentRuntimeOperation::loadContainerImage(
            localUrl(QStringLiteral("/book/01.png")), localUrl(QStringLiteral("/book.cbz"))) }));

    QVERIFY(KiriView::imageDocumentEffectPlan(
                ImageDocumentEffect::emptyContainerSelected(localUrl(QStringLiteral("/empty.cbz"))))
        == ImageDocumentRuntimePlan({ ImageDocumentRuntimeOperation::finishEmptyContainerNavigation(
            localUrl(QStringLiteral("/empty.cbz"))) }));

    QVERIFY(KiriView::imageDocumentEffectPlan(ImageDocumentEffect::containerNavigationFailed(
                localUrl(QStringLiteral("/broken.cbz")), QStringLiteral("broken")))
        == ImageDocumentRuntimePlan({
            ImageDocumentRuntimeOperation::finishContainerNavigationLoadWithError(
                localUrl(QStringLiteral("/broken.cbz")), QStringLiteral("broken")),
        }));

    QVERIFY(KiriView::imageDocumentEffectPlan(ImageDocumentEffect::pageNavigationSelected(
                localUrl(QStringLiteral("/next.png")), true))
        == ImageDocumentRuntimePlan({ ImageDocumentRuntimeOperation::loadPageNavigationUrl(
            localUrl(QStringLiteral("/next.png")), true) }));

    QVERIFY(KiriView::imageDocumentEffectPlan(
                ImageDocumentEffect::prepareFailedContainer(localUrl(QStringLiteral("/bad.zip"))))
        == ImageDocumentRuntimePlan({ ImageDocumentRuntimeOperation::prepareFailedContainer(
            localUrl(QStringLiteral("/bad.zip"))) }));
}

QTEST_GUILESS_MAIN(TestImageDocumentEffectPlan)

#include "test_imagedocumenteffectplan.moc"

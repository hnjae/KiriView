// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentsourceloadruntimeplan.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <variant>
#include <vector>

class TestImageDocumentSourceLoadRuntimePlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void replacementLoadMapsOperationsAroundSourceMutation();
    void currentLoadMapsReadingResetBeforeSourceState();
};

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

std::vector<KiriView::ImageDocumentRuntimeOperationKind> operationKinds(
    const KiriView::ImageDocumentRuntimePlan &plan)
{
    std::vector<KiriView::ImageDocumentRuntimeOperationKind> kinds;
    kinds.reserve(plan.size());
    for (const KiriView::ImageDocumentRuntimeOperation &operation : plan) {
        kinds.push_back(KiriView::imageDocumentRuntimeOperationKind(operation));
    }
    return kinds;
}

void compareOperationKinds(const KiriView::ImageDocumentRuntimePlan &plan,
    const std::vector<KiriView::ImageDocumentRuntimeOperationKind> &expected)
{
    const std::vector<KiriView::ImageDocumentRuntimeOperationKind> actual = operationKinds(plan);
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

template <typename Operation>
const Operation &operationAt(const KiriView::ImageDocumentRuntimePlan &plan, std::size_t index)
{
    return std::get<Operation>(plan.at(index));
}
}

void TestImageDocumentSourceLoadRuntimePlan::replacementLoadMapsOperationsAroundSourceMutation()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/images/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl);

    KiriView::ImageDocumentSourceLoadPlan plan;
    using Operation = KiriView::ImageDocumentSourceLoadOperation;
    plan.operations = {
        Operation::CancelFileDeletion,
        Operation::CancelNavigationAndPredecode,
        Operation::FinishSpreadTransition,
        Operation::ResetRightToLeftReading,
        Operation::ClearSecondaryPage,
        Operation::SetLoadingContainerNavigationUrlToRequested,
        Operation::PrepareSourceLoad,
        Operation::SetSourceUrlToRequested,
        Operation::BeginOpen,
        Operation::NotifyRightToLeftReadingChanged,
    };

    const KiriView::ImageDocumentRuntimePlan runtimePlan
        = KiriView::imageDocumentSourceLoadRuntimePlan(request, plan);

    using RuntimeOperation = KiriView::ImageDocumentRuntimeOperationKind;
    compareOperationKinds(runtimePlan,
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::CancelAllNavigation,
            RuntimeOperation::CancelPredecode,
            RuntimeOperation::FinishSpreadTransition,
            RuntimeOperation::ResetRightToLeftReading,
            RuntimeOperation::ClearSecondaryPage,
            RuntimeOperation::SetLoadingContainerNavigationUrl,
            RuntimeOperation::PrepareSourceLoad,
            RuntimeOperation::SetSourceUrl,
            RuntimeOperation::BeginOpen,
            RuntimeOperation::NotifyRightToLeftReadingChanged,
        });
    QCOMPARE(operationAt<KiriView::SetLoadingContainerNavigationUrlOperation>(runtimePlan, 6).url,
        containerUrl);
    QCOMPARE(operationAt<KiriView::PrepareSourceLoadOperation>(runtimePlan, 7).request.sourceUrl,
        sourceUrl);
    QCOMPARE(operationAt<KiriView::PrepareSourceLoadOperation>(runtimePlan, 7)
                 .request.containerNavigationUrl,
        containerUrl);
    QCOMPARE(operationAt<KiriView::SetSourceUrlOperation>(runtimePlan, 8).url, sourceUrl);
}

void TestImageDocumentSourceLoadRuntimePlan::currentLoadMapsReadingResetBeforeSourceState()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/images/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl);

    KiriView::ImageDocumentSourceLoadPlan plan;
    using Operation = KiriView::ImageDocumentSourceLoadOperation;
    plan.operations = {
        Operation::CancelFileDeletion,
        Operation::ResetRightToLeftReading,
        Operation::NotifyRightToLeftReadingChanged,
        Operation::ClearLoadingContainerNavigationUrl,
        Operation::SetContainerNavigationUrlToRequested,
    };

    const KiriView::ImageDocumentRuntimePlan runtimePlan
        = KiriView::imageDocumentSourceLoadRuntimePlan(request, plan);

    using RuntimeOperation = KiriView::ImageDocumentRuntimeOperationKind;
    compareOperationKinds(runtimePlan,
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::ResetRightToLeftReading,
            RuntimeOperation::NotifyRightToLeftReadingChanged,
            RuntimeOperation::ClearLoadingContainerNavigationUrl,
            RuntimeOperation::SetContainerNavigationUrl,
        });
    QCOMPARE(operationAt<KiriView::SetContainerNavigationUrlOperation>(runtimePlan, 4).url,
        containerUrl);
}

QTEST_GUILESS_MAIN(TestImageDocumentSourceLoadRuntimePlan)

#include "test_imagedocumentsourceloadruntimeplan.moc"

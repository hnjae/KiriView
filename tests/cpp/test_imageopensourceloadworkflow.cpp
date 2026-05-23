// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imageopenworkflow.h"

#include "image_document_plan_test_support.h"
#include "navigation/imagecontainer.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using KiriView::TestSupport::hasOperationTypes;
using KiriView::TestSupport::operationAt;
using KiriView::TestSupport::operationTypes;

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }
}

class TestImageOpenSourceLoadWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void currentSourceLoadUsesRuntimeSnapshotAndRequestedContainer();
    void displayedComicBookScopeSuppressesRightToLeftReadingReset();
    void replacementSourceLoadStartsFreshRuntimeWork();
    void sourceLoadPlanResolvesRequestedRuntimePayloads();
};

void TestImageOpenSourceLoadWorkflow::currentSourceLoadUsesRuntimeSnapshotAndRequestedContainer()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/images/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const KiriView::ImageDocumentSourceLoadSnapshot snapshot {
        sourceUrl,
        {},
        true,
    };
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl);
    const KiriView::ImageDocumentRuntimePlan plan
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(snapshot, request);

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::FinishSpreadTransitionOperation,
            KiriView::ClearLoadingContainerNavigationUrlOperation,
            KiriView::SetContainerNavigationUrlOperation>()));
    QCOMPARE(operationAt<KiriView::SetContainerNavigationUrlOperation>(plan, 3).url, containerUrl);
}

void TestImageOpenSourceLoadWorkflow::displayedComicBookScopeSuppressesRightToLeftReadingReset()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl(QStringLiteral("%1/01.png").arg(archiveRootUrl->toString()));
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/page.png"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());

    const KiriView::ImageDocumentSourceLoadSnapshot snapshot {
        replacementUrl,
        *archiveDocument,
        true,
    };

    KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromPageNavigation(imageUrl, true);
    KiriView::ImageDocumentRuntimePlan plan
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(snapshot, request);
    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::CancelAllNavigationOperation, KiriView::CancelPredecodeOperation,
            KiriView::ClearSecondaryPageOperation,
            KiriView::SetLoadingContainerNavigationUrlOperation,
            KiriView::PrepareSourceLoadOperation, KiriView::SetSourceUrlOperation,
            KiriView::BeginOpenOperation>()));

    request = KiriView::ImageDocumentSourceLoadRequest::fromUrl(replacementUrl);
    plan = KiriView::ImageOpenWorkflow::sourceLoadPlan(snapshot, request);
    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::FinishSpreadTransitionOperation, KiriView::ResetRightToLeftReadingOperation,
            KiriView::NotifyRightToLeftReadingChangedOperation,
            KiriView::ClearLoadingContainerNavigationUrlOperation>()));
}

void TestImageOpenSourceLoadWorkflow::replacementSourceLoadStartsFreshRuntimeWork()
{
    const QUrl currentUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/replacement.png"));
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromPageNavigation(replacementUrl, true);
    const KiriView::ImageDocumentSourceLoadSnapshot snapshot {
        currentUrl,
        {},
        true,
    };
    const KiriView::ImageDocumentRuntimePlan plan
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(snapshot, request);

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::CancelAllNavigationOperation, KiriView::CancelPredecodeOperation,
            KiriView::ResetRightToLeftReadingOperation, KiriView::ClearSecondaryPageOperation,
            KiriView::SetLoadingContainerNavigationUrlOperation,
            KiriView::PrepareSourceLoadOperation, KiriView::SetSourceUrlOperation,
            KiriView::BeginOpenOperation, KiriView::NotifyRightToLeftReadingChangedOperation>()));
}

void TestImageOpenSourceLoadWorkflow::sourceLoadPlanResolvesRequestedRuntimePayloads()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/books/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl);
    const KiriView::ImageDocumentSourceLoadSnapshot replacementSnapshot {
        localUrl(QStringLiteral("/images/current.png")),
        {},
        false,
    };
    const KiriView::ImageDocumentRuntimePlan replacementPlan
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(replacementSnapshot, request);

    QVERIFY(hasOperationTypes(replacementPlan,
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::CancelAllNavigationOperation, KiriView::CancelPredecodeOperation,
            KiriView::FinishSpreadTransitionOperation, KiriView::ClearSecondaryPageOperation,
            KiriView::SetLoadingContainerNavigationUrlOperation,
            KiriView::PrepareSourceLoadOperation, KiriView::SetSourceUrlOperation,
            KiriView::BeginOpenOperation>()));
    QCOMPARE(
        operationAt<KiriView::SetLoadingContainerNavigationUrlOperation>(replacementPlan, 5).url,
        containerUrl);
    QCOMPARE(
        operationAt<KiriView::PrepareSourceLoadOperation>(replacementPlan, 6).request.sourceUrl,
        sourceUrl);
    QCOMPARE(operationAt<KiriView::PrepareSourceLoadOperation>(replacementPlan, 6)
                 .request.containerNavigationUrl,
        containerUrl);
    QCOMPARE(operationAt<KiriView::SetSourceUrlOperation>(replacementPlan, 7).url, sourceUrl);

    const KiriView::ImageDocumentSourceLoadSnapshot currentSnapshot {
        sourceUrl,
        {},
        false,
    };
    const KiriView::ImageDocumentRuntimePlan currentPlan
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(currentSnapshot, request);
    QCOMPARE(operationAt<KiriView::SetContainerNavigationUrlOperation>(currentPlan, 3).url,
        containerUrl);
}

QTEST_GUILESS_MAIN(TestImageOpenSourceLoadWorkflow)

#include "test_imageopensourceloadworkflow.moc"

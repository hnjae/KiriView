// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imageopenworkflow.h"

#include "image_document_plan_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using kiriview::TestSupport::hasOperationTypes;
using kiriview::TestSupport::operationAt;
using kiriview::TestSupport::operationTypes;

QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }
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
    const kiriview::ImageDocumentSourceLoadSnapshot snapshot {
        sourceUrl,
        {},
        true,
    };
    const kiriview::ImageDocumentSourceLoadRequest request
        = kiriview::ImageDocumentSourceLoadRequest::fromContainerTarget(
            kiriview::ImageDocumentPageTarget {
                sourceUrl,
                kiriview::ImageDocumentPageKind::Video,
            },
            containerUrl);
    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::ImageOpenWorkflow::sourceLoadPlan(snapshot, request);

    QVERIFY(hasOperationTypes(plan,
        operationTypes<kiriview::CancelFileDeletionOperation,
            kiriview::FinishSpreadTransitionOperation,
            kiriview::ClearLoadingContainerNavigationUrlOperation,
            kiriview::SetContainerNavigationUrlOperation>()));
    QCOMPARE(operationAt<kiriview::SetContainerNavigationUrlOperation>(plan, 3).url, containerUrl);
}

void TestImageOpenSourceLoadWorkflow::displayedComicBookScopeSuppressesRightToLeftReadingReset()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl(QStringLiteral("%1/01.png").arg(archiveRootUrl->toString()));
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/page.png"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());

    const kiriview::ImageDocumentSourceLoadSnapshot snapshot {
        replacementUrl,
        *archiveCollection,
        true,
    };

    kiriview::ImageDocumentSourceLoadRequest request
        = kiriview::ImageDocumentSourceLoadRequest::fromPageNavigation(imageUrl, true);
    kiriview::ImageDocumentRuntimePlan plan
        = kiriview::ImageOpenWorkflow::sourceLoadPlan(snapshot, request);
    QVERIFY(hasOperationTypes(plan,
        operationTypes<kiriview::CancelFileDeletionOperation,
            kiriview::CancelAllNavigationOperation, kiriview::CancelPredecodeOperation,
            kiriview::SetLoadingContainerNavigationUrlOperation,
            kiriview::PrepareSourceLoadOperation, kiriview::SetSourceUrlOperation,
            kiriview::BeginOpenOperation>()));

    request = kiriview::ImageDocumentSourceLoadRequest::fromUrl(replacementUrl);
    plan = kiriview::ImageOpenWorkflow::sourceLoadPlan(snapshot, request);
    QVERIFY(hasOperationTypes(plan,
        operationTypes<kiriview::CancelFileDeletionOperation,
            kiriview::FinishSpreadTransitionOperation, kiriview::ResetRightToLeftReadingOperation,
            kiriview::NotifyRightToLeftReadingChangedOperation,
            kiriview::ClearLoadingContainerNavigationUrlOperation>()));
}

void TestImageOpenSourceLoadWorkflow::replacementSourceLoadStartsFreshRuntimeWork()
{
    const QUrl currentUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/replacement.png"));
    const kiriview::ImageDocumentSourceLoadRequest request
        = kiriview::ImageDocumentSourceLoadRequest::fromPageNavigation(replacementUrl, true);
    const kiriview::ImageDocumentSourceLoadSnapshot snapshot {
        currentUrl,
        {},
        true,
    };
    const kiriview::ImageDocumentRuntimePlan plan
        = kiriview::ImageOpenWorkflow::sourceLoadPlan(snapshot, request);

    QVERIFY(hasOperationTypes(plan,
        operationTypes<kiriview::CancelFileDeletionOperation,
            kiriview::CancelAllNavigationOperation, kiriview::CancelPredecodeOperation,
            kiriview::ResetRightToLeftReadingOperation,
            kiriview::SetLoadingContainerNavigationUrlOperation,
            kiriview::PrepareSourceLoadOperation, kiriview::SetSourceUrlOperation,
            kiriview::BeginOpenOperation, kiriview::NotifyRightToLeftReadingChangedOperation>()));
}

void TestImageOpenSourceLoadWorkflow::sourceLoadPlanResolvesRequestedRuntimePayloads()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/books/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const kiriview::ImageDocumentSourceLoadRequest request
        = kiriview::ImageDocumentSourceLoadRequest::fromContainerTarget(
            kiriview::ImageDocumentPageTarget {
                sourceUrl,
                kiriview::ImageDocumentPageKind::Video,
            },
            containerUrl);
    const kiriview::ImageDocumentSourceLoadSnapshot replacementSnapshot {
        localUrl(QStringLiteral("/images/current.png")),
        {},
        false,
    };
    const kiriview::ImageDocumentRuntimePlan replacementPlan
        = kiriview::ImageOpenWorkflow::sourceLoadPlan(replacementSnapshot, request);

    QVERIFY(hasOperationTypes(replacementPlan,
        operationTypes<kiriview::CancelFileDeletionOperation,
            kiriview::CancelAllNavigationOperation, kiriview::CancelPredecodeOperation,
            kiriview::FinishSpreadTransitionOperation, kiriview::ClearSecondaryPageOperation,
            kiriview::SetLoadingContainerNavigationUrlOperation,
            kiriview::PrepareSourceLoadOperation, kiriview::SetSourceUrlOperation,
            kiriview::BeginOpenOperation>()));
    QCOMPARE(
        operationAt<kiriview::SetLoadingContainerNavigationUrlOperation>(replacementPlan, 5).url,
        containerUrl);
    QCOMPARE(
        operationAt<kiriview::PrepareSourceLoadOperation>(replacementPlan, 6).request.sourceUrl,
        sourceUrl);
    QCOMPARE(
        operationAt<kiriview::PrepareSourceLoadOperation>(replacementPlan, 6).request.sourceKind,
        kiriview::ImageDocumentPageKind::Video);
    QCOMPARE(operationAt<kiriview::PrepareSourceLoadOperation>(replacementPlan, 6)
                 .request.containerNavigationUrl,
        containerUrl);
    QCOMPARE(
        operationAt<kiriview::SetSourceUrlOperation>(replacementPlan, 7).target.url, sourceUrl);
    QCOMPARE(operationAt<kiriview::SetSourceUrlOperation>(replacementPlan, 7).target.kind,
        kiriview::ImageDocumentPageKind::Video);

    const kiriview::ImageDocumentSourceLoadSnapshot currentSnapshot {
        sourceUrl,
        {},
        false,
    };
    const kiriview::ImageDocumentRuntimePlan currentPlan
        = kiriview::ImageOpenWorkflow::sourceLoadPlan(currentSnapshot, request);
    QCOMPARE(operationAt<kiriview::SetContainerNavigationUrlOperation>(currentPlan, 3).url,
        containerUrl);
}

QTEST_GUILESS_MAIN(TestImageOpenSourceLoadWorkflow)

#include "test_imageopensourceloadworkflow.moc"

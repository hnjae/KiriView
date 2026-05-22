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

class TestImageDocumentSourceLoadPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceLoadPolicyInputCapturesRuntimeSnapshot();
    void sourceLoadPolicyInputDetectsDisplayedComicBookScope();
    void sourceLoadPlanDecidesRightToLeftReadingResetFromLoadContext();
    void sourceLoadPlanRoutesUnchangedAndReplacementSourceLoads();
    void sourceLoadPlanResolvesRequestedRuntimePayloads();
};

void TestImageDocumentSourceLoadPolicy::sourceLoadPolicyInputCapturesRuntimeSnapshot()
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

    const KiriView::ImageDocumentSourceLoadPolicyInput input
        = KiriView::ImageOpenWorkflow::sourceLoadPolicyInput(snapshot, request);

    QCOMPARE(input.loadKind, KiriView::ImageDocumentSourceLoadKind::CurrentSource);
    QCOMPARE(input.preserveTwoPageSpreadTransition, false);
    QCOMPARE(input.rightToLeftReadingEnabled, true);
    QCOMPARE(input.sourceWithinDisplayedComicBookArchive, false);
    QCOMPARE(input.hasRequestedContainerNavigationUrl, true);
}

void TestImageDocumentSourceLoadPolicy::sourceLoadPolicyInputDetectsDisplayedComicBookScope()
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
        false,
    };

    KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromPageNavigation(imageUrl, true);
    KiriView::ImageDocumentSourceLoadPolicyInput input
        = KiriView::ImageOpenWorkflow::sourceLoadPolicyInput(snapshot, request);
    QCOMPARE(input.loadKind, KiriView::ImageDocumentSourceLoadKind::ReplacementSource);
    QCOMPARE(input.preserveTwoPageSpreadTransition, true);
    QCOMPARE(input.sourceWithinDisplayedComicBookArchive, true);

    request = KiriView::ImageDocumentSourceLoadRequest::fromUrl(replacementUrl);
    input = KiriView::ImageOpenWorkflow::sourceLoadPolicyInput(snapshot, request);
    QCOMPARE(input.sourceWithinDisplayedComicBookArchive, false);
}

void TestImageDocumentSourceLoadPolicy::
    sourceLoadPlanDecidesRightToLeftReadingResetFromLoadContext()
{
    KiriView::ImageDocumentSourceLoadPolicyInput input;
    input.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    input.preserveTwoPageSpreadTransition = true;
    input.hasRequestedContainerNavigationUrl = false;
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromUrl(
            localUrl(QStringLiteral("/images/page.png")));

    input.rightToLeftReadingEnabled = false;
    input.sourceWithinDisplayedComicBookArchive = false;
    QVERIFY(hasOperationTypes(KiriView::ImageOpenWorkflow::sourceLoadPlan(input, request),
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::ClearLoadingContainerNavigationUrlOperation>()));

    input.rightToLeftReadingEnabled = true;
    input.sourceWithinDisplayedComicBookArchive = false;
    QVERIFY(hasOperationTypes(KiriView::ImageOpenWorkflow::sourceLoadPlan(input, request),
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::ResetRightToLeftReadingOperation,
            KiriView::NotifyRightToLeftReadingChangedOperation,
            KiriView::ClearLoadingContainerNavigationUrlOperation>()));

    input.sourceWithinDisplayedComicBookArchive = true;
    QVERIFY(hasOperationTypes(KiriView::ImageOpenWorkflow::sourceLoadPlan(input, request),
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::ClearLoadingContainerNavigationUrlOperation>()));

    input.sourceWithinDisplayedComicBookArchive = false;
    input.hasRequestedContainerNavigationUrl = true;
    QVERIFY(hasOperationTypes(KiriView::ImageOpenWorkflow::sourceLoadPlan(input, request),
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::ClearLoadingContainerNavigationUrlOperation,
            KiriView::SetContainerNavigationUrlOperation>()));
}

void TestImageDocumentSourceLoadPolicy::sourceLoadPlanRoutesUnchangedAndReplacementSourceLoads()
{
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromUrl(
            localUrl(QStringLiteral("/images/page.png")));

    KiriView::ImageDocumentSourceLoadPolicyInput unchangedInput;
    unchangedInput.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    unchangedInput.preserveTwoPageSpreadTransition = false;
    unchangedInput.rightToLeftReadingEnabled = true;
    unchangedInput.sourceWithinDisplayedComicBookArchive = false;
    unchangedInput.hasRequestedContainerNavigationUrl = true;
    QVERIFY(hasOperationTypes(KiriView::ImageOpenWorkflow::sourceLoadPlan(unchangedInput, request),
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::FinishSpreadTransitionOperation,
            KiriView::ClearLoadingContainerNavigationUrlOperation,
            KiriView::SetContainerNavigationUrlOperation>()));

    unchangedInput.hasRequestedContainerNavigationUrl = false;
    QVERIFY(hasOperationTypes(KiriView::ImageOpenWorkflow::sourceLoadPlan(unchangedInput, request),
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::FinishSpreadTransitionOperation, KiriView::ResetRightToLeftReadingOperation,
            KiriView::NotifyRightToLeftReadingChangedOperation,
            KiriView::ClearLoadingContainerNavigationUrlOperation>()));

    KiriView::ImageDocumentSourceLoadPolicyInput replacementInput;
    replacementInput.loadKind = KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
    replacementInput.preserveTwoPageSpreadTransition = true;
    replacementInput.rightToLeftReadingEnabled = false;
    replacementInput.sourceWithinDisplayedComicBookArchive = false;
    replacementInput.hasRequestedContainerNavigationUrl = false;
    QVERIFY(
        hasOperationTypes(KiriView::ImageOpenWorkflow::sourceLoadPlan(replacementInput, request),
            operationTypes<KiriView::CancelFileDeletionOperation,
                KiriView::CancelAllNavigationOperation, KiriView::CancelPredecodeOperation,
                KiriView::ClearSecondaryPageOperation,
                KiriView::SetLoadingContainerNavigationUrlOperation,
                KiriView::PrepareSourceLoadOperation, KiriView::SetSourceUrlOperation,
                KiriView::BeginOpenOperation>()));

    replacementInput.rightToLeftReadingEnabled = true;
    QVERIFY(hasOperationTypes(
        KiriView::ImageOpenWorkflow::sourceLoadPlan(replacementInput, request),
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::CancelAllNavigationOperation, KiriView::CancelPredecodeOperation,
            KiriView::ResetRightToLeftReadingOperation, KiriView::ClearSecondaryPageOperation,
            KiriView::SetLoadingContainerNavigationUrlOperation,
            KiriView::PrepareSourceLoadOperation, KiriView::SetSourceUrlOperation,
            KiriView::BeginOpenOperation, KiriView::NotifyRightToLeftReadingChangedOperation>()));
}

void TestImageDocumentSourceLoadPolicy::sourceLoadPlanResolvesRequestedRuntimePayloads()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/books/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl);

    KiriView::ImageDocumentSourceLoadPolicyInput replacementInput;
    replacementInput.loadKind = KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
    replacementInput.preserveTwoPageSpreadTransition = true;
    const KiriView::ImageDocumentRuntimePlan replacementPlan
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(replacementInput, request);

    QVERIFY(hasOperationTypes(replacementPlan,
        operationTypes<KiriView::CancelFileDeletionOperation,
            KiriView::CancelAllNavigationOperation, KiriView::CancelPredecodeOperation,
            KiriView::ClearSecondaryPageOperation,
            KiriView::SetLoadingContainerNavigationUrlOperation,
            KiriView::PrepareSourceLoadOperation, KiriView::SetSourceUrlOperation,
            KiriView::BeginOpenOperation>()));
    QCOMPARE(
        operationAt<KiriView::SetLoadingContainerNavigationUrlOperation>(replacementPlan, 4).url,
        containerUrl);
    QCOMPARE(
        operationAt<KiriView::PrepareSourceLoadOperation>(replacementPlan, 5).request.sourceUrl,
        sourceUrl);
    QCOMPARE(operationAt<KiriView::PrepareSourceLoadOperation>(replacementPlan, 5)
                 .request.containerNavigationUrl,
        containerUrl);
    QCOMPARE(operationAt<KiriView::SetSourceUrlOperation>(replacementPlan, 6).url, sourceUrl);

    KiriView::ImageDocumentSourceLoadPolicyInput currentInput;
    currentInput.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    currentInput.preserveTwoPageSpreadTransition = true;
    currentInput.hasRequestedContainerNavigationUrl = true;
    const KiriView::ImageDocumentRuntimePlan currentPlan
        = KiriView::ImageOpenWorkflow::sourceLoadPlan(currentInput, request);
    QCOMPARE(operationAt<KiriView::SetContainerNavigationUrlOperation>(currentPlan, 2).url,
        containerUrl);
}

QTEST_GUILESS_MAIN(TestImageDocumentSourceLoadPolicy)

#include "test_imagedocumentsourceloadpolicy.moc"

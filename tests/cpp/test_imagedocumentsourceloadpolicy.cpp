// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentsourceloadpolicy.h"

#include "navigation/imagecontainer.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

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
        = KiriView::imageDocumentSourceLoadPolicyInput(snapshot, request);

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
        = KiriView::imageDocumentSourceLoadPolicyInput(snapshot, request);
    QCOMPARE(input.loadKind, KiriView::ImageDocumentSourceLoadKind::ReplacementSource);
    QCOMPARE(input.preserveTwoPageSpreadTransition, true);
    QCOMPARE(input.sourceWithinDisplayedComicBookArchive, true);

    request = KiriView::ImageDocumentSourceLoadRequest::fromUrl(replacementUrl);
    input = KiriView::imageDocumentSourceLoadPolicyInput(snapshot, request);
    QCOMPARE(input.sourceWithinDisplayedComicBookArchive, false);
}

void TestImageDocumentSourceLoadPolicy::
    sourceLoadPlanDecidesRightToLeftReadingResetFromLoadContext()
{
    using RuntimeOperation = KiriView::ImageDocumentRuntimeOperationKind;

    KiriView::ImageDocumentSourceLoadPolicyInput input;
    input.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    input.preserveTwoPageSpreadTransition = true;
    input.hasRequestedContainerNavigationUrl = false;
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromUrl(
            localUrl(QStringLiteral("/images/page.png")));

    input.rightToLeftReadingEnabled = false;
    input.sourceWithinDisplayedComicBookArchive = false;
    compareOperationKinds(KiriView::ImageDocumentSourceLoadPolicy::plan(input, request),
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::ClearLoadingContainerNavigationUrl,
        });

    input.rightToLeftReadingEnabled = true;
    input.sourceWithinDisplayedComicBookArchive = false;
    compareOperationKinds(KiriView::ImageDocumentSourceLoadPolicy::plan(input, request),
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::ResetRightToLeftReading,
            RuntimeOperation::NotifyRightToLeftReadingChanged,
            RuntimeOperation::ClearLoadingContainerNavigationUrl,
        });

    input.sourceWithinDisplayedComicBookArchive = true;
    compareOperationKinds(KiriView::ImageDocumentSourceLoadPolicy::plan(input, request),
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::ClearLoadingContainerNavigationUrl,
        });

    input.sourceWithinDisplayedComicBookArchive = false;
    input.hasRequestedContainerNavigationUrl = true;
    compareOperationKinds(KiriView::ImageDocumentSourceLoadPolicy::plan(input, request),
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::ClearLoadingContainerNavigationUrl,
            RuntimeOperation::SetContainerNavigationUrl,
        });
}

void TestImageDocumentSourceLoadPolicy::sourceLoadPlanRoutesUnchangedAndReplacementSourceLoads()
{
    using RuntimeOperation = KiriView::ImageDocumentRuntimeOperationKind;
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromUrl(
            localUrl(QStringLiteral("/images/page.png")));

    KiriView::ImageDocumentSourceLoadPolicyInput unchangedInput;
    unchangedInput.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    unchangedInput.preserveTwoPageSpreadTransition = false;
    unchangedInput.rightToLeftReadingEnabled = true;
    unchangedInput.sourceWithinDisplayedComicBookArchive = false;
    unchangedInput.hasRequestedContainerNavigationUrl = true;
    compareOperationKinds(KiriView::ImageDocumentSourceLoadPolicy::plan(unchangedInput, request),
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::FinishSpreadTransition,
            RuntimeOperation::ClearLoadingContainerNavigationUrl,
            RuntimeOperation::SetContainerNavigationUrl,
        });

    unchangedInput.hasRequestedContainerNavigationUrl = false;
    compareOperationKinds(KiriView::ImageDocumentSourceLoadPolicy::plan(unchangedInput, request),
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::FinishSpreadTransition,
            RuntimeOperation::ResetRightToLeftReading,
            RuntimeOperation::NotifyRightToLeftReadingChanged,
            RuntimeOperation::ClearLoadingContainerNavigationUrl,
        });

    KiriView::ImageDocumentSourceLoadPolicyInput replacementInput;
    replacementInput.loadKind = KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
    replacementInput.preserveTwoPageSpreadTransition = true;
    replacementInput.rightToLeftReadingEnabled = false;
    replacementInput.sourceWithinDisplayedComicBookArchive = false;
    replacementInput.hasRequestedContainerNavigationUrl = false;
    compareOperationKinds(KiriView::ImageDocumentSourceLoadPolicy::plan(replacementInput, request),
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::CancelAllNavigation,
            RuntimeOperation::CancelPredecode,
            RuntimeOperation::ClearSecondaryPage,
            RuntimeOperation::SetLoadingContainerNavigationUrl,
            RuntimeOperation::PrepareSourceLoad,
            RuntimeOperation::SetSourceUrl,
            RuntimeOperation::BeginOpen,
        });

    replacementInput.rightToLeftReadingEnabled = true;
    compareOperationKinds(KiriView::ImageDocumentSourceLoadPolicy::plan(replacementInput, request),
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::CancelAllNavigation,
            RuntimeOperation::CancelPredecode,
            RuntimeOperation::ResetRightToLeftReading,
            RuntimeOperation::ClearSecondaryPage,
            RuntimeOperation::SetLoadingContainerNavigationUrl,
            RuntimeOperation::PrepareSourceLoad,
            RuntimeOperation::SetSourceUrl,
            RuntimeOperation::BeginOpen,
            RuntimeOperation::NotifyRightToLeftReadingChanged,
        });
}

void TestImageDocumentSourceLoadPolicy::sourceLoadPlanResolvesRequestedRuntimePayloads()
{
    using RuntimeOperation = KiriView::ImageDocumentRuntimeOperationKind;
    const QUrl sourceUrl = localUrl(QStringLiteral("/books/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl);

    KiriView::ImageDocumentSourceLoadPolicyInput replacementInput;
    replacementInput.loadKind = KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
    replacementInput.preserveTwoPageSpreadTransition = true;
    const KiriView::ImageDocumentRuntimePlan replacementPlan
        = KiriView::ImageDocumentSourceLoadPolicy::plan(replacementInput, request);

    compareOperationKinds(replacementPlan,
        {
            RuntimeOperation::CancelFileDeletion,
            RuntimeOperation::CancelAllNavigation,
            RuntimeOperation::CancelPredecode,
            RuntimeOperation::ClearSecondaryPage,
            RuntimeOperation::SetLoadingContainerNavigationUrl,
            RuntimeOperation::PrepareSourceLoad,
            RuntimeOperation::SetSourceUrl,
            RuntimeOperation::BeginOpen,
        });
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
        = KiriView::ImageDocumentSourceLoadPolicy::plan(currentInput, request);
    QCOMPARE(operationAt<KiriView::SetContainerNavigationUrlOperation>(currentPlan, 2).url,
        containerUrl);
}

QTEST_GUILESS_MAIN(TestImageDocumentSourceLoadPolicy)

#include "test_imagedocumentsourceloadpolicy.moc"

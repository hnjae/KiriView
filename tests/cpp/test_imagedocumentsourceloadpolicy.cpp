// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadpolicy.h"

#include "imagecontainer.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

void compareSourceLoadPlans(const KiriView::ImageDocumentSourceLoadPlan &actual,
    const KiriView::ImageDocumentSourceLoadPlan &expected)
{
    QCOMPARE(actual.operations.size(), expected.operations.size());
    for (std::size_t index = 0; index < expected.operations.size(); ++index) {
        QCOMPARE(actual.operations.at(index), expected.operations.at(index));
    }
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
    using Operation = KiriView::ImageDocumentSourceLoadOperation;

    KiriView::ImageDocumentSourceLoadPolicyInput input;
    input.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    input.preserveTwoPageSpreadTransition = true;
    input.hasRequestedContainerNavigationUrl = false;

    input.rightToLeftReadingEnabled = false;
    input.sourceWithinDisplayedComicBookArchive = false;
    compareSourceLoadPlans(KiriView::ImageDocumentSourceLoadPolicy::plan(input),
        KiriView::ImageDocumentSourceLoadPlan {
            { Operation::CancelFileDeletion, Operation::ClearLoadingContainerNavigationUrl } });

    input.rightToLeftReadingEnabled = true;
    input.sourceWithinDisplayedComicBookArchive = false;
    compareSourceLoadPlans(KiriView::ImageDocumentSourceLoadPolicy::plan(input),
        KiriView::ImageDocumentSourceLoadPlan { { Operation::CancelFileDeletion,
            Operation::ResetRightToLeftReading, Operation::NotifyRightToLeftReadingChanged,
            Operation::ClearLoadingContainerNavigationUrl } });

    input.sourceWithinDisplayedComicBookArchive = true;
    compareSourceLoadPlans(KiriView::ImageDocumentSourceLoadPolicy::plan(input),
        KiriView::ImageDocumentSourceLoadPlan {
            { Operation::CancelFileDeletion, Operation::ClearLoadingContainerNavigationUrl } });

    input.sourceWithinDisplayedComicBookArchive = false;
    input.hasRequestedContainerNavigationUrl = true;
    compareSourceLoadPlans(KiriView::ImageDocumentSourceLoadPolicy::plan(input),
        KiriView::ImageDocumentSourceLoadPlan {
            { Operation::CancelFileDeletion, Operation::ClearLoadingContainerNavigationUrl,
                Operation::SetContainerNavigationUrlToRequested } });
}

void TestImageDocumentSourceLoadPolicy::sourceLoadPlanRoutesUnchangedAndReplacementSourceLoads()
{
    using Operation = KiriView::ImageDocumentSourceLoadOperation;

    KiriView::ImageDocumentSourceLoadPolicyInput unchangedInput;
    unchangedInput.loadKind = KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    unchangedInput.preserveTwoPageSpreadTransition = false;
    unchangedInput.rightToLeftReadingEnabled = true;
    unchangedInput.sourceWithinDisplayedComicBookArchive = false;
    unchangedInput.hasRequestedContainerNavigationUrl = true;
    const KiriView::ImageDocumentSourceLoadPlan unchanged
        = KiriView::ImageDocumentSourceLoadPolicy::plan(unchangedInput);
    compareSourceLoadPlans(unchanged,
        KiriView::ImageDocumentSourceLoadPlan { { Operation::CancelFileDeletion,
            Operation::FinishSpreadTransition, Operation::ClearLoadingContainerNavigationUrl,
            Operation::SetContainerNavigationUrlToRequested } });

    unchangedInput.hasRequestedContainerNavigationUrl = false;
    const KiriView::ImageDocumentSourceLoadPlan resettingUnchanged
        = KiriView::ImageDocumentSourceLoadPolicy::plan(unchangedInput);
    compareSourceLoadPlans(resettingUnchanged,
        KiriView::ImageDocumentSourceLoadPlan {
            { Operation::CancelFileDeletion, Operation::FinishSpreadTransition,
                Operation::ResetRightToLeftReading, Operation::NotifyRightToLeftReadingChanged,
                Operation::ClearLoadingContainerNavigationUrl } });

    KiriView::ImageDocumentSourceLoadPolicyInput replacementInput;
    replacementInput.loadKind = KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
    replacementInput.preserveTwoPageSpreadTransition = true;
    replacementInput.rightToLeftReadingEnabled = false;
    replacementInput.sourceWithinDisplayedComicBookArchive = false;
    replacementInput.hasRequestedContainerNavigationUrl = false;
    const KiriView::ImageDocumentSourceLoadPlan replacement
        = KiriView::ImageDocumentSourceLoadPolicy::plan(replacementInput);
    compareSourceLoadPlans(replacement,
        KiriView::ImageDocumentSourceLoadPlan { { Operation::CancelFileDeletion,
            Operation::CancelNavigationAndPredecode, Operation::ClearSecondaryPage,
            Operation::SetLoadingContainerNavigationUrlToRequested, Operation::PrepareSourceLoad,
            Operation::SetSourceUrlToRequested, Operation::BeginOpen } });

    replacementInput.rightToLeftReadingEnabled = true;
    const KiriView::ImageDocumentSourceLoadPlan resettingReplacement
        = KiriView::ImageDocumentSourceLoadPolicy::plan(replacementInput);
    compareSourceLoadPlans(resettingReplacement,
        KiriView::ImageDocumentSourceLoadPlan { { Operation::CancelFileDeletion,
            Operation::CancelNavigationAndPredecode, Operation::ResetRightToLeftReading,
            Operation::ClearSecondaryPage, Operation::SetLoadingContainerNavigationUrlToRequested,
            Operation::PrepareSourceLoad, Operation::SetSourceUrlToRequested, Operation::BeginOpen,
            Operation::NotifyRightToLeftReadingChanged } });
}

QTEST_GUILESS_MAIN(TestImageDocumentSourceLoadPolicy)

#include "test_imagedocumentsourceloadpolicy.moc"

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionrouteplan.h"

#include "navigation/mediaformatregistry.h"

#include <QObject>
#include <QTest>
#include <QUrl>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

template <typename Operation>
const Operation *operationAt(const KiriView::DocumentSessionRoutePlan &plan, std::size_t index)
{
    if (index >= plan.operations.size()) {
        return nullptr;
    }

    return std::get_if<Operation>(&plan.operations.at(index));
}

template <typename Operation> bool hasOperation(const KiriView::DocumentSessionRoutePlan &plan)
{
    for (const KiriView::DocumentSessionRouteOperation &operation : plan.operations) {
        if (std::holds_alternative<Operation>(operation)) {
            return true;
        }
    }

    return false;
}
}

class TestDocumentSessionRoutePlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyUrlProducesEmptyClearPlan();
    void directVideoLocalAndKdeArchiveUrlsRouteToDirectVideo();
    void directImageLocalAndKdeArchiveUrlsRouteToDirectImage();
    void localDirectoryAndGeneralArchiveNamesRouteToImageDocument();
    void unsupportedExtensionRoutesToImageDocument();
    void fileNamesMatchCaseInsensitively();
    void sourceRoutesPrepareSessionForTopLevelRouting();
    void routePlanDoesNotAdvertiseVideoBeyondDirectUrlClassification();
    void sourceRoutesClearDirectMediaNavigationButMediaRoutesKeepRequestedReadout();
    void directImageCursorActionPreservesStableCursorOnlyForImageReplacement();
};

void TestDocumentSessionRoutePlan::emptyUrlProducesEmptyClearPlan()
{
    const KiriView::DocumentSessionRoutePlan plan = KiriView::documentSessionRoutePlanForSourceUrl(
        QUrl(), KiriView::DocumentSessionKind::Video);

    QCOMPARE(plan.kind, KiriView::DocumentSessionRouteKind::Empty);
    QVERIFY(plan.sourceUrl.isEmpty());
    QCOMPARE(plan.operations.size(), std::size_t(11));
    QVERIFY(operationAt<KiriView::ClearSessionErrorStringRouteOperation>(plan, 0) != nullptr);
    QVERIFY(operationAt<KiriView::CancelDirectMediaNavigationRouteOperation>(plan, 1) != nullptr);
    QVERIFY(operationAt<KiriView::CancelMediaDeletionRouteOperation>(plan, 2) != nullptr);
    QVERIFY(operationAt<KiriView::ClearDirectMediaNavigationRouteOperation>(plan, 3) != nullptr);
    QVERIFY(operationAt<KiriView::ClearDirectMediaCursorRouteOperation>(plan, 4) != nullptr);
    QVERIFY(operationAt<KiriView::LeaveVideoModeRouteOperation>(plan, 5) != nullptr);
    QVERIFY(operationAt<KiriView::ClearImageDocumentRouteOperation>(plan, 6) != nullptr);
    QVERIFY(operationAt<KiriView::EnterEmptyDocumentRouteOperation>(plan, 7) != nullptr);
    QVERIFY(operationAt<KiriView::ClearSourceIdentityRouteOperation>(plan, 8) != nullptr);
    QVERIFY(operationAt<KiriView::RecomputePublicProjectionRouteOperation>(plan, 9) != nullptr);
    QVERIFY(operationAt<KiriView::ClearMediaPredecodeRouteOperation>(plan, 10) != nullptr);
    QVERIFY(!hasOperation<KiriView::RefreshDirectMediaNavigationAfterRoutingRouteOperation>(plan));
}

void TestDocumentSessionRoutePlan::directVideoLocalAndKdeArchiveUrlsRouteToDirectVideo()
{
    const QUrl localVideo = localUrl(QStringLiteral("/media/clip.mp4"));
    const QUrl archiveVideo = QUrl(QStringLiteral("zip:///books/book.zip!/chapter/clip.mov"));

    for (const QUrl &url : { localVideo, archiveVideo }) {
        const KiriView::DocumentSessionRoutePlan plan
            = KiriView::documentSessionRoutePlanForSourceUrl(
                url, KiriView::DocumentSessionKind::Image);

        QVERIFY(KiriView::isSupportedDirectVideoUrl(url));
        QCOMPARE(plan.kind, KiriView::DocumentSessionRouteKind::DirectVideo);
        QCOMPARE(plan.sourceUrl, url);
        const auto *cursor = operationAt<KiriView::SetDirectVideoCursorRouteOperation>(plan, 4);
        QVERIFY(cursor != nullptr);
        QCOMPARE(cursor->url, url);
        QVERIFY(operationAt<KiriView::ClearImageDocumentRouteOperation>(plan, 5) != nullptr);
        const auto *enterVideo = operationAt<KiriView::EnterVideoDocumentRouteOperation>(plan, 6);
        QVERIFY(enterVideo != nullptr);
        QCOMPARE(enterVideo->url, url);
        const auto *identity
            = operationAt<KiriView::UseOriginalSourceIdentityRouteOperation>(plan, 7);
        QVERIFY(identity != nullptr);
        QCOMPARE(identity->url, url);
        QVERIFY(operationAt<KiriView::RecomputePublicProjectionRouteOperation>(plan, 8) != nullptr);
        QVERIFY(
            operationAt<KiriView::RefreshDirectMediaNavigationAfterRoutingRouteOperation>(plan, 9)
            != nullptr);
        QVERIFY(!hasOperation<KiriView::ClearMediaPredecodeRouteOperation>(plan));
    }
}

void TestDocumentSessionRoutePlan::directImageLocalAndKdeArchiveUrlsRouteToDirectImage()
{
    const QUrl localImage = localUrl(QStringLiteral("/media/page.png"));
    const QUrl archiveImage = QUrl(QStringLiteral("zip:///books/book.zip!/chapter/page.avif"));

    for (const QUrl &url : { localImage, archiveImage }) {
        const KiriView::DocumentSessionRoutePlan plan
            = KiriView::documentSessionRoutePlanForSourceUrl(
                url, KiriView::DocumentSessionKind::Image);

        QVERIFY(KiriView::isSupportedDirectImageUrl(url));
        QCOMPARE(plan.kind, KiriView::DocumentSessionRouteKind::DirectImage);
        QCOMPARE(plan.sourceUrl, url);
        const auto *cursor = operationAt<KiriView::RequestDirectImageCursorRouteOperation>(plan, 4);
        QVERIFY(cursor != nullptr);
        QCOMPARE(cursor->url, url);
        QVERIFY(operationAt<KiriView::LeaveVideoModeRouteOperation>(plan, 5) != nullptr);
        const auto *enterImage = operationAt<KiriView::EnterImageDocumentRouteOperation>(plan, 6);
        QVERIFY(enterImage != nullptr);
        QCOMPARE(enterImage->url, url);
        QVERIFY(operationAt<KiriView::SyncDirectImageCursorFromDocumentRouteOperation>(plan, 7)
            != nullptr);
        QVERIFY(operationAt<KiriView::UseImageDocumentSourceIdentityRouteOperation>(plan, 8)
            != nullptr);
        QVERIFY(operationAt<KiriView::RecomputePublicProjectionRouteOperation>(plan, 9) != nullptr);
        QVERIFY(
            operationAt<KiriView::RefreshDirectMediaNavigationAfterRoutingRouteOperation>(plan, 10)
            != nullptr);
        QVERIFY(!hasOperation<KiriView::ClearMediaPredecodeRouteOperation>(plan));
    }
}

void TestDocumentSessionRoutePlan::localDirectoryAndGeneralArchiveNamesRouteToImageDocument()
{
    for (const QUrl &url :
        { localUrl(QStringLiteral("/books/")), localUrl(QStringLiteral("/books/book.zip")),
            localUrl(QStringLiteral("/books/book.tar")), localUrl(QStringLiteral("/books/book.7z")),
            localUrl(QStringLiteral("/books/book.rar")) }) {
        const KiriView::DocumentSessionRoutePlan plan
            = KiriView::documentSessionRoutePlanForSourceUrl(
                url, KiriView::DocumentSessionKind::Video);

        QCOMPARE(plan.kind, KiriView::DocumentSessionRouteKind::ImageDocument);
        QVERIFY(operationAt<KiriView::ClearDirectMediaCursorRouteOperation>(plan, 4) != nullptr);
        QVERIFY(operationAt<KiriView::LeaveVideoModeRouteOperation>(plan, 5) != nullptr);
        QVERIFY(operationAt<KiriView::EnterImageDocumentRouteOperation>(plan, 6) != nullptr);
        QVERIFY(operationAt<KiriView::SyncDirectImageCursorFromDocumentRouteOperation>(plan, 7)
            != nullptr);
        QVERIFY(
            operationAt<KiriView::RefreshDirectMediaNavigationAfterRoutingRouteOperation>(plan, 10)
            != nullptr);
    }
}

void TestDocumentSessionRoutePlan::unsupportedExtensionRoutesToImageDocument()
{
    const QUrl unsupported = localUrl(QStringLiteral("/media/readme.txt"));
    const KiriView::DocumentSessionRoutePlan plan = KiriView::documentSessionRoutePlanForSourceUrl(
        unsupported, KiriView::DocumentSessionKind::Image);

    QVERIFY(!KiriView::isSupportedDirectVideoUrl(unsupported));
    QVERIFY(!KiriView::isSupportedDirectImageUrl(unsupported));
    QCOMPARE(plan.kind, KiriView::DocumentSessionRouteKind::ImageDocument);
    QVERIFY(operationAt<KiriView::ClearDirectMediaCursorRouteOperation>(plan, 4) != nullptr);
    QVERIFY(operationAt<KiriView::EnterImageDocumentRouteOperation>(plan, 6) != nullptr);
}

void TestDocumentSessionRoutePlan::fileNamesMatchCaseInsensitively()
{
    const QUrl video = localUrl(QStringLiteral("/media/CLIP.M4V"));
    const QUrl image = localUrl(QStringLiteral("/media/SCAN.TIFF"));

    QCOMPARE(
        KiriView::documentSessionRoutePlanForSourceUrl(video, KiriView::DocumentSessionKind::Empty)
            .kind,
        KiriView::DocumentSessionRouteKind::DirectVideo);
    QCOMPARE(
        KiriView::documentSessionRoutePlanForSourceUrl(image, KiriView::DocumentSessionKind::Empty)
            .kind,
        KiriView::DocumentSessionRouteKind::DirectImage);
}

void TestDocumentSessionRoutePlan::sourceRoutesPrepareSessionForTopLevelRouting()
{
    const QUrl image = localUrl(QStringLiteral("/media/page.png"));

    const KiriView::DocumentSessionRoutePlan sourcePlan
        = KiriView::documentSessionRoutePlanForSourceUrl(
            image, KiriView::DocumentSessionKind::Image);
    QVERIFY(operationAt<KiriView::ClearSessionErrorStringRouteOperation>(sourcePlan, 0) != nullptr);
    QVERIFY(
        operationAt<KiriView::CancelDirectMediaNavigationRouteOperation>(sourcePlan, 1) != nullptr);
    QVERIFY(operationAt<KiriView::CancelMediaDeletionRouteOperation>(sourcePlan, 2) != nullptr);

    const KiriView::DocumentSessionRoutePlan mediaPlan
        = KiriView::documentSessionRoutePlanForMediaUrl(
            image, KiriView::DocumentSessionKind::Image);
    QVERIFY(!hasOperation<KiriView::ClearSessionErrorStringRouteOperation>(mediaPlan));
    QVERIFY(!hasOperation<KiriView::CancelDirectMediaNavigationRouteOperation>(mediaPlan));
    QVERIFY(!hasOperation<KiriView::CancelMediaDeletionRouteOperation>(mediaPlan));
}

void TestDocumentSessionRoutePlan::routePlanDoesNotAdvertiseVideoBeyondDirectUrlClassification()
{
    const QUrl archiveRoot = QUrl(QStringLiteral("zip:///books/book.zip!/"));
    const QUrl directoryRoot = localUrl(QStringLiteral("/books/"));
    const QUrl archiveVideoEntry = QUrl(QStringLiteral("zip:///books/book.zip!/clip.mp4"));

    for (const QUrl &url : { archiveRoot, directoryRoot }) {
        const KiriView::DocumentSessionRoutePlan plan
            = KiriView::documentSessionRoutePlanForSourceUrl(
                url, KiriView::DocumentSessionKind::Image);
        QCOMPARE(plan.kind, KiriView::DocumentSessionRouteKind::ImageDocument);
        QVERIFY(!hasOperation<KiriView::EnterVideoDocumentRouteOperation>(plan));
    }

    QCOMPARE(KiriView::documentSessionRoutePlanForSourceUrl(
                 archiveVideoEntry, KiriView::DocumentSessionKind::Image)
                 .kind,
        KiriView::DocumentSessionRouteKind::DirectVideo);
}

void TestDocumentSessionRoutePlan::
    sourceRoutesClearDirectMediaNavigationButMediaRoutesKeepRequestedReadout()
{
    const QUrl image = localUrl(QStringLiteral("/media/page.png"));

    QVERIFY(hasOperation<KiriView::ClearDirectMediaNavigationRouteOperation>(
        KiriView::documentSessionRoutePlanForSourceUrl(
            image, KiriView::DocumentSessionKind::Image)));
    QVERIFY(!hasOperation<KiriView::ClearDirectMediaNavigationRouteOperation>(
        KiriView::documentSessionRoutePlanForMediaUrl(
            image, KiriView::DocumentSessionKind::Image)));
}

void TestDocumentSessionRoutePlan::
    directImageCursorActionPreservesStableCursorOnlyForImageReplacement()
{
    const QUrl image = localUrl(QStringLiteral("/media/page.png"));

    QVERIFY(operationAt<KiriView::RequestDirectImageCursorRouteOperation>(
                KiriView::documentSessionRoutePlanForSourceUrl(
                    image, KiriView::DocumentSessionKind::Image),
                4)
        != nullptr);
    QVERIFY(operationAt<KiriView::ClearThenRequestDirectImageCursorRouteOperation>(
                KiriView::documentSessionRoutePlanForSourceUrl(
                    image, KiriView::DocumentSessionKind::Video),
                4)
        != nullptr);
    QVERIFY(operationAt<KiriView::ClearThenRequestDirectImageCursorRouteOperation>(
                KiriView::documentSessionRoutePlanForSourceUrl(
                    image, KiriView::DocumentSessionKind::Empty),
                4)
        != nullptr);
}

QTEST_GUILESS_MAIN(TestDocumentSessionRoutePlan)

#include "test_documentsessionrouteplan.moc"

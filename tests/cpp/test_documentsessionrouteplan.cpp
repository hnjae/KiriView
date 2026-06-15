// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionrouteplan.h"

#include "navigation/mediaformatregistry.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

template <typename Operation>
const Operation *operationAt(const kiriview::DocumentSessionRoutePlan &plan, std::size_t index)
{
    if (index >= plan.operations.size()) {
        return nullptr;
    }

    return std::get_if<Operation>(&plan.operations.at(index));
}

template <typename Operation> bool hasOperation(const kiriview::DocumentSessionRoutePlan &plan)
{
    for (const kiriview::DocumentSessionRouteOperation &operation : plan.operations) {
        if (std::holds_alternative<Operation>(operation)) {
            return true;
        }
    }

    return false;
}

bool isPublicationOperation(const kiriview::DocumentSessionRouteOperation &operation)
{
    return std::holds_alternative<kiriview::RecomputePublicProjectionRouteOperation>(operation);
}

bool isFollowUpOperation(const kiriview::DocumentSessionRouteOperation &operation)
{
    return std::holds_alternative<kiriview::RefreshDirectMediaNavigationAfterRoutingRouteOperation>(
               operation)
        || std::holds_alternative<kiriview::ClearMediaPredecodeRouteOperation>(operation);
}

void verifyRoutePhaseOrder(const kiriview::DocumentSessionRoutePlan &plan)
{
    bool published = false;
    bool followUpStarted = false;

    for (const kiriview::DocumentSessionRouteOperation &operation : plan.operations) {
        if (isPublicationOperation(operation)) {
            QVERIFY(!followUpStarted);
            published = true;
            continue;
        }

        if (isFollowUpOperation(operation)) {
            QVERIFY(published);
            followUpStarted = true;
            continue;
        }

        QVERIFY(!published);
        QVERIFY(!followUpStarted);
    }
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
    void deletedImageFallbackRoutesAfterClearingImageDocument();
    void deletedVideoFallbackRoutesFromEmptySession();
    void deletedMediaWithoutFallbackClearsNavigationAndPredecode();
    void routePlansKeepMutationPublicationFollowUpOrder();
};

void TestDocumentSessionRoutePlan::emptyUrlProducesEmptyClearPlan()
{
    const kiriview::DocumentSessionRoutePlan plan = kiriview::documentSessionRoutePlanForSourceUrl(
        QUrl(), kiriview::DocumentSessionKind::Video);

    QCOMPARE(plan.kind, kiriview::DocumentSessionRouteKind::Empty);
    QVERIFY(plan.sourceUrl.isEmpty());
    QCOMPARE(plan.operations.size(), std::size_t(11));
    QVERIFY(operationAt<kiriview::ClearSessionErrorStringRouteOperation>(plan, 0) != nullptr);
    QVERIFY(operationAt<kiriview::CancelDirectMediaNavigationRouteOperation>(plan, 1) != nullptr);
    QVERIFY(operationAt<kiriview::CancelMediaDeletionRouteOperation>(plan, 2) != nullptr);
    QVERIFY(operationAt<kiriview::ClearDirectMediaNavigationRouteOperation>(plan, 3) != nullptr);
    QVERIFY(operationAt<kiriview::ClearDirectMediaCursorRouteOperation>(plan, 4) != nullptr);
    QVERIFY(operationAt<kiriview::LeaveVideoModeRouteOperation>(plan, 5) != nullptr);
    QVERIFY(operationAt<kiriview::ClearImageDocumentRouteOperation>(plan, 6) != nullptr);
    QVERIFY(operationAt<kiriview::EnterEmptyDocumentRouteOperation>(plan, 7) != nullptr);
    QVERIFY(operationAt<kiriview::ClearSourceIdentityRouteOperation>(plan, 8) != nullptr);
    QVERIFY(operationAt<kiriview::RecomputePublicProjectionRouteOperation>(plan, 9) != nullptr);
    QVERIFY(operationAt<kiriview::ClearMediaPredecodeRouteOperation>(plan, 10) != nullptr);
    QVERIFY(!hasOperation<kiriview::RefreshDirectMediaNavigationAfterRoutingRouteOperation>(plan));
}

void TestDocumentSessionRoutePlan::directVideoLocalAndKdeArchiveUrlsRouteToDirectVideo()
{
    const QUrl localVideo = localUrl(QStringLiteral("/media/clip.mp4"));
    const QUrl archiveVideo = QUrl(QStringLiteral("zip:///books/book.zip!/chapter/clip.mov"));

    for (const QUrl &url : { localVideo, archiveVideo }) {
        const kiriview::DocumentSessionRoutePlan plan
            = kiriview::documentSessionRoutePlanForSourceUrl(
                url, kiriview::DocumentSessionKind::Image);

        QVERIFY(kiriview::isSupportedDirectVideoUrl(url));
        QCOMPARE(plan.kind, kiriview::DocumentSessionRouteKind::DirectVideo);
        QCOMPARE(plan.sourceUrl, url);
        const auto *cursor = operationAt<kiriview::SetDirectVideoCursorRouteOperation>(plan, 4);
        QVERIFY(cursor != nullptr);
        QCOMPARE(cursor->url, url);
        QVERIFY(operationAt<kiriview::ClearImageDocumentRouteOperation>(plan, 5) != nullptr);
        const auto *enterVideo = operationAt<kiriview::EnterVideoDocumentRouteOperation>(plan, 6);
        QVERIFY(enterVideo != nullptr);
        QCOMPARE(enterVideo->url, url);
        const auto *identity
            = operationAt<kiriview::UseOriginalSourceIdentityRouteOperation>(plan, 7);
        QVERIFY(identity != nullptr);
        QCOMPARE(identity->url, url);
        QVERIFY(operationAt<kiriview::RecomputePublicProjectionRouteOperation>(plan, 8) != nullptr);
        QVERIFY(
            operationAt<kiriview::RefreshDirectMediaNavigationAfterRoutingRouteOperation>(plan, 9)
            != nullptr);
        QVERIFY(!hasOperation<kiriview::ClearMediaPredecodeRouteOperation>(plan));
    }
}

void TestDocumentSessionRoutePlan::directImageLocalAndKdeArchiveUrlsRouteToDirectImage()
{
    const QUrl localImage = localUrl(QStringLiteral("/media/page.png"));
    const QUrl archiveImage = QUrl(QStringLiteral("zip:///books/book.zip!/chapter/page.avif"));

    for (const QUrl &url : { localImage, archiveImage }) {
        const kiriview::DocumentSessionRoutePlan plan
            = kiriview::documentSessionRoutePlanForSourceUrl(
                url, kiriview::DocumentSessionKind::Image);

        QVERIFY(kiriview::isSupportedDirectImageUrl(url));
        QCOMPARE(plan.kind, kiriview::DocumentSessionRouteKind::DirectImage);
        QCOMPARE(plan.sourceUrl, url);
        const auto *cursor = operationAt<kiriview::RequestDirectImageCursorRouteOperation>(plan, 4);
        QVERIFY(cursor != nullptr);
        QCOMPARE(cursor->url, url);
        QVERIFY(operationAt<kiriview::LeaveVideoModeRouteOperation>(plan, 5) != nullptr);
        const auto *enterImage = operationAt<kiriview::EnterImageDocumentRouteOperation>(plan, 6);
        QVERIFY(enterImage != nullptr);
        QCOMPARE(enterImage->url, url);
        QVERIFY(operationAt<kiriview::SyncDirectImageCursorFromDocumentRouteOperation>(plan, 7)
            != nullptr);
        QVERIFY(operationAt<kiriview::UseImageDocumentSourceIdentityRouteOperation>(plan, 8)
            != nullptr);
        QVERIFY(operationAt<kiriview::RecomputePublicProjectionRouteOperation>(plan, 9) != nullptr);
        QVERIFY(
            operationAt<kiriview::RefreshDirectMediaNavigationAfterRoutingRouteOperation>(plan, 10)
            != nullptr);
        QVERIFY(!hasOperation<kiriview::ClearMediaPredecodeRouteOperation>(plan));
    }
}

void TestDocumentSessionRoutePlan::localDirectoryAndGeneralArchiveNamesRouteToImageDocument()
{
    for (const QUrl &url :
        { localUrl(QStringLiteral("/books/")), localUrl(QStringLiteral("/books/book.zip")),
            localUrl(QStringLiteral("/books/book.tar")), localUrl(QStringLiteral("/books/book.7z")),
            localUrl(QStringLiteral("/books/book.rar")) }) {
        const kiriview::DocumentSessionRoutePlan plan
            = kiriview::documentSessionRoutePlanForSourceUrl(
                url, kiriview::DocumentSessionKind::Video);

        QCOMPARE(plan.kind, kiriview::DocumentSessionRouteKind::ImageDocument);
        QVERIFY(operationAt<kiriview::ClearDirectMediaCursorRouteOperation>(plan, 4) != nullptr);
        QVERIFY(operationAt<kiriview::LeaveVideoModeRouteOperation>(plan, 5) != nullptr);
        QVERIFY(operationAt<kiriview::EnterImageDocumentRouteOperation>(plan, 6) != nullptr);
        QVERIFY(operationAt<kiriview::SyncDirectImageCursorFromDocumentRouteOperation>(plan, 7)
            != nullptr);
        QVERIFY(
            operationAt<kiriview::RefreshDirectMediaNavigationAfterRoutingRouteOperation>(plan, 10)
            != nullptr);
    }
}

void TestDocumentSessionRoutePlan::unsupportedExtensionRoutesToImageDocument()
{
    const QUrl unsupported = localUrl(QStringLiteral("/media/readme.txt"));
    const kiriview::DocumentSessionRoutePlan plan = kiriview::documentSessionRoutePlanForSourceUrl(
        unsupported, kiriview::DocumentSessionKind::Image);

    QVERIFY(!kiriview::isSupportedDirectVideoUrl(unsupported));
    QVERIFY(!kiriview::isSupportedDirectImageUrl(unsupported));
    QCOMPARE(plan.kind, kiriview::DocumentSessionRouteKind::ImageDocument);
    QVERIFY(operationAt<kiriview::ClearDirectMediaCursorRouteOperation>(plan, 4) != nullptr);
    QVERIFY(operationAt<kiriview::EnterImageDocumentRouteOperation>(plan, 6) != nullptr);
}

void TestDocumentSessionRoutePlan::fileNamesMatchCaseInsensitively()
{
    const QUrl video = localUrl(QStringLiteral("/media/CLIP.M4V"));
    const QUrl image = localUrl(QStringLiteral("/media/SCAN.TIFF"));

    QCOMPARE(
        kiriview::documentSessionRoutePlanForSourceUrl(video, kiriview::DocumentSessionKind::Empty)
            .kind,
        kiriview::DocumentSessionRouteKind::DirectVideo);
    QCOMPARE(
        kiriview::documentSessionRoutePlanForSourceUrl(image, kiriview::DocumentSessionKind::Empty)
            .kind,
        kiriview::DocumentSessionRouteKind::DirectImage);
}

void TestDocumentSessionRoutePlan::sourceRoutesPrepareSessionForTopLevelRouting()
{
    const QUrl image = localUrl(QStringLiteral("/media/page.png"));

    const kiriview::DocumentSessionRoutePlan sourcePlan
        = kiriview::documentSessionRoutePlanForSourceUrl(
            image, kiriview::DocumentSessionKind::Image);
    QVERIFY(operationAt<kiriview::ClearSessionErrorStringRouteOperation>(sourcePlan, 0) != nullptr);
    QVERIFY(
        operationAt<kiriview::CancelDirectMediaNavigationRouteOperation>(sourcePlan, 1) != nullptr);
    QVERIFY(operationAt<kiriview::CancelMediaDeletionRouteOperation>(sourcePlan, 2) != nullptr);

    const kiriview::DocumentSessionRoutePlan mediaPlan
        = kiriview::documentSessionRoutePlanForMediaUrl(
            image, kiriview::DocumentSessionKind::Image);
    QVERIFY(!hasOperation<kiriview::ClearSessionErrorStringRouteOperation>(mediaPlan));
    QVERIFY(!hasOperation<kiriview::CancelDirectMediaNavigationRouteOperation>(mediaPlan));
    QVERIFY(!hasOperation<kiriview::CancelMediaDeletionRouteOperation>(mediaPlan));
}

void TestDocumentSessionRoutePlan::routePlanDoesNotAdvertiseVideoBeyondDirectUrlClassification()
{
    const QUrl archiveRoot = QUrl(QStringLiteral("zip:///books/book.zip!/"));
    const QUrl directoryRoot = localUrl(QStringLiteral("/books/"));
    const QUrl archiveVideoEntry = QUrl(QStringLiteral("zip:///books/book.zip!/clip.mp4"));

    for (const QUrl &url : { archiveRoot, directoryRoot }) {
        const kiriview::DocumentSessionRoutePlan plan
            = kiriview::documentSessionRoutePlanForSourceUrl(
                url, kiriview::DocumentSessionKind::Image);
        QCOMPARE(plan.kind, kiriview::DocumentSessionRouteKind::ImageDocument);
        QVERIFY(!hasOperation<kiriview::EnterVideoDocumentRouteOperation>(plan));
    }

    QCOMPARE(kiriview::documentSessionRoutePlanForSourceUrl(
                 archiveVideoEntry, kiriview::DocumentSessionKind::Image)
                 .kind,
        kiriview::DocumentSessionRouteKind::DirectVideo);
}

void TestDocumentSessionRoutePlan::
    sourceRoutesClearDirectMediaNavigationButMediaRoutesKeepRequestedReadout()
{
    const QUrl image = localUrl(QStringLiteral("/media/page.png"));

    QVERIFY(hasOperation<kiriview::ClearDirectMediaNavigationRouteOperation>(
        kiriview::documentSessionRoutePlanForSourceUrl(
            image, kiriview::DocumentSessionKind::Image)));
    QVERIFY(!hasOperation<kiriview::ClearDirectMediaNavigationRouteOperation>(
        kiriview::documentSessionRoutePlanForMediaUrl(
            image, kiriview::DocumentSessionKind::Image)));
}

void TestDocumentSessionRoutePlan::
    directImageCursorActionPreservesStableCursorOnlyForImageReplacement()
{
    const QUrl image = localUrl(QStringLiteral("/media/page.png"));

    QVERIFY(operationAt<kiriview::RequestDirectImageCursorRouteOperation>(
                kiriview::documentSessionRoutePlanForSourceUrl(
                    image, kiriview::DocumentSessionKind::Image),
                4)
        != nullptr);
    QVERIFY(operationAt<kiriview::ClearThenRequestDirectImageCursorRouteOperation>(
                kiriview::documentSessionRoutePlanForSourceUrl(
                    image, kiriview::DocumentSessionKind::Video),
                4)
        != nullptr);
    QVERIFY(operationAt<kiriview::ClearThenRequestDirectImageCursorRouteOperation>(
                kiriview::documentSessionRoutePlanForSourceUrl(
                    image, kiriview::DocumentSessionKind::Empty),
                4)
        != nullptr);
}

void TestDocumentSessionRoutePlan::deletedImageFallbackRoutesAfterClearingImageDocument()
{
    const QUrl fallback = localUrl(QStringLiteral("/media/fallback.png"));

    const kiriview::DocumentSessionRoutePlan plan
        = kiriview::documentSessionRoutePlanAfterMediaDeletion(
            kiriview::DocumentSessionKind::Image, fallback);

    QCOMPARE(plan.kind, kiriview::DocumentSessionRouteKind::DirectImage);
    QCOMPARE(plan.sourceUrl, fallback);
    QVERIFY(operationAt<kiriview::ClearImageDocumentRouteOperation>(plan, 0) != nullptr);
    const auto *cursor = operationAt<kiriview::RequestDirectImageCursorRouteOperation>(plan, 1);
    QVERIFY(cursor != nullptr);
    QCOMPARE(cursor->url, fallback);
    QVERIFY(!hasOperation<kiriview::ClearDirectMediaNavigationRouteOperation>(plan));
    QVERIFY(!hasOperation<kiriview::ClearMediaPredecodeRouteOperation>(plan));
}

void TestDocumentSessionRoutePlan::deletedVideoFallbackRoutesFromEmptySession()
{
    const QUrl fallback = localUrl(QStringLiteral("/media/fallback.png"));

    const kiriview::DocumentSessionRoutePlan plan
        = kiriview::documentSessionRoutePlanAfterMediaDeletion(
            kiriview::DocumentSessionKind::Video, fallback);

    QCOMPARE(plan.kind, kiriview::DocumentSessionRouteKind::DirectImage);
    QCOMPARE(plan.sourceUrl, fallback);
    QVERIFY(operationAt<kiriview::LeaveVideoModeRouteOperation>(plan, 0) != nullptr);
    QVERIFY(operationAt<kiriview::EnterEmptyDocumentRouteOperation>(plan, 1) != nullptr);
    const auto *cursor
        = operationAt<kiriview::ClearThenRequestDirectImageCursorRouteOperation>(plan, 2);
    QVERIFY(cursor != nullptr);
    QCOMPARE(cursor->url, fallback);
}

void TestDocumentSessionRoutePlan::deletedMediaWithoutFallbackClearsNavigationAndPredecode()
{
    const kiriview::DocumentSessionRoutePlan plan
        = kiriview::documentSessionRoutePlanAfterMediaDeletion(
            kiriview::DocumentSessionKind::Video, std::nullopt);

    QCOMPARE(plan.kind, kiriview::DocumentSessionRouteKind::Empty);
    QVERIFY(plan.sourceUrl.isEmpty());
    QCOMPARE(plan.operations.size(), std::size_t(8));
    QVERIFY(operationAt<kiriview::ClearDirectMediaNavigationRouteOperation>(plan, 0) != nullptr);
    QVERIFY(operationAt<kiriview::ClearDirectMediaCursorRouteOperation>(plan, 1) != nullptr);
    QVERIFY(operationAt<kiriview::LeaveVideoModeRouteOperation>(plan, 2) != nullptr);
    QVERIFY(operationAt<kiriview::ClearImageDocumentRouteOperation>(plan, 3) != nullptr);
    QVERIFY(operationAt<kiriview::EnterEmptyDocumentRouteOperation>(plan, 4) != nullptr);
    QVERIFY(operationAt<kiriview::ClearSourceIdentityRouteOperation>(plan, 5) != nullptr);
    QVERIFY(operationAt<kiriview::RecomputePublicProjectionRouteOperation>(plan, 6) != nullptr);
    QVERIFY(operationAt<kiriview::ClearMediaPredecodeRouteOperation>(plan, 7) != nullptr);
}

void TestDocumentSessionRoutePlan::routePlansKeepMutationPublicationFollowUpOrder()
{
    verifyRoutePhaseOrder(kiriview::documentSessionRoutePlanForSourceUrl(
        QUrl(), kiriview::DocumentSessionKind::Video));
    verifyRoutePhaseOrder(kiriview::documentSessionRoutePlanForSourceUrl(
        localUrl(QStringLiteral("/media/page.png")), kiriview::DocumentSessionKind::Image));
    verifyRoutePhaseOrder(kiriview::documentSessionRoutePlanForSourceUrl(
        localUrl(QStringLiteral("/media/clip.mp4")), kiriview::DocumentSessionKind::Image));
    verifyRoutePhaseOrder(kiriview::documentSessionRoutePlanForSourceUrl(
        localUrl(QStringLiteral("/books/")), kiriview::DocumentSessionKind::Video));
    verifyRoutePhaseOrder(kiriview::documentSessionRoutePlanAfterMediaDeletion(
        kiriview::DocumentSessionKind::Image, localUrl(QStringLiteral("/media/fallback.png"))));
    verifyRoutePhaseOrder(kiriview::documentSessionRoutePlanAfterMediaDeletion(
        kiriview::DocumentSessionKind::Video, std::nullopt));
}

QTEST_GUILESS_MAIN(TestDocumentSessionRoutePlan)

#include "test_documentsessionrouteplan.moc"

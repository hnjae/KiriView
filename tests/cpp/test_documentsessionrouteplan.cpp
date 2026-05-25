// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionrouteplan.h"

#include "navigation/mediaformatregistry.h"

#include <QObject>
#include <QTest>
#include <QUrl>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }
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
    void routePlanDoesNotAdvertiseVideoBeyondDirectUrlClassification();
    void sourceRoutesClearMediaNavigationButMediaRoutesKeepRequestedReadout();
    void directImageCursorActionPreservesStableCursorOnlyForImageReplacement();
};

void TestDocumentSessionRoutePlan::emptyUrlProducesEmptyClearPlan()
{
    const KiriView::DocumentSessionRoutePlan plan = KiriView::documentSessionRoutePlanForSourceUrl(
        QUrl(), KiriView::DocumentSessionKind::Video);

    QCOMPARE(plan.kind, KiriView::DocumentSessionRouteKind::Empty);
    QVERIFY(plan.sourceUrl.isEmpty());
    QCOMPARE(plan.cursorAction, KiriView::DocumentSessionRouteCursorAction::Clear);
    QCOMPARE(plan.sourceIdentityAction, KiriView::DocumentSessionRouteSourceIdentityAction::Clear);
    QVERIFY(plan.mediaNavigation.clearBeforeRouting);
    QVERIFY(!plan.mediaNavigation.refreshAfterRouting);
    QCOMPARE(plan.document.clear, KiriView::DocumentSessionRouteDocumentClear::ImageAndVideo);
    QCOMPARE(plan.document.enter, KiriView::DocumentSessionRouteDocumentEnter::Empty);
    QVERIFY(!plan.document.syncDirectImageCursorFromDocument);
    QVERIFY(plan.predecode.clear);
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
        QCOMPARE(plan.cursorAction, KiriView::DocumentSessionRouteCursorAction::SetDirectVideo);
        QCOMPARE(plan.sourceIdentityAction,
            KiriView::DocumentSessionRouteSourceIdentityAction::UseOriginalUrl);
        QCOMPARE(plan.document.clear, KiriView::DocumentSessionRouteDocumentClear::Image);
        QCOMPARE(plan.document.enter, KiriView::DocumentSessionRouteDocumentEnter::Video);
        QVERIFY(plan.mediaNavigation.refreshAfterRouting);
        QVERIFY(!plan.predecode.clear);
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
        QCOMPARE(plan.cursorAction, KiriView::DocumentSessionRouteCursorAction::RequestDirectImage);
        QCOMPARE(plan.sourceIdentityAction,
            KiriView::DocumentSessionRouteSourceIdentityAction::UseImageDocumentSourceUrl);
        QCOMPARE(plan.document.clear, KiriView::DocumentSessionRouteDocumentClear::Video);
        QCOMPARE(plan.document.enter, KiriView::DocumentSessionRouteDocumentEnter::Image);
        QVERIFY(plan.document.syncDirectImageCursorFromDocument);
        QVERIFY(plan.mediaNavigation.refreshAfterRouting);
        QVERIFY(!plan.predecode.clear);
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
        QCOMPARE(plan.cursorAction, KiriView::DocumentSessionRouteCursorAction::Clear);
        QCOMPARE(plan.document.clear, KiriView::DocumentSessionRouteDocumentClear::Video);
        QCOMPARE(plan.document.enter, KiriView::DocumentSessionRouteDocumentEnter::Image);
        QVERIFY(plan.document.syncDirectImageCursorFromDocument);
        QVERIFY(plan.mediaNavigation.refreshAfterRouting);
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
    QCOMPARE(plan.cursorAction, KiriView::DocumentSessionRouteCursorAction::Clear);
    QCOMPARE(plan.document.enter, KiriView::DocumentSessionRouteDocumentEnter::Image);
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
        QVERIFY(plan.document.enter != KiriView::DocumentSessionRouteDocumentEnter::Video);
    }

    QCOMPARE(KiriView::documentSessionRoutePlanForSourceUrl(
                 archiveVideoEntry, KiriView::DocumentSessionKind::Image)
                 .kind,
        KiriView::DocumentSessionRouteKind::DirectVideo);
}

void TestDocumentSessionRoutePlan::
    sourceRoutesClearMediaNavigationButMediaRoutesKeepRequestedReadout()
{
    const QUrl image = localUrl(QStringLiteral("/media/page.png"));

    QVERIFY(
        KiriView::documentSessionRoutePlanForSourceUrl(image, KiriView::DocumentSessionKind::Image)
            .mediaNavigation.clearBeforeRouting);
    QVERIFY(
        !KiriView::documentSessionRoutePlanForMediaUrl(image, KiriView::DocumentSessionKind::Image)
            .mediaNavigation.clearBeforeRouting);
}

void TestDocumentSessionRoutePlan::
    directImageCursorActionPreservesStableCursorOnlyForImageReplacement()
{
    const QUrl image = localUrl(QStringLiteral("/media/page.png"));

    QCOMPARE(
        KiriView::documentSessionRoutePlanForSourceUrl(image, KiriView::DocumentSessionKind::Image)
            .cursorAction,
        KiriView::DocumentSessionRouteCursorAction::RequestDirectImage);
    QCOMPARE(
        KiriView::documentSessionRoutePlanForSourceUrl(image, KiriView::DocumentSessionKind::Video)
            .cursorAction,
        KiriView::DocumentSessionRouteCursorAction::ClearThenRequestDirectImage);
    QCOMPARE(
        KiriView::documentSessionRoutePlanForSourceUrl(image, KiriView::DocumentSessionKind::Empty)
            .cursorAction,
        KiriView::DocumentSessionRouteCursorAction::ClearThenRequestDirectImage);
}

QTEST_GUILESS_MAIN(TestDocumentSessionRoutePlan)

#include "test_documentsessionrouteplan.moc"

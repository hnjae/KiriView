// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/mediaopenwithtarget.h"

#include <QTest>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::ImagePageScopeLocation archiveScope(const QString &scheme)
{
    QUrl rootUrl;
    rootUrl.setScheme(scheme);
    rootUrl.setPath(QStringLiteral("/books/book.%1/").arg(scheme));
    return KiriView::ImagePageScopeLocation::fromUrls(
        localUrl(QStringLiteral("/books/book.%1").arg(scheme)), rootUrl,
        KiriView::ImagePageScopeKind::ComicBookArchive);
}
}

class TestMediaOpenWithTarget : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyAndUnreadyDocumentsHaveNoTarget();
    void directImageUsesDisplayedUrl();
    void directVideoUsesSourceUrl();
    void directoryDocumentUsesCurrentImageUrl();
    void kioSupportedArchiveDocumentsUseCurrentImageUrl_data();
    void kioSupportedArchiveDocumentsUseCurrentImageUrl();
    void rarArchiveDocumentsHaveNoTarget();
};

void TestMediaOpenWithTarget::emptyAndUnreadyDocumentsHaveNoTarget()
{
    QVERIFY(KiriView::mediaOpenWithTargetUrl({}).isEmpty());

    QVERIFY(KiriView::mediaOpenWithTargetUrl(KiriView::MediaOpenWithTargetInput {
                                                 KiriView::DocumentSessionKind::Image,
                                                 false,
                                                 localUrl(QStringLiteral("/images/page.png")),
                                             })
            .isEmpty());

    QVERIFY(KiriView::mediaOpenWithTargetUrl(KiriView::MediaOpenWithTargetInput {
                                                 KiriView::DocumentSessionKind::Video,
                                                 false,
                                                 {},
                                                 {},
                                                 false,
                                                 localUrl(QStringLiteral("/videos/clip.mp4")),
                                             })
            .isEmpty());
}

void TestMediaOpenWithTarget::directImageUsesDisplayedUrl()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    QCOMPARE(KiriView::mediaOpenWithTargetUrl(KiriView::MediaOpenWithTargetInput {
                 KiriView::DocumentSessionKind::Image,
                 true,
                 imageUrl,
             }),
        imageUrl);
}

void TestMediaOpenWithTarget::directVideoUsesSourceUrl()
{
    const QUrl videoUrl = localUrl(QStringLiteral("/videos/clip.mp4"));
    QCOMPARE(KiriView::mediaOpenWithTargetUrl(KiriView::MediaOpenWithTargetInput {
                 KiriView::DocumentSessionKind::Video,
                 false,
                 {},
                 {},
                 true,
                 videoUrl,
             }),
        videoUrl);
}

void TestMediaOpenWithTarget::directoryDocumentUsesCurrentImageUrl()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/book/page.png"));
    const KiriView::ImagePageScopeLocation scope
        = KiriView::ImagePageScopeLocation::fromUrls(localUrl(QStringLiteral("/book")),
            localUrl(QStringLiteral("/book")), KiriView::ImagePageScopeKind::Directory);
    QCOMPARE(KiriView::mediaOpenWithTargetUrl(KiriView::MediaOpenWithTargetInput {
                 KiriView::DocumentSessionKind::Image,
                 true,
                 imageUrl,
                 scope,
             }),
        imageUrl);
}

void TestMediaOpenWithTarget::kioSupportedArchiveDocumentsUseCurrentImageUrl_data()
{
    QTest::addColumn<QString>("scheme");

    QTest::newRow("zip") << QStringLiteral("zip");
    QTest::newRow("tar") << QStringLiteral("tar");
    QTest::newRow("sevenz") << QStringLiteral("sevenz");
}

void TestMediaOpenWithTarget::kioSupportedArchiveDocumentsUseCurrentImageUrl()
{
    QFETCH(QString, scheme);

    QUrl imageUrl;
    imageUrl.setScheme(scheme);
    imageUrl.setPath(QStringLiteral("/books/book.%1/page.png").arg(scheme));
    QCOMPARE(KiriView::mediaOpenWithTargetUrl(KiriView::MediaOpenWithTargetInput {
                 KiriView::DocumentSessionKind::Image,
                 true,
                 imageUrl,
                 archiveScope(scheme),
             }),
        imageUrl);
}

void TestMediaOpenWithTarget::rarArchiveDocumentsHaveNoTarget()
{
    QUrl imageUrl;
    imageUrl.setScheme(QStringLiteral("rar"));
    imageUrl.setPath(QStringLiteral("/books/book.rar/page.png"));
    QVERIFY(KiriView::mediaOpenWithTargetUrl(KiriView::MediaOpenWithTargetInput {
                                                 KiriView::DocumentSessionKind::Image,
                                                 true,
                                                 imageUrl,
                                                 archiveScope(QStringLiteral("rar")),
                                             })
            .isEmpty());
}

QTEST_GUILESS_MAIN(TestMediaOpenWithTarget)

#include "test_mediaopenwithtarget.moc"

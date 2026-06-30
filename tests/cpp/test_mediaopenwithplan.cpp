// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/mediaopenwithplan.h"

#include <QTest>

namespace {
QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }

kiriview::OpenedCollectionScopeLocation archiveScope(const QString& scheme)
{
    QUrl rootUrl;
    rootUrl.setScheme(scheme);
    rootUrl.setPath(QStringLiteral("/books/book.%1/").arg(scheme));
    return kiriview::OpenedCollectionScopeLocation::fromUrls(
        localUrl(QStringLiteral("/books/book.%1").arg(scheme)), rootUrl,
        kiriview::OpenedCollectionScopeKind::ComicBookArchive);
}
}

class TestMediaOpenWithPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyAndUnreadyDocumentsHaveNoRequest();
    void directImageUsesDisplayedUrl();
    void directVideoUsesSourceUrl();
    void directoryCollectionUsesCurrentImageUrl();
    void kioSupportedArchiveCollectionVideoUsesCurrentVideoUrl();
    void kioSupportedArchiveCollectionsUseCurrentImageUrl_data();
    void kioSupportedArchiveCollectionsUseCurrentImageUrl();
    void rarArchiveCollectionsHaveNoTarget();
    void rarArchiveCollectionVideoHasNoTarget();
};

namespace {
void expectNoRequest(const kiriview::MediaOpenWithPlan& plan)
{
    QVERIFY(!plan.hasRequest());
    QVERIFY(!plan.request.has_value());
}

void expectRequestTarget(const kiriview::MediaOpenWithPlan& plan, const QUrl& targetUrl)
{
    QVERIFY(plan.hasRequest());
    QVERIFY(plan.request.has_value());
    QCOMPARE(plan.request->targetUrl, targetUrl);
}
}

void TestMediaOpenWithPlan::emptyAndUnreadyDocumentsHaveNoRequest()
{
    expectNoRequest(kiriview::mediaOpenWithPlan({}));

    expectNoRequest(kiriview::mediaOpenWithPlan(kiriview::MediaOpenWithPlanInput {
        kiriview::DocumentSessionKind::Image,
        false,
        localUrl(QStringLiteral("/images/page.png")),
    }));

    expectNoRequest(kiriview::mediaOpenWithPlan(kiriview::MediaOpenWithPlanInput {
        kiriview::DocumentSessionKind::Video,
        false,
        {},
        {},
        false,
        localUrl(QStringLiteral("/videos/clip.mp4")),
    }));
}

void TestMediaOpenWithPlan::directImageUsesDisplayedUrl()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    expectRequestTarget(kiriview::mediaOpenWithPlan(kiriview::MediaOpenWithPlanInput {
                            kiriview::DocumentSessionKind::Image,
                            true,
                            imageUrl,
                        }),
        imageUrl);
}

void TestMediaOpenWithPlan::directVideoUsesSourceUrl()
{
    const QUrl videoUrl = localUrl(QStringLiteral("/videos/clip.mp4"));
    expectRequestTarget(kiriview::mediaOpenWithPlan(kiriview::MediaOpenWithPlanInput {
                            kiriview::DocumentSessionKind::Video,
                            false,
                            {},
                            {},
                            true,
                            videoUrl,
                        }),
        videoUrl);
}

void TestMediaOpenWithPlan::directoryCollectionUsesCurrentImageUrl()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/book/page.png"));
    const kiriview::OpenedCollectionScopeLocation scope
        = kiriview::OpenedCollectionScopeLocation::fromUrls(localUrl(QStringLiteral("/book")),
            localUrl(QStringLiteral("/book")), kiriview::OpenedCollectionScopeKind::Directory);
    expectRequestTarget(kiriview::mediaOpenWithPlan(kiriview::MediaOpenWithPlanInput {
                            kiriview::DocumentSessionKind::Image,
                            true,
                            imageUrl,
                            scope,
                        }),
        imageUrl);
}

void TestMediaOpenWithPlan::kioSupportedArchiveCollectionVideoUsesCurrentVideoUrl()
{
    const QUrl videoUrl(QStringLiteral("zip:///books/book.zip!/chapter/clip.mp4"));
    expectRequestTarget(kiriview::mediaOpenWithPlan(kiriview::MediaOpenWithPlanInput {
                            kiriview::DocumentSessionKind::Video,
                            false,
                            {},
                            archiveScope(QStringLiteral("zip")),
                            true,
                            videoUrl,
                        }),
        videoUrl);
}

void TestMediaOpenWithPlan::kioSupportedArchiveCollectionsUseCurrentImageUrl_data()
{
    QTest::addColumn<QString>("scheme");

    QTest::newRow("zip") << QStringLiteral("zip");
    QTest::newRow("tar") << QStringLiteral("tar");
    QTest::newRow("sevenz") << QStringLiteral("sevenz");
}

void TestMediaOpenWithPlan::kioSupportedArchiveCollectionsUseCurrentImageUrl()
{
    QFETCH(QString, scheme);

    QUrl imageUrl;
    imageUrl.setScheme(scheme);
    imageUrl.setPath(QStringLiteral("/books/book.%1/page.png").arg(scheme));
    expectRequestTarget(kiriview::mediaOpenWithPlan(kiriview::MediaOpenWithPlanInput {
                            kiriview::DocumentSessionKind::Image,
                            true,
                            imageUrl,
                            archiveScope(scheme),
                        }),
        imageUrl);
}

void TestMediaOpenWithPlan::rarArchiveCollectionsHaveNoTarget()
{
    QUrl imageUrl;
    imageUrl.setScheme(QStringLiteral("rar"));
    imageUrl.setPath(QStringLiteral("/books/book.rar/page.png"));
    expectNoRequest(kiriview::mediaOpenWithPlan(kiriview::MediaOpenWithPlanInput {
        kiriview::DocumentSessionKind::Image,
        true,
        imageUrl,
        archiveScope(QStringLiteral("rar")),
    }));
}

void TestMediaOpenWithPlan::rarArchiveCollectionVideoHasNoTarget()
{
    const QUrl videoUrl(QStringLiteral("rar:///books/book.rar!/chapter/clip.mp4"));
    expectNoRequest(kiriview::mediaOpenWithPlan(kiriview::MediaOpenWithPlanInput {
        kiriview::DocumentSessionKind::Video,
        false,
        {},
        archiveScope(QStringLiteral("rar")),
        true,
        videoUrl,
    }));
}

QTEST_GUILESS_MAIN(TestMediaOpenWithPlan)

#include "test_mediaopenwithplan.moc"

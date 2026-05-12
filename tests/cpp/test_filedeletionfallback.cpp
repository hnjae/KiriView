// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletionfallback.h"

#include "image_test_support.h"
#include "imagecontainer.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <variant>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::comicBookContainerCandidate;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;
}

class TestFileDeletionFallback : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void regularImagePlanUsesSiblingImageContext();
    void comicBookPagePlanUsesArchiveContainer();
    void generalArchivePageHasNoFallbackPlan();
    void imageFallbackPrefersNextImage();
    void imageFallbackFallsBackToPreviousImage();
    void comicBookFallbackKeepsNextAndPreviousCandidates();
};

void TestFileDeletionFallback::regularImagePlanUsesSiblingImageContext()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/02.png"));

    const KiriView::DeletionFallbackPlan plan = KiriView::deletionFallbackPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromUrl(imageUrl));
    const auto *imagePlan = std::get_if<KiriView::ImageDeletionFallbackPlan>(&plan);

    QVERIFY(imagePlan != nullptr);
    QCOMPARE(imagePlan->currentUrl, imageUrl);
    QCOMPARE(imagePlan->currentName, QStringLiteral("02.png"));
    QCOMPARE(imagePlan->imageContext.currentUrl(), imageUrl);
}

void TestFileDeletionFallback::comicBookPagePlanUsesArchiveContainer()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    const KiriView::DeletionFallbackPlan plan = KiriView::deletionFallbackPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, *archiveDocument));
    const auto *comicBookPlan = std::get_if<KiriView::ComicBookDeletionFallbackPlan>(&plan);

    QVERIFY(comicBookPlan != nullptr);
    QCOMPARE(comicBookPlan->currentContainerUrl, archiveUrl);
    QCOMPARE(comicBookPlan->currentName, QStringLiteral("book.cbz"));
}

void TestFileDeletionFallback::generalArchivePageHasNoFallbackPlan()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/archives/document.zip"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    const KiriView::DeletionFallbackPlan plan = KiriView::deletionFallbackPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, *archiveDocument));

    QVERIFY(std::holds_alternative<KiriView::NoDeletionFallbackPlan>(plan));
}

void TestFileDeletionFallback::imageFallbackPrefersNextImage()
{
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const KiriView::ImageDeletionFallbackPlan plan {
        KiriView::ImageCandidateListContext::forDirectory(
            currentUrl, localUrl(QStringLiteral("/images/"))),
        currentUrl,
        QStringLiteral("02.png"),
    };

    const std::optional<QUrl> fallbackUrl = KiriView::imageDeletionFallbackUrl(
        {
            imageCandidate(localUrl(QStringLiteral("/images/01.png"))),
            imageCandidate(localUrl(QStringLiteral("/images/03.png"))),
        },
        plan);

    QVERIFY(fallbackUrl.has_value());
    QCOMPARE(*fallbackUrl, localUrl(QStringLiteral("/images/03.png")));
}

void TestFileDeletionFallback::imageFallbackFallsBackToPreviousImage()
{
    const QUrl currentUrl = localUrl(QStringLiteral("/images/03.png"));
    const KiriView::ImageDeletionFallbackPlan plan {
        KiriView::ImageCandidateListContext::forDirectory(
            currentUrl, localUrl(QStringLiteral("/images/"))),
        currentUrl,
        QStringLiteral("03.png"),
    };

    const std::optional<QUrl> fallbackUrl = KiriView::imageDeletionFallbackUrl(
        {
            imageCandidate(localUrl(QStringLiteral("/images/01.png"))),
            imageCandidate(localUrl(QStringLiteral("/images/02.png"))),
        },
        plan);

    QVERIFY(fallbackUrl.has_value());
    QCOMPARE(*fallbackUrl, localUrl(QStringLiteral("/images/02.png")));
}

void TestFileDeletionFallback::comicBookFallbackKeepsNextAndPreviousCandidates()
{
    const KiriView::ComicBookDeletionFallbackPlan plan {
        localUrl(QStringLiteral("/books/b.cbz")),
        QStringLiteral("b.cbz"),
    };

    const KiriView::ComicBookDeletionFallbackCandidates candidates
        = KiriView::comicBookDeletionFallbackCandidates(
            {
                comicBookContainerCandidate(localUrl(QStringLiteral("/books/a.cbz"))),
                comicBookContainerCandidate(localUrl(QStringLiteral("/books/c.cbz"))),
            },
            plan);

    QVERIFY(candidates.preferred.has_value());
    QCOMPARE(candidates.preferred->url, localUrl(QStringLiteral("/books/c.cbz")));
    QVERIFY(candidates.fallback.has_value());
    QCOMPARE(candidates.fallback->url, localUrl(QStringLiteral("/books/a.cbz")));
}

QTEST_GUILESS_MAIN(TestFileDeletionFallback)

#include "test_filedeletionfallback.moc"

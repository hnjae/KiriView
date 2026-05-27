// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imageremovalfallback.h"

#include "candidate_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTemporaryDir>
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

class TestImageRemovalFallback : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyLocationHasNoRemovalPlanTarget();
    void regularImagePlanTargetsDisplayedUrl();
    void explicitKioArchiveImagePlanTargetsDisplayedUrl();
    void directArchiveDocumentPlanTargetsArchiveFile();
    void directDirectoryDocumentPlanTargetsDirectory();
    void regularImagePlanUsesSiblingImageContext();
    void comicBookPagePlanUsesArchiveContainer();
    void generalArchivePageHasNoFallbackPlan();
    void directoryDocumentPageHasNoFallbackPlan();
    void imageFallbackPrefersNextImage();
    void imageFallbackFallsBackToPreviousImage();
    void imageFallbackReturnsNoUrlWithoutSiblingImages();
    void comicBookFallbackKeepsNextAndPreviousCandidates();
    void comicBookFallbackReturnsNoCandidatesWithoutSiblingContainers();
};

void TestImageRemovalFallback::emptyLocationHasNoRemovalPlanTarget()
{
    const KiriView::ImageRemovalPlan plan
        = KiriView::imageRemovalPlanForDisplayedLocation(KiriView::DisplayedImageLocation());

    QVERIFY(!plan.hasTarget());
    QVERIFY(plan.targetUrl.isEmpty());
    QVERIFY(std::holds_alternative<KiriView::NoImageRemovalFallback>(plan.fallbackPlan));
}

void TestImageRemovalFallback::regularImagePlanTargetsDisplayedUrl()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));

    const KiriView::ImageRemovalPlan plan = KiriView::imageRemovalPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromUrl(imageUrl));

    QVERIFY(plan.hasTarget());
    QCOMPARE(plan.targetUrl, imageUrl);
}

void TestImageRemovalFallback::explicitKioArchiveImagePlanTargetsDisplayedUrl()
{
    QUrl imageUrl;
    imageUrl.setScheme(QStringLiteral("zip"));
    imageUrl.setPath(QStringLiteral("/books/book.zip/page.png"));

    const KiriView::ImageRemovalPlan plan = KiriView::imageRemovalPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromUrl(imageUrl));

    QVERIFY(plan.hasTarget());
    QCOMPARE(plan.targetUrl, imageUrl);
}

void TestImageRemovalFallback::directArchiveDocumentPlanTargetsArchiveFile()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    const KiriView::ImageRemovalPlan plan = KiriView::imageRemovalPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromImagePageScope(pageUrl, *archiveDocument));

    QVERIFY(plan.hasTarget());
    QCOMPARE(plan.targetUrl, archiveUrl);
}

void TestImageRemovalFallback::directDirectoryDocumentPlanTargetsDirectory()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = QUrl::fromLocalFile(directory.path());
    const std::optional<KiriView::ImagePageScopeLocation> directoryDocument
        = KiriView::directOpenImagePageScopeLocationForLocalUrl(directoryUrl);
    QVERIFY(directoryDocument.has_value());
    QUrl pageUrl = directoryDocument->rootUrl();
    pageUrl.setPath(directoryDocument->rootUrl().path() + QStringLiteral("page.png"));

    const KiriView::ImageRemovalPlan plan = KiriView::imageRemovalPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromImagePageScope(pageUrl, *directoryDocument));

    QVERIFY(plan.hasTarget());
    QCOMPARE(plan.targetUrl, directoryUrl);
}

void TestImageRemovalFallback::regularImagePlanUsesSiblingImageContext()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/02.png"));

    const KiriView::ImageRemovalPlan plan = KiriView::imageRemovalPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromUrl(imageUrl));
    const auto *imagePlan = std::get_if<KiriView::ImageRemovalFallback>(&plan.fallbackPlan);

    QVERIFY(imagePlan != nullptr);
    QCOMPARE(imagePlan->currentUrl, imageUrl);
    QCOMPARE(imagePlan->currentName, QStringLiteral("02.png"));
    QCOMPARE(imagePlan->imageContext.currentUrl(), imageUrl);
}

void TestImageRemovalFallback::comicBookPagePlanUsesArchiveContainer()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    const KiriView::ImageRemovalPlan plan = KiriView::imageRemovalPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromImagePageScope(pageUrl, *archiveDocument));
    const auto *comicBookPlan = std::get_if<KiriView::ComicBookRemovalFallback>(&plan.fallbackPlan);

    QVERIFY(comicBookPlan != nullptr);
    QCOMPARE(comicBookPlan->currentContainerUrl, archiveUrl);
    QCOMPARE(comicBookPlan->candidateDirectoryUrl, localUrl(QStringLiteral("/books/")));
    QCOMPARE(comicBookPlan->currentName, QStringLiteral("book.cbz"));
}

void TestImageRemovalFallback::generalArchivePageHasNoFallbackPlan()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/archives/document.zip"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    const KiriView::ImageRemovalPlan plan = KiriView::imageRemovalPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromImagePageScope(pageUrl, *archiveDocument));

    QVERIFY(std::holds_alternative<KiriView::NoImageRemovalFallback>(plan.fallbackPlan));
}

void TestImageRemovalFallback::directoryDocumentPageHasNoFallbackPlan()
{
    const KiriView::ImagePageScopeLocation directoryDocument
        = KiriView::ImagePageScopeLocation::fromUrls(localUrl(QStringLiteral("/books/")),
            localUrl(QStringLiteral("/books/")), KiriView::ImagePageScopeKind::Directory);
    const QUrl pageUrl = localUrl(QStringLiteral("/books/page.png"));

    const KiriView::ImageRemovalPlan plan = KiriView::imageRemovalPlanForDisplayedLocation(
        KiriView::DisplayedImageLocation::fromImagePageScope(pageUrl, directoryDocument));

    QVERIFY(std::holds_alternative<KiriView::NoImageRemovalFallback>(plan.fallbackPlan));
}

void TestImageRemovalFallback::imageFallbackPrefersNextImage()
{
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const KiriView::ImageRemovalFallback fallback {
        KiriView::ImageCandidateListContext::forDirectory(
            currentUrl, localUrl(QStringLiteral("/images/"))),
        currentUrl,
        QStringLiteral("02.png"),
    };

    const std::optional<KiriView::ImageNavigationTarget> fallbackTarget
        = KiriView::imageRemovalFallbackTarget(
            {
                imageCandidate(localUrl(QStringLiteral("/images/01.png"))),
                KiriView::ImageNavigationCandidate { localUrl(QStringLiteral("/images/03.bin")),
                    QStringLiteral("03.bin"), KiriView::ImageNavigationCandidateKind::Video },
            },
            fallback);

    QVERIFY(fallbackTarget.has_value());
    QCOMPARE(fallbackTarget->url, localUrl(QStringLiteral("/images/03.bin")));
    QCOMPARE(fallbackTarget->kind, KiriView::ImageNavigationCandidateKind::Video);
}

void TestImageRemovalFallback::imageFallbackFallsBackToPreviousImage()
{
    const QUrl currentUrl = localUrl(QStringLiteral("/images/03.png"));
    const KiriView::ImageRemovalFallback fallback {
        KiriView::ImageCandidateListContext::forDirectory(
            currentUrl, localUrl(QStringLiteral("/images/"))),
        currentUrl,
        QStringLiteral("03.png"),
    };

    const std::optional<KiriView::ImageNavigationTarget> fallbackTarget
        = KiriView::imageRemovalFallbackTarget(
            {
                imageCandidate(localUrl(QStringLiteral("/images/01.png"))),
                imageCandidate(localUrl(QStringLiteral("/images/02.png"))),
            },
            fallback);

    QVERIFY(fallbackTarget.has_value());
    QCOMPARE(fallbackTarget->url, localUrl(QStringLiteral("/images/02.png")));
    QCOMPARE(fallbackTarget->kind, KiriView::ImageNavigationCandidateKind::Image);
}

void TestImageRemovalFallback::imageFallbackReturnsNoUrlWithoutSiblingImages()
{
    const QUrl currentUrl = localUrl(QStringLiteral("/images/03.png"));
    const KiriView::ImageRemovalFallback fallback {
        KiriView::ImageCandidateListContext::forDirectory(
            currentUrl, localUrl(QStringLiteral("/images/"))),
        currentUrl,
        QStringLiteral("03.png"),
    };

    const std::optional<KiriView::ImageNavigationTarget> fallbackTarget
        = KiriView::imageRemovalFallbackTarget({}, fallback);

    QVERIFY(!fallbackTarget.has_value());
}

void TestImageRemovalFallback::comicBookFallbackKeepsNextAndPreviousCandidates()
{
    const KiriView::ComicBookRemovalFallback fallback {
        localUrl(QStringLiteral("/books/b.cbz")),
        localUrl(QStringLiteral("/books/")),
        QStringLiteral("b.cbz"),
    };

    const KiriView::ComicBookRemovalFallbackCandidates candidates
        = KiriView::comicBookRemovalFallbackCandidates(
            {
                comicBookContainerCandidate(localUrl(QStringLiteral("/books/a.cbz"))),
                comicBookContainerCandidate(localUrl(QStringLiteral("/books/c.cbz"))),
            },
            fallback);

    QVERIFY(candidates.preferred.has_value());
    QCOMPARE(candidates.preferred->url, localUrl(QStringLiteral("/books/c.cbz")));
    QVERIFY(candidates.fallback.has_value());
    QCOMPARE(candidates.fallback->url, localUrl(QStringLiteral("/books/a.cbz")));
}

void TestImageRemovalFallback::comicBookFallbackReturnsNoCandidatesWithoutSiblingContainers()
{
    const KiriView::ComicBookRemovalFallback fallback {
        localUrl(QStringLiteral("/books/b.cbz")),
        localUrl(QStringLiteral("/books/")),
        QStringLiteral("b.cbz"),
    };

    const KiriView::ComicBookRemovalFallbackCandidates candidates
        = KiriView::comicBookRemovalFallbackCandidates({}, fallback);

    QVERIFY(!candidates.preferred.has_value());
    QVERIFY(!candidates.fallback.has_value());
}

QTEST_GUILESS_MAIN(TestImageRemovalFallback)

#include "test_imageremovalfallback.moc"

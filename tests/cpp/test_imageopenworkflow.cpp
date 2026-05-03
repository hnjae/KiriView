// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <variant>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::ImageLoadSession loadSession(const QUrl &sourceUrl, const QUrl &imageUrl,
    const KiriView::ArchiveDocumentLocation &archiveDocument
    = KiriView::ArchiveDocumentLocation::none(),
    const QUrl &containerNavigationUrl = QUrl())
{
    return KiriView::ImageLoadSession {
        1,
        KiriView::ImageLoadRequest::fromLocation(
            sourceUrl, archiveDocument, containerNavigationUrl),
        archiveDocument.isEmpty()
            ? KiriView::DisplayedImageLocation::fromUrl(imageUrl)
            : KiriView::DisplayedImageLocation::fromArchiveDocument(imageUrl, archiveDocument),
    };
}

template <typename Effect> const Effect *findEffect(const KiriView::ImageDocumentEffects &effects)
{
    for (const KiriView::ImageDocumentEffect &effect : effects) {
        if (const auto *payload = std::get_if<Effect>(&effect.payload)) {
            return payload;
        }
    }

    return nullptr;
}

template <typename Effect> bool hasEffect(const KiriView::ImageDocumentEffects &effects)
{
    return findEffect<Effect>(effects) != nullptr;
}
}

class TestImageOpenWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void firstImageLoadSuccessTransitionsToReady();
    void directArchiveImageLoadSuccessDisablesContainerNavigation();
    void replacementLoadFailureKeepsDisplayedImage();
    void emptyContainerFailureSelectsFailedContainer();
    void animationFailureClearsImageAndResetsZoom();
};

void TestImageOpenWorkflow::firstImageLoadSuccessTransitionsToReady()
{
    KiriView::ImageDocumentState state;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setSourceUrl(imageUrl);

    const KiriView::ImageDocumentEffects beginEffects
        = KiriView::ImageOpenWorkflow::beginSourceLoad(state, false);
    QVERIFY(hasEffect<KiriView::ClearImageEffect>(beginEffects));
    QVERIFY(hasEffect<KiriView::ResetZoomEffect>(beginEffects));
    QVERIFY(state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Loading);

    const KiriView::ImageDocumentEffects successEffects
        = KiriView::ImageOpenWorkflow::finishSuccessfulImageLoad(
            state, loadSession(imageUrl, imageUrl));
    QVERIFY(hasEffect<KiriView::UpdatePageNavigationEffect>(successEffects));
    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedUrl(), imageUrl);
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(state.errorString().isEmpty());
}

void TestImageOpenWorkflow::directArchiveImageLoadSuccessDisablesContainerNavigation()
{
    KiriView::ImageDocumentState state;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    QUrl imageUrl = archiveDocument->rootUrl();
    imageUrl.setPath(archiveDocument->rootUrl().path() + QStringLiteral("01.png"));

    state.setSourceUrl(archiveUrl);

    const KiriView::ImageDocumentEffects successEffects
        = KiriView::ImageOpenWorkflow::finishSuccessfulImageLoad(
            state, loadSession(archiveUrl, imageUrl, *archiveDocument));

    QVERIFY(hasEffect<KiriView::UpdatePageNavigationEffect>(successEffects));
    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedUrl(), imageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.zip"));
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QVERIFY(!state.containerNavigationAvailable());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
}

void TestImageOpenWorkflow::replacementLoadFailureKeepsDisplayedImage()
{
    KiriView::ImageDocumentState state;
    const QUrl displayedUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/missing.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    state.setSourceUrl(replacementUrl);
    state.setLoading(true);
    state.setStatus(KiriView::ImageDocumentStatus::Ready);

    const KiriView::ImageDocumentEffects effects
        = KiriView::ImageOpenWorkflow::finishReplacementLoadWithError(
            state, QStringLiteral("missing"));
    QVERIFY(!hasEffect<KiriView::ClearImageEffect>(effects));
    QVERIFY(hasEffect<KiriView::UpdatePageNavigationEffect>(effects));
    QVERIFY(hasEffect<KiriView::ScheduleAdjacentImagePredecodeEffect>(effects));
    QCOMPARE(state.sourceUrl(), displayedUrl);
    QCOMPARE(state.displayedUrl(), displayedUrl);
    QCOMPARE(state.errorString(), QStringLiteral("missing"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
}

void TestImageOpenWorkflow::emptyContainerFailureSelectsFailedContainer()
{
    KiriView::ImageDocumentState state;
    const QUrl containerUrl = localUrl(QStringLiteral("/images/empty/"));
    state.setLoading(true);
    state.setLoadingContainerNavigationUrl(containerUrl);

    const KiriView::ImageDocumentEffects effects
        = KiriView::ImageOpenWorkflow::finishContainerNavigationLoadWithError(
            state, containerUrl, QStringLiteral("empty"));
    QVERIFY(hasEffect<KiriView::ClearImageEffect>(effects));
    const auto *prepareFailedContainer
        = findEffect<KiriView::PrepareFailedContainerEffect>(effects);
    QVERIFY(prepareFailedContainer != nullptr);
    QCOMPARE(prepareFailedContainer->containerUrl, containerUrl);
    QCOMPARE(state.sourceUrl(), containerUrl);
    QCOMPARE(state.containerNavigationUrl(), containerUrl);
    QCOMPARE(state.errorString(), QStringLiteral("empty"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
}

void TestImageOpenWorkflow::animationFailureClearsImageAndResetsZoom()
{
    KiriView::ImageDocumentState state;
    const QUrl displayedUrl = localUrl(QStringLiteral("/images/animated.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    state.setContainerNavigationUrl(localUrl(QStringLiteral("/images/")));
    state.setLoading(true);
    state.setStatus(KiriView::ImageDocumentStatus::Ready);

    const KiriView::ImageDocumentEffects effects
        = KiriView::ImageOpenWorkflow::finishAnimationLoadWithError(
            state, QStringLiteral("animation failed"));

    QVERIFY(hasEffect<KiriView::ClearImageEffect>(effects));
    QVERIFY(hasEffect<KiriView::ResetZoomEffect>(effects));
    QVERIFY(state.containerNavigationUrl().isEmpty());
    QCOMPARE(state.errorString(), QStringLiteral("animation failed"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
}

QTEST_GUILESS_MAIN(TestImageOpenWorkflow)

#include "test_imageopenworkflow.moc"

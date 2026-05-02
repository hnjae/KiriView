// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::ImageLoadSession loadSession(const QUrl &sourceUrl, const QUrl &imageUrl,
    const QUrl &comicBookRootUrl = QUrl(), const QUrl &containerNavigationUrl = QUrl())
{
    return KiriView::ImageLoadSession {
        1,
        KiriView::ImageLoadRequest { sourceUrl, comicBookRootUrl, containerNavigationUrl },
        KiriView::DisplayedImageLocation { imageUrl, comicBookRootUrl },
    };
}
}

class TestImageOpenWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void firstImageLoadSuccessTransitionsToReady();
    void replacementLoadFailureKeepsDisplayedImage();
    void emptyContainerFailureSelectsFailedContainer();
};

void TestImageOpenWorkflow::firstImageLoadSuccessTransitionsToReady()
{
    KiriView::ImageDocumentState state;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setSourceUrl(imageUrl);

    const KiriView::ImageOpenCommands beginCommands
        = KiriView::ImageOpenWorkflow::beginSourceLoad(state, false);
    QVERIFY(beginCommands.clearImage);
    QVERIFY(beginCommands.resetZoom);
    QVERIFY(state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Loading);

    const KiriView::ImageOpenCommands successCommands
        = KiriView::ImageOpenWorkflow::finishSuccessfulImageLoad(
            state, loadSession(imageUrl, imageUrl));
    QVERIFY(successCommands.updatePageNavigation);
    QCOMPARE(state.sourceUrl(), imageUrl);
    QCOMPARE(state.displayedUrl(), imageUrl);
    QCOMPARE(state.containerNavigationUrl(), localUrl(QStringLiteral("/images/")));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(state.errorString().isEmpty());
}

void TestImageOpenWorkflow::replacementLoadFailureKeepsDisplayedImage()
{
    KiriView::ImageDocumentState state;
    const QUrl displayedUrl = localUrl(QStringLiteral("/images/current.png"));
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/missing.png"));
    state.setDisplayedImageLocation({ displayedUrl, QUrl() });
    state.setSourceUrl(replacementUrl);
    state.setLoading(true);
    state.setStatus(KiriView::ImageDocumentStatus::Ready);

    const KiriView::ImageOpenCommands commands
        = KiriView::ImageOpenWorkflow::finishReplacementLoadWithError(
            state, QStringLiteral("missing"));
    QVERIFY(!commands.clearImage);
    QVERIFY(commands.updatePageNavigation);
    QVERIFY(commands.scheduleAdjacentPredecode);
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

    const KiriView::ImageOpenCommands commands
        = KiriView::ImageOpenWorkflow::finishContainerNavigationLoadWithError(
            state, containerUrl, QStringLiteral("empty"));
    QVERIFY(commands.clearImage);
    QCOMPARE(commands.failedContainerUrl, containerUrl);
    QCOMPARE(state.sourceUrl(), containerUrl);
    QCOMPARE(state.containerNavigationUrl(), containerUrl);
    QCOMPARE(state.errorString(), QStringLiteral("empty"));
    QVERIFY(!state.loading());
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Error);
}

QTEST_GUILESS_MAIN(TestImageOpenWorkflow)

#include "test_imageopenworkflow.moc"

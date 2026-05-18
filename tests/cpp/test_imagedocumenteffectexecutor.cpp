// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectexecutor.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <QUrl>

namespace {
using KiriView::ImageDocumentEffect;
using KiriView::ImageDocumentEffectOperations;
using KiriView::ImageDocumentEffects;

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

struct RecordedEffectOperations {
    ImageDocumentEffectOperations operations;
    QStringList events;
    QUrl url;
    QUrl secondaryUrl;
    QString errorString;
    bool flag = false;

    RecordedEffectOperations()
    {
        operations.cancelFileDeletion = [this]() { record(QStringLiteral("cancelFileDeletion")); };
        operations.stopPresentationAnimation
            = [this]() { record(QStringLiteral("stopPresentationAnimation")); };
        operations.shutdownSpread = [this]() { record(QStringLiteral("shutdownSpread")); };
        operations.clearArchiveSession
            = [this]() { record(QStringLiteral("clearArchiveSession")); };
        operations.clearPredecode = [this]() { record(QStringLiteral("clearPredecode")); };
        operations.cancelPredecode = [this]() { record(QStringLiteral("cancelPredecode")); };
        operations.finishSpreadTransition
            = [this]() { record(QStringLiteral("finishSpreadTransition")); };
        operations.clearSecondaryPage = [this]() { record(QStringLiteral("clearSecondaryPage")); };
        operations.cancelPageNavigationUpdate
            = [this]() { record(QStringLiteral("cancelPageNavigationUpdate")); };
        operations.cancelNavigation = [this]() { record(QStringLiteral("cancelNavigation")); };
        operations.cancelContainerNavigation
            = [this]() { record(QStringLiteral("cancelContainerNavigation")); };
        operations.cancelOpen = [this]() { record(QStringLiteral("cancelOpen")); };
        operations.clearDisplayedImageLocation
            = [this]() { record(QStringLiteral("clearDisplayedImageLocation")); };
        operations.clearPresentationImage
            = [this]() { record(QStringLiteral("clearPresentationImage")); };
        operations.clearPageNavigation
            = [this]() { record(QStringLiteral("clearPageNavigation")); };
        operations.notifyRightToLeftReadingChanged
            = [this]() { record(QStringLiteral("notifyRightToLeftReadingChanged")); };
        operations.resetZoom = [this]() { record(QStringLiteral("resetZoom")); };
        operations.updatePageNavigation
            = [this]() { record(QStringLiteral("updatePageNavigation")); };
        operations.scheduleAdjacentImagePredecode
            = [this]() { record(QStringLiteral("scheduleAdjacentImagePredecode")); };
        operations.loadUrl = [this](const QUrl &targetUrl) {
            url = targetUrl;
            record(QStringLiteral("loadUrl"));
        };
        operations.loadContainerImage = [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            url = imageUrl;
            secondaryUrl = containerUrl;
            record(QStringLiteral("loadContainerImage"));
        };
        operations.finishEmptyContainerNavigation = [this](const QUrl &containerUrl) {
            url = containerUrl;
            record(QStringLiteral("finishEmptyContainerNavigation"));
        };
        operations.finishContainerNavigationLoadWithError
            = [this](const QUrl &containerUrl, const QString &message) {
                  url = containerUrl;
                  errorString = message;
                  record(QStringLiteral("finishContainerNavigationLoadWithError"));
              };
        operations.loadPageNavigationUrl = [this](const QUrl &targetUrl, bool preserveTransition) {
            url = targetUrl;
            flag = preserveTransition;
            record(QStringLiteral("loadPageNavigationUrl"));
        };
        operations.prepareFailedContainer = [this](const QUrl &containerUrl) {
            url = containerUrl;
            record(QStringLiteral("prepareFailedContainer"));
        };
        operations.setSourceUrl = [this](const QUrl &sourceUrl) {
            url = sourceUrl;
            record(QStringLiteral("setSourceUrl"));
        };
        operations.setErrorString = [this](const QString &message) {
            errorString = message;
            record(QStringLiteral("setErrorString"));
        };
        operations.finishEmptySourceLoad = [this]() {
            record(QStringLiteral("finishEmptySourceLoad"));
            return ImageDocumentEffects {
                ImageDocumentEffect::clearImage(),
                ImageDocumentEffect::resetZoom(),
            };
        };
    }

    void clear()
    {
        events.clear();
        url = QUrl();
        secondaryUrl = QUrl();
        errorString.clear();
        flag = false;
    }

    void record(const QString &event) { events.append(event); }
};
}

class TestImageDocumentEffectExecutor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clearImageDispatchesOrderedRuntimeOperations();
    void clearDeletedImageDispatchesDeletionClearThenGeneratedEffects();
    void shutdownRuntimeDispatchesOrderedLifecycleOperations();
    void payloadEffectsDispatchToRuntimeOperations();
};

void TestImageDocumentEffectExecutor::clearImageDispatchesOrderedRuntimeOperations()
{
    RecordedEffectOperations recorded;
    KiriView::ImageDocumentEffectExecutor executor(recorded.operations);

    executor.dispatch(KiriView::ImageDocumentEffect::clearImage());

    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("clearArchiveSession"),
            QStringLiteral("clearPredecode"),
            QStringLiteral("finishSpreadTransition"),
            QStringLiteral("clearSecondaryPage"),
            QStringLiteral("cancelPageNavigationUpdate"),
            QStringLiteral("clearDisplayedImageLocation"),
            QStringLiteral("clearPresentationImage"),
            QStringLiteral("clearPageNavigation"),
            QStringLiteral("notifyRightToLeftReadingChanged"),
        }));
}

void TestImageDocumentEffectExecutor::clearDeletedImageDispatchesDeletionClearThenGeneratedEffects()
{
    RecordedEffectOperations recorded;
    KiriView::ImageDocumentEffectExecutor executor(recorded.operations);

    executor.dispatch(KiriView::ImageDocumentEffect::clearDeletedImage());

    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("clearArchiveSession"),
            QStringLiteral("cancelNavigation"),
            QStringLiteral("cancelContainerNavigation"),
            QStringLiteral("cancelPredecode"),
            QStringLiteral("cancelOpen"),
            QStringLiteral("finishSpreadTransition"),
            QStringLiteral("clearSecondaryPage"),
            QStringLiteral("setSourceUrl"),
            QStringLiteral("setErrorString"),
            QStringLiteral("finishEmptySourceLoad"),
            QStringLiteral("clearArchiveSession"),
            QStringLiteral("clearPredecode"),
            QStringLiteral("finishSpreadTransition"),
            QStringLiteral("clearSecondaryPage"),
            QStringLiteral("cancelPageNavigationUpdate"),
            QStringLiteral("clearDisplayedImageLocation"),
            QStringLiteral("clearPresentationImage"),
            QStringLiteral("clearPageNavigation"),
            QStringLiteral("notifyRightToLeftReadingChanged"),
            QStringLiteral("resetZoom"),
        }));
    QVERIFY(recorded.url.isEmpty());
    QVERIFY(recorded.errorString.isEmpty());
}

void TestImageDocumentEffectExecutor::shutdownRuntimeDispatchesOrderedLifecycleOperations()
{
    RecordedEffectOperations recorded;
    KiriView::ImageDocumentEffectExecutor executor(recorded.operations);

    executor.shutdownRuntime();

    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("cancelFileDeletion"),
            QStringLiteral("stopPresentationAnimation"),
            QStringLiteral("shutdownSpread"),
            QStringLiteral("cancelPredecode"),
            QStringLiteral("cancelPageNavigationUpdate"),
            QStringLiteral("cancelContainerNavigation"),
            QStringLiteral("cancelNavigation"),
            QStringLiteral("cancelOpen"),
            QStringLiteral("clearArchiveSession"),
        }));
}

void TestImageDocumentEffectExecutor::payloadEffectsDispatchToRuntimeOperations()
{
    RecordedEffectOperations recorded;
    KiriView::ImageDocumentEffectExecutor executor(recorded.operations);

    executor.dispatch(
        KiriView::ImageDocumentEffect::openUrl(localUrl(QStringLiteral("/image.png"))));
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("loadUrl") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/image.png")));

    recorded.clear();
    executor.dispatch(KiriView::ImageDocumentEffect::containerImageSelected(
        localUrl(QStringLiteral("/book/01.png")), localUrl(QStringLiteral("/book.cbz"))));
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("loadContainerImage") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/book/01.png")));
    QCOMPARE(recorded.secondaryUrl, localUrl(QStringLiteral("/book.cbz")));

    recorded.clear();
    executor.dispatch(KiriView::ImageDocumentEffect::emptyContainerSelected(
        localUrl(QStringLiteral("/empty.cbz"))));
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("finishEmptyContainerNavigation") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/empty.cbz")));

    recorded.clear();
    executor.dispatch(KiriView::ImageDocumentEffect::containerNavigationFailed(
        localUrl(QStringLiteral("/broken.cbz")), QStringLiteral("broken")));
    QCOMPARE(
        recorded.events, QStringList({ QStringLiteral("finishContainerNavigationLoadWithError") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/broken.cbz")));
    QCOMPARE(recorded.errorString, QStringLiteral("broken"));

    recorded.clear();
    executor.dispatch(KiriView::ImageDocumentEffect::pageNavigationSelected(
        localUrl(QStringLiteral("/next.png")), true));
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("loadPageNavigationUrl") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/next.png")));
    QVERIFY(recorded.flag);

    recorded.clear();
    executor.dispatch(KiriView::ImageDocumentEffect::prepareFailedContainer(
        localUrl(QStringLiteral("/bad.zip"))));
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("prepareFailedContainer") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/bad.zip")));

    recorded.clear();
    executor.dispatch(KiriView::ImageDocumentEffect::updatePageNavigation());
    executor.dispatch(KiriView::ImageDocumentEffect::scheduleAdjacentImagePredecode());
    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("updatePageNavigation"),
            QStringLiteral("scheduleAdjacentImagePredecode"),
        }));
}

QTEST_GUILESS_MAIN(TestImageDocumentEffectExecutor)

#include "test_imagedocumenteffectexecutor.moc"

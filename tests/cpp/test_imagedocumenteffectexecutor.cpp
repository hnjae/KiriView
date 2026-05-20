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
        operations.lifecycle.cancelFileDeletion
            = [this]() { record(QStringLiteral("cancelFileDeletion")); };
        operations.lifecycle.stopPresentationAnimation
            = [this]() { record(QStringLiteral("stopPresentationAnimation")); };
        operations.lifecycle.shutdownSpread
            = [this]() { record(QStringLiteral("shutdownSpread")); };
        operations.archive.clearSession
            = [this]() { record(QStringLiteral("clearArchiveSession")); };
        operations.predecode.clearPredecode
            = [this]() { record(QStringLiteral("clearPredecode")); };
        operations.predecode.cancelPredecode
            = [this]() { record(QStringLiteral("cancelPredecode")); };
        operations.predecode.scheduleAdjacentImagePredecode
            = [this]() { record(QStringLiteral("scheduleAdjacentImagePredecode")); };
        operations.spread.finishSpreadTransition
            = [this]() { record(QStringLiteral("finishSpreadTransition")); };
        operations.spread.resetRightToLeftReading
            = [this]() { record(QStringLiteral("resetRightToLeftReading")); };
        operations.spread.clearSecondaryPage
            = [this]() { record(QStringLiteral("clearSecondaryPage")); };
        operations.spread.notifyRightToLeftReadingChanged
            = [this]() { record(QStringLiteral("notifyRightToLeftReadingChanged")); };
        operations.spread.resetZoom = [this]() { record(QStringLiteral("resetZoom")); };
        operations.spread.prepareFailedContainer = [this](const QUrl &containerUrl) {
            url = containerUrl;
            record(QStringLiteral("prepareFailedContainer"));
        };
        operations.navigation.cancelPageNavigationUpdate
            = [this]() { record(QStringLiteral("cancelPageNavigationUpdate")); };
        operations.navigation.cancelNavigation
            = [this]() { record(QStringLiteral("cancelNavigation")); };
        operations.navigation.cancelContainerNavigation
            = [this]() { record(QStringLiteral("cancelContainerNavigation")); };
        operations.navigation.cancelAllNavigation
            = [this]() { record(QStringLiteral("cancelAllNavigation")); };
        operations.navigation.clearPageNavigation
            = [this]() { record(QStringLiteral("clearPageNavigation")); };
        operations.navigation.updatePageNavigation
            = [this]() { record(QStringLiteral("updatePageNavigation")); };
        operations.navigation.loadUrl = [this](const QUrl &targetUrl) {
            url = targetUrl;
            record(QStringLiteral("loadUrl"));
        };
        operations.navigation.loadContainerImage
            = [this](const QUrl &imageUrl, const QUrl &containerUrl) {
                  url = imageUrl;
                  secondaryUrl = containerUrl;
                  record(QStringLiteral("loadContainerImage"));
              };
        operations.navigation.finishEmptyContainerNavigation = [this](const QUrl &containerUrl) {
            url = containerUrl;
            record(QStringLiteral("finishEmptyContainerNavigation"));
        };
        operations.navigation.finishContainerNavigationLoadWithError
            = [this](const QUrl &containerUrl, const QString &message) {
                  url = containerUrl;
                  errorString = message;
                  record(QStringLiteral("finishContainerNavigationLoadWithError"));
              };
        operations.navigation.loadPageNavigationUrl
            = [this](const QUrl &targetUrl, bool preserveTransition) {
                  url = targetUrl;
                  flag = preserveTransition;
                  record(QStringLiteral("loadPageNavigationUrl"));
              };
        operations.open.cancelOpen = [this]() { record(QStringLiteral("cancelOpen")); };
        operations.open.clearDisplayedImageLocation
            = [this]() { record(QStringLiteral("clearDisplayedImageLocation")); };
        operations.open.clearPresentationImage
            = [this]() { record(QStringLiteral("clearPresentationImage")); };
        operations.sourceLoad.clearLoadingContainerNavigationUrl
            = [this]() { record(QStringLiteral("clearLoadingContainerNavigationUrl")); };
        operations.sourceLoad.setLoadingContainerNavigationUrl = [this](const QUrl &targetUrl) {
            url = targetUrl;
            record(QStringLiteral("setLoadingContainerNavigationUrl"));
        };
        operations.sourceLoad.setContainerNavigationUrl = [this](const QUrl &targetUrl) {
            url = targetUrl;
            record(QStringLiteral("setContainerNavigationUrl"));
        };
        operations.sourceLoad.prepareSourceLoad
            = [this](const KiriView::ImageDocumentSourceLoadRequest &request) {
                  url = request.sourceUrl;
                  secondaryUrl = request.containerNavigationUrl;
                  flag = request.preserveTwoPageSpreadTransition;
                  record(QStringLiteral("prepareSourceLoad"));
              };
        operations.open.setSourceUrl = [this](const QUrl &sourceUrl) {
            url = sourceUrl;
            record(QStringLiteral("setSourceUrl"));
        };
        operations.sourceLoad.beginOpen = [this]() { record(QStringLiteral("beginOpen")); };
        operations.open.setErrorString = [this](const QString &message) {
            errorString = message;
            record(QStringLiteral("setErrorString"));
        };
        operations.open.finishEmptySourceLoad = [this]() {
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
    void runtimePlansDispatchSourceLoadOperations();
    void runtimePlansDispatchEveryOperationExplicitly();
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
            QStringLiteral("cancelAllNavigation"),
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
            QStringLiteral("cancelAllNavigation"),
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

void TestImageDocumentEffectExecutor::runtimePlansDispatchSourceLoadOperations()
{
    RecordedEffectOperations recorded;
    KiriView::ImageDocumentEffectExecutor executor(recorded.operations);
    const QUrl sourceUrl = localUrl(QStringLiteral("/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/book.cbz"));

    executor.dispatchPlan({
        KiriView::CancelFileDeletionOperation {},
        KiriView::CancelAllNavigationOperation {},
        KiriView::CancelPredecodeOperation {},
        KiriView::FinishSpreadTransitionOperation {},
        KiriView::ResetRightToLeftReadingOperation {},
        KiriView::ClearSecondaryPageOperation {},
        KiriView::ClearLoadingContainerNavigationUrlOperation {},
        KiriView::SetLoadingContainerNavigationUrlOperation { containerUrl },
        KiriView::SetContainerNavigationUrlOperation { containerUrl },
        KiriView::PrepareSourceLoadOperation {
            KiriView::ImageDocumentSourceLoadRequest::fromPageNavigation(sourceUrl, true),
        },
        KiriView::SetSourceUrlOperation { sourceUrl },
        KiriView::BeginOpenOperation {},
        KiriView::NotifyRightToLeftReadingChangedOperation {},
    });

    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("cancelFileDeletion"),
            QStringLiteral("cancelAllNavigation"),
            QStringLiteral("cancelPredecode"),
            QStringLiteral("finishSpreadTransition"),
            QStringLiteral("resetRightToLeftReading"),
            QStringLiteral("clearSecondaryPage"),
            QStringLiteral("clearLoadingContainerNavigationUrl"),
            QStringLiteral("setLoadingContainerNavigationUrl"),
            QStringLiteral("setContainerNavigationUrl"),
            QStringLiteral("prepareSourceLoad"),
            QStringLiteral("setSourceUrl"),
            QStringLiteral("beginOpen"),
            QStringLiteral("notifyRightToLeftReadingChanged"),
        }));
    QCOMPARE(recorded.url, sourceUrl);
    QVERIFY(recorded.secondaryUrl.isEmpty());
    QVERIFY(recorded.flag);
}

void TestImageDocumentEffectExecutor::runtimePlansDispatchEveryOperationExplicitly()
{
    RecordedEffectOperations recorded;
    KiriView::ImageDocumentEffectExecutor executor(recorded.operations);
    const QUrl sourceUrl = localUrl(QStringLiteral("/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/book.cbz"));

    executor.dispatchPlan({
        KiriView::CancelFileDeletionOperation {},
        KiriView::StopPresentationAnimationOperation {},
        KiriView::ShutdownSpreadOperation {},
        KiriView::ClearArchiveSessionOperation {},
        KiriView::ClearPredecodeOperation {},
        KiriView::CancelPredecodeOperation {},
        KiriView::ScheduleAdjacentImagePredecodeOperation {},
        KiriView::FinishSpreadTransitionOperation {},
        KiriView::ResetRightToLeftReadingOperation {},
        KiriView::ClearSecondaryPageOperation {},
        KiriView::NotifyRightToLeftReadingChangedOperation {},
        KiriView::ResetZoomOperation {},
        KiriView::PrepareFailedContainerOperation { containerUrl },
        KiriView::CancelPageNavigationUpdateOperation {},
        KiriView::CancelNavigationOperation {},
        KiriView::CancelContainerNavigationOperation {},
        KiriView::CancelAllNavigationOperation {},
        KiriView::ClearPageNavigationOperation {},
        KiriView::UpdatePageNavigationOperation {},
        KiriView::LoadUrlOperation { sourceUrl },
        KiriView::LoadContainerImageOperation { sourceUrl, containerUrl },
        KiriView::FinishEmptyContainerNavigationOperation { containerUrl },
        KiriView::FinishContainerNavigationLoadWithErrorOperation {
            containerUrl,
            QStringLiteral("broken"),
        },
        KiriView::LoadPageNavigationUrlOperation { sourceUrl, true },
        KiriView::CancelOpenOperation {},
        KiriView::ClearDisplayedImageLocationOperation {},
        KiriView::ClearPresentationImageOperation {},
        KiriView::ClearLoadingContainerNavigationUrlOperation {},
        KiriView::SetLoadingContainerNavigationUrlOperation { containerUrl },
        KiriView::SetContainerNavigationUrlOperation { containerUrl },
        KiriView::PrepareSourceLoadOperation {
            KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl),
        },
        KiriView::SetSourceUrlOperation { sourceUrl },
        KiriView::BeginOpenOperation {},
        KiriView::SetErrorStringOperation { QStringLiteral("failed") },
        KiriView::FinishEmptySourceLoadOperation {},
    });

    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("cancelFileDeletion"),
            QStringLiteral("stopPresentationAnimation"),
            QStringLiteral("shutdownSpread"),
            QStringLiteral("clearArchiveSession"),
            QStringLiteral("clearPredecode"),
            QStringLiteral("cancelPredecode"),
            QStringLiteral("scheduleAdjacentImagePredecode"),
            QStringLiteral("finishSpreadTransition"),
            QStringLiteral("resetRightToLeftReading"),
            QStringLiteral("clearSecondaryPage"),
            QStringLiteral("notifyRightToLeftReadingChanged"),
            QStringLiteral("resetZoom"),
            QStringLiteral("prepareFailedContainer"),
            QStringLiteral("cancelPageNavigationUpdate"),
            QStringLiteral("cancelNavigation"),
            QStringLiteral("cancelContainerNavigation"),
            QStringLiteral("cancelAllNavigation"),
            QStringLiteral("clearPageNavigation"),
            QStringLiteral("updatePageNavigation"),
            QStringLiteral("loadUrl"),
            QStringLiteral("loadContainerImage"),
            QStringLiteral("finishEmptyContainerNavigation"),
            QStringLiteral("finishContainerNavigationLoadWithError"),
            QStringLiteral("loadPageNavigationUrl"),
            QStringLiteral("cancelOpen"),
            QStringLiteral("clearDisplayedImageLocation"),
            QStringLiteral("clearPresentationImage"),
            QStringLiteral("clearLoadingContainerNavigationUrl"),
            QStringLiteral("setLoadingContainerNavigationUrl"),
            QStringLiteral("setContainerNavigationUrl"),
            QStringLiteral("prepareSourceLoad"),
            QStringLiteral("setSourceUrl"),
            QStringLiteral("beginOpen"),
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
}

QTEST_GUILESS_MAIN(TestImageDocumentEffectExecutor)

#include "test_imagedocumenteffectexecutor.moc"

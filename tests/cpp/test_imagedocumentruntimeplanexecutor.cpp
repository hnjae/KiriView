// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntimeplanexecutor.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <QUrl>

namespace {
using KiriView::ImageDocumentRuntimeOperations;
using KiriView::ImageDocumentRuntimePlan;

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

struct RecordedRuntimeOperations {
    ImageDocumentRuntimeOperations operations;
    QStringList events;
    QUrl url;
    QUrl secondaryUrl;
    KiriView::ImageDocumentPageKind kind = KiriView::ImageDocumentPageKind::Image;
    KiriView::NavigationDirection direction = KiriView::NavigationDirection::Next;
    QString errorString;
    bool flag = false;

    RecordedRuntimeOperations()
    {
        operations.lifecycle.cancelFileDeletion
            = [this]() { record(QStringLiteral("cancelFileDeletion")); };
        operations.lifecycle.stopPresentationAnimation
            = [this]() { record(QStringLiteral("stopPresentationAnimation")); };
        operations.lifecycle.shutdownSpread
            = [this]() { record(QStringLiteral("shutdownSpread")); };
        operations.mediaEntrySource.clear
            = [this]() { record(QStringLiteral("clearMediaEntrySource")); };
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
        operations.navigation.loadUrl = [this](const KiriView::ImageDocumentPageTarget &target) {
            url = target.url;
            kind = target.kind;
            record(QStringLiteral("loadUrl"));
        };
        operations.navigation.loadContainerImage
            = [this](const KiriView::ImageDocumentPageTarget &target, const QUrl &containerUrl) {
                  url = target.url;
                  kind = target.kind;
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
        operations.navigation.reportContainerNavigationBoundary
            = [this](KiriView::NavigationDirection boundaryDirection) {
                  direction = boundaryDirection;
                  record(QStringLiteral("reportContainerNavigationBoundary"));
              };
        operations.navigation.loadPageNavigationUrl
            = [this](const KiriView::ImageDocumentPageTarget &target, bool preserveTransition) {
                  url = target.url;
                  kind = target.kind;
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
        operations.open.setSourceUrl = [this](const KiriView::ImageDocumentPageTarget &target) {
            url = target.url;
            kind = target.kind;
            record(QStringLiteral("setSourceUrl"));
        };
        operations.sourceLoad.beginOpen = [this]() { record(QStringLiteral("beginOpen")); };
        operations.open.setErrorString = [this](const QString &message) {
            errorString = message;
            record(QStringLiteral("setErrorString"));
        };
        operations.open.finishEmptySourceLoad
            = [this]() { record(QStringLiteral("finishEmptySourceLoad")); };
    }

    void clear()
    {
        events.clear();
        url = QUrl();
        secondaryUrl = QUrl();
        kind = KiriView::ImageDocumentPageKind::Image;
        direction = KiriView::NavigationDirection::Next;
        errorString.clear();
        flag = false;
    }

    void record(const QString &event) { events.append(event); }
};
}

class TestImageDocumentRuntimePlanExecutor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clearImageDispatchesOrderedRuntimeOperations();
    void clearDeletedImageDispatchesDeletionClearThenOpenCompletion();
    void shutdownRuntimeDispatchesOrderedLifecycleOperations();
    void payloadRuntimePlansDispatchToOperations();
    void runtimePlansDispatchSourceLoadOperations();
    void runtimePlansDispatchEveryOperationExplicitly();
};

void TestImageDocumentRuntimePlanExecutor::clearImageDispatchesOrderedRuntimeOperations()
{
    RecordedRuntimeOperations recorded;
    KiriView::ImageDocumentRuntimePlanExecutor executor(recorded.operations);

    executor.dispatchPlan(KiriView::imageDocumentClearImagePlan());

    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("clearMediaEntrySource"),
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

void TestImageDocumentRuntimePlanExecutor::
    clearDeletedImageDispatchesDeletionClearThenOpenCompletion()
{
    RecordedRuntimeOperations recorded;
    KiriView::ImageDocumentRuntimePlanExecutor executor(recorded.operations);

    executor.dispatchPlan(KiriView::imageDocumentClearDeletedImagePlan());

    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("clearMediaEntrySource"),
            QStringLiteral("cancelAllNavigation"),
            QStringLiteral("cancelPredecode"),
            QStringLiteral("cancelOpen"),
            QStringLiteral("finishSpreadTransition"),
            QStringLiteral("clearSecondaryPage"),
            QStringLiteral("setSourceUrl"),
            QStringLiteral("setErrorString"),
            QStringLiteral("finishEmptySourceLoad"),
        }));
    QVERIFY(recorded.url.isEmpty());
    QVERIFY(recorded.errorString.isEmpty());
}

void TestImageDocumentRuntimePlanExecutor::shutdownRuntimeDispatchesOrderedLifecycleOperations()
{
    RecordedRuntimeOperations recorded;
    KiriView::ImageDocumentRuntimePlanExecutor executor(recorded.operations);

    executor.shutdownRuntime();

    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("cancelFileDeletion"),
            QStringLiteral("stopPresentationAnimation"),
            QStringLiteral("shutdownSpread"),
            QStringLiteral("cancelPredecode"),
            QStringLiteral("cancelAllNavigation"),
            QStringLiteral("cancelOpen"),
            QStringLiteral("clearMediaEntrySource"),
        }));
}

void TestImageDocumentRuntimePlanExecutor::payloadRuntimePlansDispatchToOperations()
{
    RecordedRuntimeOperations recorded;
    KiriView::ImageDocumentRuntimePlanExecutor executor(recorded.operations);

    executor.dispatchPlan(ImageDocumentRuntimePlan { KiriView::LoadUrlOperation {
        KiriView::ImageDocumentPageTarget {
            localUrl(QStringLiteral("/image.png")),
            KiriView::ImageDocumentPageKind::Video,
        },
    } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("loadUrl") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/image.png")));
    QCOMPARE(recorded.kind, KiriView::ImageDocumentPageKind::Video);

    recorded.clear();
    executor.dispatchPlan(ImageDocumentRuntimePlan { KiriView::LoadContainerImageOperation {
        KiriView::ImageDocumentPageTarget {
            localUrl(QStringLiteral("/book/01.png")),
            KiriView::ImageDocumentPageKind::Video,
        },
        localUrl(QStringLiteral("/book.cbz")),
    } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("loadContainerImage") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/book/01.png")));
    QCOMPARE(recorded.kind, KiriView::ImageDocumentPageKind::Video);
    QCOMPARE(recorded.secondaryUrl, localUrl(QStringLiteral("/book.cbz")));

    recorded.clear();
    executor.dispatchPlan(
        ImageDocumentRuntimePlan { KiriView::FinishEmptyContainerNavigationOperation {
            localUrl(QStringLiteral("/empty.cbz")),
        } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("finishEmptyContainerNavigation") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/empty.cbz")));

    recorded.clear();
    executor.dispatchPlan(
        ImageDocumentRuntimePlan { KiriView::FinishContainerNavigationLoadWithErrorOperation {
            localUrl(QStringLiteral("/broken.cbz")),
            QStringLiteral("broken"),
        } });
    QCOMPARE(
        recorded.events, QStringList({ QStringLiteral("finishContainerNavigationLoadWithError") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/broken.cbz")));
    QCOMPARE(recorded.errorString, QStringLiteral("broken"));

    recorded.clear();
    executor.dispatchPlan(
        ImageDocumentRuntimePlan { KiriView::ReportContainerNavigationBoundaryOperation {
            KiriView::NavigationDirection::Previous,
        } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("reportContainerNavigationBoundary") }));
    QCOMPARE(recorded.direction, KiriView::NavigationDirection::Previous);

    recorded.clear();
    executor.dispatchPlan(ImageDocumentRuntimePlan { KiriView::LoadPageNavigationUrlOperation {
        KiriView::ImageDocumentPageTarget {
            localUrl(QStringLiteral("/next.png")),
            KiriView::ImageDocumentPageKind::Video,
        },
        true,
    } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("loadPageNavigationUrl") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/next.png")));
    QCOMPARE(recorded.kind, KiriView::ImageDocumentPageKind::Video);
    QVERIFY(recorded.flag);

    recorded.clear();
    executor.dispatchPlan(ImageDocumentRuntimePlan { KiriView::PrepareFailedContainerOperation {
        localUrl(QStringLiteral("/bad.zip")),
    } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("prepareFailedContainer") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/bad.zip")));

    recorded.clear();
    executor.dispatchPlan(ImageDocumentRuntimePlan {
        KiriView::UpdatePageNavigationOperation {},
        KiriView::ScheduleAdjacentImagePredecodeOperation {},
    });
    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("updatePageNavigation"),
            QStringLiteral("scheduleAdjacentImagePredecode"),
        }));
}

void TestImageDocumentRuntimePlanExecutor::runtimePlansDispatchSourceLoadOperations()
{
    RecordedRuntimeOperations recorded;
    KiriView::ImageDocumentRuntimePlanExecutor executor(recorded.operations);
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
        KiriView::SetSourceUrlOperation {
            KiriView::ImageDocumentPageTarget {
                sourceUrl,
                KiriView::ImageDocumentPageKind::Video,
            },
        },
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
    QCOMPARE(recorded.kind, KiriView::ImageDocumentPageKind::Video);
    QVERIFY(recorded.secondaryUrl.isEmpty());
    QVERIFY(recorded.flag);
}

void TestImageDocumentRuntimePlanExecutor::runtimePlansDispatchEveryOperationExplicitly()
{
    RecordedRuntimeOperations recorded;
    KiriView::ImageDocumentRuntimePlanExecutor executor(recorded.operations);
    const QUrl sourceUrl = localUrl(QStringLiteral("/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/book.cbz"));

    executor.dispatchPlan({
        KiriView::CancelFileDeletionOperation {},
        KiriView::StopPresentationAnimationOperation {},
        KiriView::ShutdownSpreadOperation {},
        KiriView::ClearMediaEntrySourceOperation {},
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
        KiriView::LoadUrlOperation { KiriView::ImageDocumentPageTarget {
            sourceUrl,
            KiriView::ImageDocumentPageKind::Image,
        } },
        KiriView::LoadContainerImageOperation {
            KiriView::ImageDocumentPageTarget {
                sourceUrl,
                KiriView::ImageDocumentPageKind::Image,
            },
            containerUrl,
        },
        KiriView::FinishEmptyContainerNavigationOperation { containerUrl },
        KiriView::FinishContainerNavigationLoadWithErrorOperation {
            containerUrl,
            QStringLiteral("broken"),
        },
        KiriView::ReportContainerNavigationBoundaryOperation {
            KiriView::NavigationDirection::Next,
        },
        KiriView::LoadPageNavigationUrlOperation {
            KiriView::ImageDocumentPageTarget {
                sourceUrl,
                KiriView::ImageDocumentPageKind::Image,
            },
            true,
        },
        KiriView::CancelOpenOperation {},
        KiriView::ClearDisplayedImageLocationOperation {},
        KiriView::ClearPresentationImageOperation {},
        KiriView::ClearLoadingContainerNavigationUrlOperation {},
        KiriView::SetLoadingContainerNavigationUrlOperation { containerUrl },
        KiriView::SetContainerNavigationUrlOperation { containerUrl },
        KiriView::PrepareSourceLoadOperation {
            KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl),
        },
        KiriView::SetSourceUrlOperation {
            KiriView::ImageDocumentPageTarget {
                sourceUrl,
                KiriView::ImageDocumentPageKind::Image,
            },
        },
        KiriView::BeginOpenOperation {},
        KiriView::SetErrorStringOperation { QStringLiteral("failed") },
        KiriView::FinishEmptySourceLoadOperation {},
    });

    QCOMPARE(recorded.events,
        QStringList({
            QStringLiteral("cancelFileDeletion"),
            QStringLiteral("stopPresentationAnimation"),
            QStringLiteral("shutdownSpread"),
            QStringLiteral("clearMediaEntrySource"),
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
            QStringLiteral("reportContainerNavigationBoundary"),
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
        }));
}

QTEST_GUILESS_MAIN(TestImageDocumentRuntimePlanExecutor)

#include "test_imagedocumentruntimeplanexecutor.moc"

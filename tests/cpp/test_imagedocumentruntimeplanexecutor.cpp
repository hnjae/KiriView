// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntimeplanexecutor.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <QUrl>

namespace {
using kiriview::ImageDocumentRuntimeOperations;
using kiriview::ImageDocumentRuntimePlan;

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

struct RecordedRuntimeOperations {
    ImageDocumentRuntimeOperations operations;
    QStringList events;
    QUrl url;
    QUrl secondaryUrl;
    kiriview::ImageDocumentPageKind kind = kiriview::ImageDocumentPageKind::Image;
    kiriview::NavigationDirection direction = kiriview::NavigationDirection::Next;
    kiriview::ContainerNavigationListFailureKind listFailureKind
        = kiriview::ContainerNavigationListFailureKind::DirectoryListing;
    kiriview::ContainerNavigationListFailureSeverity listFailureSeverity
        = kiriview::ContainerNavigationListFailureSeverity::Diagnostic;
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
        operations.navigation.loadUrl = [this](const kiriview::ImageDocumentPageTarget &target) {
            url = target.url;
            kind = target.kind;
            record(QStringLiteral("loadUrl"));
        };
        operations.navigation.loadContainerImage
            = [this](const kiriview::ImageDocumentPageTarget &target, const QUrl &containerUrl) {
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
            = [this](kiriview::NavigationDirection boundaryDirection) {
                  direction = boundaryDirection;
                  record(QStringLiteral("reportContainerNavigationBoundary"));
              };
        operations.navigation.reportContainerNavigationListFailure
            = [this](const kiriview::ContainerNavigationListFailure &failure) {
                  url = failure.currentContainerUrl;
                  secondaryUrl = failure.parentUrl;
                  direction = failure.direction;
                  listFailureKind = failure.kind;
                  listFailureSeverity = failure.severity;
                  errorString = failure.diagnosticDetail;
                  record(QStringLiteral("reportContainerNavigationListFailure"));
              };
        operations.navigation.loadPageNavigationUrl
            = [this](const kiriview::ImageDocumentPageTarget &target, bool preserveTransition) {
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
            = [this](const kiriview::ImageDocumentSourceLoadRequest &request) {
                  url = request.sourceUrl;
                  secondaryUrl = request.containerNavigationUrl;
                  flag = request.preserveTwoPageSpreadTransition;
                  record(QStringLiteral("prepareSourceLoad"));
              };
        operations.open.setSourceUrl = [this](const kiriview::ImageDocumentPageTarget &target) {
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
        kind = kiriview::ImageDocumentPageKind::Image;
        direction = kiriview::NavigationDirection::Next;
        listFailureKind = kiriview::ContainerNavigationListFailureKind::DirectoryListing;
        listFailureSeverity = kiriview::ContainerNavigationListFailureSeverity::Diagnostic;
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
    kiriview::ImageDocumentRuntimePlanExecutor executor(recorded.operations);

    executor.dispatchPlan(kiriview::imageDocumentClearImagePlan());

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
    kiriview::ImageDocumentRuntimePlanExecutor executor(recorded.operations);

    executor.dispatchPlan(kiriview::imageDocumentClearDeletedImagePlan());

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
    kiriview::ImageDocumentRuntimePlanExecutor executor(recorded.operations);

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
    kiriview::ImageDocumentRuntimePlanExecutor executor(recorded.operations);

    executor.dispatchPlan(ImageDocumentRuntimePlan { kiriview::LoadUrlOperation {
        kiriview::ImageDocumentPageTarget {
            localUrl(QStringLiteral("/image.png")),
            kiriview::ImageDocumentPageKind::Video,
        },
    } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("loadUrl") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/image.png")));
    QCOMPARE(recorded.kind, kiriview::ImageDocumentPageKind::Video);

    recorded.clear();
    executor.dispatchPlan(ImageDocumentRuntimePlan { kiriview::LoadContainerImageOperation {
        kiriview::ImageDocumentPageTarget {
            localUrl(QStringLiteral("/book/01.png")),
            kiriview::ImageDocumentPageKind::Video,
        },
        localUrl(QStringLiteral("/book.cbz")),
    } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("loadContainerImage") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/book/01.png")));
    QCOMPARE(recorded.kind, kiriview::ImageDocumentPageKind::Video);
    QCOMPARE(recorded.secondaryUrl, localUrl(QStringLiteral("/book.cbz")));

    recorded.clear();
    executor.dispatchPlan(
        ImageDocumentRuntimePlan { kiriview::FinishEmptyContainerNavigationOperation {
            localUrl(QStringLiteral("/empty.cbz")),
        } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("finishEmptyContainerNavigation") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/empty.cbz")));

    recorded.clear();
    executor.dispatchPlan(
        ImageDocumentRuntimePlan { kiriview::FinishContainerNavigationLoadWithErrorOperation {
            localUrl(QStringLiteral("/broken.cbz")),
            QStringLiteral("broken"),
        } });
    QCOMPARE(
        recorded.events, QStringList({ QStringLiteral("finishContainerNavigationLoadWithError") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/broken.cbz")));
    QCOMPARE(recorded.errorString, QStringLiteral("broken"));

    recorded.clear();
    executor.dispatchPlan(
        ImageDocumentRuntimePlan { kiriview::ReportContainerNavigationBoundaryOperation {
            kiriview::NavigationDirection::Previous,
        } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("reportContainerNavigationBoundary") }));
    QCOMPARE(recorded.direction, kiriview::NavigationDirection::Previous);

    recorded.clear();
    executor.dispatchPlan(
        ImageDocumentRuntimePlan { kiriview::ReportContainerNavigationListFailureOperation {
            kiriview::ContainerNavigationListFailure {
                localUrl(QStringLiteral("/books/a/")),
                localUrl(QStringLiteral("/books/")),
                kiriview::NavigationDirection::Next,
                kiriview::ContainerNavigationListFailureKind::DirectoryListing,
                QStringLiteral("provider failure"),
                kiriview::ContainerNavigationListFailureSeverity::Diagnostic,
            },
        } });
    QCOMPARE(
        recorded.events, QStringList({ QStringLiteral("reportContainerNavigationListFailure") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/books/a/")));
    QCOMPARE(recorded.secondaryUrl, localUrl(QStringLiteral("/books/")));
    QCOMPARE(recorded.direction, kiriview::NavigationDirection::Next);
    QCOMPARE(
        recorded.listFailureKind, kiriview::ContainerNavigationListFailureKind::DirectoryListing);
    QCOMPARE(recorded.errorString, QStringLiteral("provider failure"));
    QCOMPARE(
        recorded.listFailureSeverity, kiriview::ContainerNavigationListFailureSeverity::Diagnostic);

    recorded.clear();
    executor.dispatchPlan(ImageDocumentRuntimePlan { kiriview::LoadPageNavigationUrlOperation {
        kiriview::ImageDocumentPageTarget {
            localUrl(QStringLiteral("/next.png")),
            kiriview::ImageDocumentPageKind::Video,
        },
        true,
    } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("loadPageNavigationUrl") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/next.png")));
    QCOMPARE(recorded.kind, kiriview::ImageDocumentPageKind::Video);
    QVERIFY(recorded.flag);

    recorded.clear();
    executor.dispatchPlan(ImageDocumentRuntimePlan { kiriview::PrepareFailedContainerOperation {
        localUrl(QStringLiteral("/bad.zip")),
    } });
    QCOMPARE(recorded.events, QStringList({ QStringLiteral("prepareFailedContainer") }));
    QCOMPARE(recorded.url, localUrl(QStringLiteral("/bad.zip")));

    recorded.clear();
    executor.dispatchPlan(ImageDocumentRuntimePlan {
        kiriview::UpdatePageNavigationOperation {},
        kiriview::ScheduleAdjacentImagePredecodeOperation {},
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
    kiriview::ImageDocumentRuntimePlanExecutor executor(recorded.operations);
    const QUrl sourceUrl = localUrl(QStringLiteral("/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/book.cbz"));

    executor.dispatchPlan({
        kiriview::CancelFileDeletionOperation {},
        kiriview::CancelAllNavigationOperation {},
        kiriview::CancelPredecodeOperation {},
        kiriview::FinishSpreadTransitionOperation {},
        kiriview::ResetRightToLeftReadingOperation {},
        kiriview::ClearSecondaryPageOperation {},
        kiriview::ClearLoadingContainerNavigationUrlOperation {},
        kiriview::SetLoadingContainerNavigationUrlOperation { containerUrl },
        kiriview::SetContainerNavigationUrlOperation { containerUrl },
        kiriview::PrepareSourceLoadOperation {
            kiriview::ImageDocumentSourceLoadRequest::fromPageNavigation(sourceUrl, true),
        },
        kiriview::SetSourceUrlOperation {
            kiriview::ImageDocumentPageTarget {
                sourceUrl,
                kiriview::ImageDocumentPageKind::Video,
            },
        },
        kiriview::BeginOpenOperation {},
        kiriview::NotifyRightToLeftReadingChangedOperation {},
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
    QCOMPARE(recorded.kind, kiriview::ImageDocumentPageKind::Video);
    QVERIFY(recorded.secondaryUrl.isEmpty());
    QVERIFY(recorded.flag);
}

void TestImageDocumentRuntimePlanExecutor::runtimePlansDispatchEveryOperationExplicitly()
{
    RecordedRuntimeOperations recorded;
    kiriview::ImageDocumentRuntimePlanExecutor executor(recorded.operations);
    const QUrl sourceUrl = localUrl(QStringLiteral("/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/book.cbz"));

    executor.dispatchPlan({
        kiriview::CancelFileDeletionOperation {},
        kiriview::StopPresentationAnimationOperation {},
        kiriview::ShutdownSpreadOperation {},
        kiriview::ClearMediaEntrySourceOperation {},
        kiriview::ClearPredecodeOperation {},
        kiriview::CancelPredecodeOperation {},
        kiriview::ScheduleAdjacentImagePredecodeOperation {},
        kiriview::FinishSpreadTransitionOperation {},
        kiriview::ResetRightToLeftReadingOperation {},
        kiriview::ClearSecondaryPageOperation {},
        kiriview::NotifyRightToLeftReadingChangedOperation {},
        kiriview::ResetZoomOperation {},
        kiriview::PrepareFailedContainerOperation { containerUrl },
        kiriview::CancelPageNavigationUpdateOperation {},
        kiriview::CancelNavigationOperation {},
        kiriview::CancelContainerNavigationOperation {},
        kiriview::CancelAllNavigationOperation {},
        kiriview::ClearPageNavigationOperation {},
        kiriview::UpdatePageNavigationOperation {},
        kiriview::LoadUrlOperation { kiriview::ImageDocumentPageTarget {
            sourceUrl,
            kiriview::ImageDocumentPageKind::Image,
        } },
        kiriview::LoadContainerImageOperation {
            kiriview::ImageDocumentPageTarget {
                sourceUrl,
                kiriview::ImageDocumentPageKind::Image,
            },
            containerUrl,
        },
        kiriview::FinishEmptyContainerNavigationOperation { containerUrl },
        kiriview::FinishContainerNavigationLoadWithErrorOperation {
            containerUrl,
            QStringLiteral("broken"),
        },
        kiriview::ReportContainerNavigationBoundaryOperation {
            kiriview::NavigationDirection::Next,
        },
        kiriview::ReportContainerNavigationListFailureOperation {
            kiriview::ContainerNavigationListFailure {
                sourceUrl,
                containerUrl,
                kiriview::NavigationDirection::Next,
                kiriview::ContainerNavigationListFailureKind::DirectoryListing,
                QStringLiteral("provider failure"),
                kiriview::ContainerNavigationListFailureSeverity::Diagnostic,
            },
        },
        kiriview::LoadPageNavigationUrlOperation {
            kiriview::ImageDocumentPageTarget {
                sourceUrl,
                kiriview::ImageDocumentPageKind::Image,
            },
            true,
        },
        kiriview::CancelOpenOperation {},
        kiriview::ClearDisplayedImageLocationOperation {},
        kiriview::ClearPresentationImageOperation {},
        kiriview::ClearLoadingContainerNavigationUrlOperation {},
        kiriview::SetLoadingContainerNavigationUrlOperation { containerUrl },
        kiriview::SetContainerNavigationUrlOperation { containerUrl },
        kiriview::PrepareSourceLoadOperation {
            kiriview::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl),
        },
        kiriview::SetSourceUrlOperation {
            kiriview::ImageDocumentPageTarget {
                sourceUrl,
                kiriview::ImageDocumentPageKind::Image,
            },
        },
        kiriview::BeginOpenOperation {},
        kiriview::SetErrorStringOperation { QStringLiteral("failed") },
        kiriview::FinishEmptySourceLoadOperation {},
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
            QStringLiteral("reportContainerNavigationListFailure"),
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

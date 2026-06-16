// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntimeworkflow.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <utility>

namespace {
struct RecordedWorkflowOperations {
    kiriview::ImageDocumentRuntimeOperations operations;
    QStringList events;

    RecordedWorkflowOperations()
    {
        operations.mediaEntrySource.clear
            = [this]() { events.append(QStringLiteral("clearMediaEntrySource")); };
        operations.predecode.clearPredecode
            = [this]() { events.append(QStringLiteral("clearPredecode")); };
        operations.spread.finishSpreadTransition
            = [this]() { events.append(QStringLiteral("finishSpreadTransition")); };
        operations.spread.clearSecondaryPage
            = [this]() { events.append(QStringLiteral("clearSecondaryPage")); };
        operations.navigation.cancelPageNavigationUpdate
            = [this]() { events.append(QStringLiteral("cancelPageNavigationUpdate")); };
        operations.open.clearDisplayedImageLocation
            = [this]() { events.append(QStringLiteral("clearDisplayedImageLocation")); };
        operations.open.clearPresentationImage
            = [this]() { events.append(QStringLiteral("clearPresentationImage")); };
        operations.navigation.clearPageNavigation
            = [this]() { events.append(QStringLiteral("clearPageNavigation")); };
        operations.spread.notifyRightToLeftReadingChanged
            = [this]() { events.append(QStringLiteral("notifyRightToLeftReadingChanged")); };
    }
};
}

class TestImageDocumentRuntimeWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dispatchesRuntimePlansThroughInjectedOperations();
};

void TestImageDocumentRuntimeWorkflow::dispatchesRuntimePlansThroughInjectedOperations()
{
    RecordedWorkflowOperations recorded;
    kiriview::ImageDocumentRuntimeWorkflow workflow(std::move(recorded.operations));

    workflow.dispatchPlan(kiriview::imageDocumentClearImagePlan());

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

QTEST_GUILESS_MAIN(TestImageDocumentRuntimeWorkflow)

#include "test_imagedocumentruntimeworkflow.moc"

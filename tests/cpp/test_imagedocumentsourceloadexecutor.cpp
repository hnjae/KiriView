// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadexecutor.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <QUrl>

class TestImageDocumentSourceLoadExecutor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void replacementLoadExecutesRuntimeEffectsAroundSourceMutation();
    void currentLoadExecutesReadingResetBeforeSourceState();
};

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

void appendEvent(QStringList *events, const QString &event) { events->append(event); }

void appendUrlEvent(QStringList *events, const QString &event, const QUrl &url)
{
    events->append(event + QLatin1Char('=') + url.toString());
}

KiriView::ImageDocumentSourceLoadOperations recordingOperations(QStringList *events)
{
    KiriView::ImageDocumentSourceLoadOperations operations;
    operations.cancelNavigationAndPredecode
        = [events]() { appendEvent(events, QStringLiteral("cancelNavigationAndPredecode")); };
    operations.finishSpreadTransition
        = [events]() { appendEvent(events, QStringLiteral("finishSpreadTransition")); };
    operations.resetRightToLeftReading
        = [events]() { appendEvent(events, QStringLiteral("resetRightToLeftReading")); };
    operations.notifyRightToLeftReadingChanged
        = [events]() { appendEvent(events, QStringLiteral("notifyRightToLeftReadingChanged")); };
    operations.clearSecondaryPage
        = [events]() { appendEvent(events, QStringLiteral("clearSecondaryPage")); };
    operations.clearLoadingContainerNavigationUrl
        = [events]() { appendEvent(events, QStringLiteral("clearLoadingContainerNavigationUrl")); };
    operations.setLoadingContainerNavigationUrl = [events](const QUrl &url) {
        appendUrlEvent(events, QStringLiteral("setLoadingContainerNavigationUrl"), url);
    };
    operations.setContainerNavigationUrl = [events](const QUrl &url) {
        appendUrlEvent(events, QStringLiteral("setContainerNavigationUrl"), url);
    };
    operations.prepareSourceLoad
        = [events](const KiriView::ImageDocumentSourceLoadRequest &request) {
              appendUrlEvent(events, QStringLiteral("prepareSourceLoad"), request.sourceUrl);
          };
    operations.setSourceUrl = [events](const QUrl &url) {
        appendUrlEvent(events, QStringLiteral("setSourceUrl"), url);
    };
    operations.beginOpen = [events]() { appendEvent(events, QStringLiteral("beginOpen")); };
    return operations;
}
}

void TestImageDocumentSourceLoadExecutor::
    replacementLoadExecutesRuntimeEffectsAroundSourceMutation()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/images/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl);

    KiriView::ImageDocumentSourceLoadPlan plan;
    using Operation = KiriView::ImageDocumentSourceLoadOperation;
    plan.operations = {
        Operation::CancelNavigationAndPredecode,
        Operation::FinishSpreadTransition,
        Operation::ResetRightToLeftReading,
        Operation::ClearSecondaryPage,
        Operation::SetLoadingContainerNavigationUrlToRequested,
        Operation::PrepareSourceLoad,
        Operation::SetSourceUrlToRequested,
        Operation::BeginOpen,
        Operation::NotifyRightToLeftReadingChanged,
    };

    QStringList events;
    KiriView::executeImageDocumentSourceLoadPlan(request, plan, recordingOperations(&events));

    QCOMPARE(events,
        QStringList({
            QStringLiteral("cancelNavigationAndPredecode"),
            QStringLiteral("finishSpreadTransition"),
            QStringLiteral("resetRightToLeftReading"),
            QStringLiteral("clearSecondaryPage"),
            QStringLiteral("setLoadingContainerNavigationUrl=%1").arg(containerUrl.toString()),
            QStringLiteral("prepareSourceLoad=%1").arg(sourceUrl.toString()),
            QStringLiteral("setSourceUrl=%1").arg(sourceUrl.toString()),
            QStringLiteral("beginOpen"),
            QStringLiteral("notifyRightToLeftReadingChanged"),
        }));
}

void TestImageDocumentSourceLoadExecutor::currentLoadExecutesReadingResetBeforeSourceState()
{
    const QUrl sourceUrl = localUrl(QStringLiteral("/images/page.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const KiriView::ImageDocumentSourceLoadRequest request
        = KiriView::ImageDocumentSourceLoadRequest::fromContainerImage(sourceUrl, containerUrl);

    KiriView::ImageDocumentSourceLoadPlan plan;
    using Operation = KiriView::ImageDocumentSourceLoadOperation;
    plan.operations = {
        Operation::ResetRightToLeftReading,
        Operation::NotifyRightToLeftReadingChanged,
        Operation::ClearLoadingContainerNavigationUrl,
        Operation::SetContainerNavigationUrlToRequested,
    };

    QStringList events;
    KiriView::executeImageDocumentSourceLoadPlan(request, plan, recordingOperations(&events));

    QCOMPARE(events,
        QStringList({
            QStringLiteral("resetRightToLeftReading"),
            QStringLiteral("notifyRightToLeftReadingChanged"),
            QStringLiteral("clearLoadingContainerNavigationUrl"),
            QStringLiteral("setContainerNavigationUrl=%1").arg(containerUrl.toString()),
        }));
}

QTEST_GUILESS_MAIN(TestImageDocumentSourceLoadExecutor)

#include "test_imagedocumentsourceloadexecutor.moc"

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentnavigationruntimeplan.h"

#include "image_document_plan_test_support.h"
#include "localization/imageerrortext.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>

namespace {
using KiriView::ImageDocumentRuntimePlan;
using KiriView::TestSupport::hasOperationTypes;
using KiriView::TestSupport::operationAt;
using KiriView::TestSupport::operationTypes;

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }
}

class TestImageDocumentNavigationRuntimePlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void openImageDocumentPageNavigationMapsToLoadUrl();
    void openContainerImageDocumentPageNavigationMapsToContainerLoad();
    void emptyContainerErrorMapsToEmptyContainerCompletion();
    void invalidComicMediaEntrySourceErrorMapsToLocalizedOpenError();
    void genericContainerErrorKeepsReportedErrorString();
    void containerBoundaryMapsToBoundaryOperation();
    void containerListErrorIsDiagnosticOnly();
    void clearCurrentImageDocumentPageNavigationExpandsDeletedImageClearPlan();
    void mixedNavigationPlanPreservesOperationOrder();
};

void TestImageDocumentNavigationRuntimePlan::openImageDocumentPageNavigationMapsToLoadUrl()
{
    const QUrl url = localUrl(QStringLiteral("/images/02.png"));
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentRuntimePlanForNavigationPlan({
        KiriView::OpenImageDocumentPageUrlEffect {
            KiriView::ImageDocumentPageTarget { url, KiriView::ImageDocumentPageKind::Video },
        },
    });

    QVERIFY(hasOperationTypes(plan, operationTypes<KiriView::LoadUrlOperation>()));
    QCOMPARE(operationAt<KiriView::LoadUrlOperation>(plan, 0).target.url, url);
    QCOMPARE(operationAt<KiriView::LoadUrlOperation>(plan, 0).target.kind,
        KiriView::ImageDocumentPageKind::Video);
}

void TestImageDocumentNavigationRuntimePlan::
    openContainerImageDocumentPageNavigationMapsToContainerLoad()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/books/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentRuntimePlanForNavigationPlan({
        KiriView::OpenContainerImageDocumentPageNavigationEffect {
            KiriView::ImageDocumentPageTarget {
                imageUrl,
                KiriView::ImageDocumentPageKind::Video,
            },
            containerUrl,
        },
    });

    QVERIFY(hasOperationTypes(plan, operationTypes<KiriView::LoadContainerImageOperation>()));
    QCOMPARE(operationAt<KiriView::LoadContainerImageOperation>(plan, 0).target.url, imageUrl);
    QCOMPARE(operationAt<KiriView::LoadContainerImageOperation>(plan, 0).target.kind,
        KiriView::ImageDocumentPageKind::Video);
    QCOMPARE(
        operationAt<KiriView::LoadContainerImageOperation>(plan, 0).containerUrl, containerUrl);
}

void TestImageDocumentNavigationRuntimePlan::emptyContainerErrorMapsToEmptyContainerCompletion()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/empty.cbz"));
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentRuntimePlanForNavigationPlan({
        KiriView::ReportContainerNavigationErrorEffect {
            containerUrl,
            KiriView::ContainerNavigationError::EmptyContainer,
            QStringLiteral("ignored"),
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<KiriView::FinishEmptyContainerNavigationOperation>()));
    QCOMPARE(operationAt<KiriView::FinishEmptyContainerNavigationOperation>(plan, 0).containerUrl,
        containerUrl);
}

void TestImageDocumentNavigationRuntimePlan::
    invalidComicMediaEntrySourceErrorMapsToLocalizedOpenError()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/broken.cbz"));
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentRuntimePlanForNavigationPlan({
        KiriView::ReportContainerNavigationErrorEffect {
            containerUrl,
            KiriView::ContainerNavigationError::InvalidComicBookArchive,
            QStringLiteral("ignored"),
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<KiriView::FinishContainerNavigationLoadWithErrorOperation>()));
    QCOMPARE(operationAt<KiriView::FinishContainerNavigationLoadWithErrorOperation>(plan, 0)
                 .containerUrl,
        containerUrl);
    QCOMPARE(
        operationAt<KiriView::FinishContainerNavigationLoadWithErrorOperation>(plan, 0).errorString,
        KiriView::imageErrorText(KiriView::ImageErrorTextId::OpenComicBookArchive));
}

void TestImageDocumentNavigationRuntimePlan::genericContainerErrorKeepsReportedErrorString()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/broken.zip"));
    const QString errorString = QStringLiteral("provider failure");
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentRuntimePlanForNavigationPlan({
        KiriView::ReportContainerNavigationErrorEffect {
            containerUrl,
            KiriView::ContainerNavigationError::Generic,
            errorString,
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<KiriView::FinishContainerNavigationLoadWithErrorOperation>()));
    QCOMPARE(operationAt<KiriView::FinishContainerNavigationLoadWithErrorOperation>(plan, 0)
                 .containerUrl,
        containerUrl);
    QCOMPARE(
        operationAt<KiriView::FinishContainerNavigationLoadWithErrorOperation>(plan, 0).errorString,
        errorString);
}

void TestImageDocumentNavigationRuntimePlan::containerBoundaryMapsToBoundaryOperation()
{
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentRuntimePlanForNavigationPlan({
        KiriView::ReportContainerNavigationBoundaryEffect {
            KiriView::NavigationDirection::Previous,
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<KiriView::ReportContainerNavigationBoundaryOperation>()));
    QCOMPARE(operationAt<KiriView::ReportContainerNavigationBoundaryOperation>(plan, 0).direction,
        KiriView::NavigationDirection::Previous);
}

void TestImageDocumentNavigationRuntimePlan::containerListErrorIsDiagnosticOnly()
{
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QString diagnostic = QStringLiteral("provider failure");
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentRuntimePlanForNavigationPlan({
        KiriView::ReportContainerNavigationListErrorEffect {
            KiriView::ContainerNavigationListFailure {
                currentContainerUrl,
                parentUrl,
                KiriView::NavigationDirection::Next,
                KiriView::ContainerNavigationListFailureKind::DirectoryListing,
                diagnostic,
                KiriView::ContainerNavigationListFailureSeverity::Diagnostic,
            },
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<KiriView::ReportContainerNavigationListFailureOperation>()));
    const KiriView::ContainerNavigationListFailure &failure
        = operationAt<KiriView::ReportContainerNavigationListFailureOperation>(plan, 0).failure;
    QCOMPARE(failure.currentContainerUrl, currentContainerUrl);
    QCOMPARE(failure.parentUrl, parentUrl);
    QCOMPARE(failure.direction, KiriView::NavigationDirection::Next);
    QCOMPARE(failure.kind, KiriView::ContainerNavigationListFailureKind::DirectoryListing);
    QCOMPARE(failure.diagnosticDetail, diagnostic);
    QCOMPARE(failure.severity, KiriView::ContainerNavigationListFailureSeverity::Diagnostic);
}

void TestImageDocumentNavigationRuntimePlan::
    clearCurrentImageDocumentPageNavigationExpandsDeletedImageClearPlan()
{
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentRuntimePlanForNavigationPlan({
        KiriView::ClearCurrentImageDocumentPageNavigationEffect {},
    });
    const ImageDocumentRuntimePlan expected = KiriView::imageDocumentClearDeletedImagePlan();

    QCOMPARE(plan.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(plan.at(index).index(), expected.at(index).index());
    }
}

void TestImageDocumentNavigationRuntimePlan::mixedNavigationPlanPreservesOperationOrder()
{
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl imageUrl = localUrl(QStringLiteral("/books/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const ImageDocumentRuntimePlan plan = KiriView::imageDocumentRuntimePlanForNavigationPlan({
        KiriView::OpenImageDocumentPageUrlEffect {
            KiriView::ImageDocumentPageTarget {
                firstUrl,
                KiriView::ImageDocumentPageKind::Image,
            },
        },
        KiriView::OpenContainerImageDocumentPageNavigationEffect {
            KiriView::ImageDocumentPageTarget {
                imageUrl,
                KiriView::ImageDocumentPageKind::Image,
            },
            containerUrl,
        },
        KiriView::ReportContainerNavigationErrorEffect {
            containerUrl,
            KiriView::ContainerNavigationError::EmptyContainer,
            QString(),
        },
        KiriView::ReportContainerNavigationBoundaryEffect {
            KiriView::NavigationDirection::Next,
        },
    });

    QVERIFY(hasOperationTypes(plan,
        operationTypes<KiriView::LoadUrlOperation, KiriView::LoadContainerImageOperation,
            KiriView::FinishEmptyContainerNavigationOperation,
            KiriView::ReportContainerNavigationBoundaryOperation>()));
    QCOMPARE(operationAt<KiriView::LoadUrlOperation>(plan, 0).target.url, firstUrl);
    QCOMPARE(operationAt<KiriView::LoadContainerImageOperation>(plan, 1).target.url, imageUrl);
    QCOMPARE(operationAt<KiriView::FinishEmptyContainerNavigationOperation>(plan, 2).containerUrl,
        containerUrl);
    QCOMPARE(operationAt<KiriView::ReportContainerNavigationBoundaryOperation>(plan, 3).direction,
        KiriView::NavigationDirection::Next);
}

QTEST_GUILESS_MAIN(TestImageDocumentNavigationRuntimePlan)

#include "test_imagedocumentnavigationruntimeplan.moc"

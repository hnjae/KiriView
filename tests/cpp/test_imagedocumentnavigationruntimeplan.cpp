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
using kiriview::ImageDocumentRuntimePlan;
using kiriview::TestSupport::hasOperationTypes;
using kiriview::TestSupport::operationAt;
using kiriview::TestSupport::operationTypes;

QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }
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
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentRuntimePlanForNavigationPlan({
        kiriview::OpenImageDocumentPageUrlEffect {
            kiriview::ImageDocumentPageTarget { url, kiriview::ImageDocumentPageKind::Video },
        },
    });

    QVERIFY(hasOperationTypes(plan, operationTypes<kiriview::LoadUrlOperation>()));
    QCOMPARE(operationAt<kiriview::LoadUrlOperation>(plan, 0).target.url, url);
    QCOMPARE(operationAt<kiriview::LoadUrlOperation>(plan, 0).target.kind,
        kiriview::ImageDocumentPageKind::Video);
}

void TestImageDocumentNavigationRuntimePlan::
    openContainerImageDocumentPageNavigationMapsToContainerLoad()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/books/book/01.png"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentRuntimePlanForNavigationPlan({
        kiriview::OpenContainerImageDocumentPageNavigationEffect {
            kiriview::ImageDocumentPageTarget {
                imageUrl,
                kiriview::ImageDocumentPageKind::Video,
            },
            containerUrl,
        },
    });

    QVERIFY(hasOperationTypes(plan, operationTypes<kiriview::LoadContainerImageOperation>()));
    QCOMPARE(operationAt<kiriview::LoadContainerImageOperation>(plan, 0).target.url, imageUrl);
    QCOMPARE(operationAt<kiriview::LoadContainerImageOperation>(plan, 0).target.kind,
        kiriview::ImageDocumentPageKind::Video);
    QCOMPARE(
        operationAt<kiriview::LoadContainerImageOperation>(plan, 0).containerUrl, containerUrl);
}

void TestImageDocumentNavigationRuntimePlan::emptyContainerErrorMapsToEmptyContainerCompletion()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/empty.cbz"));
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentRuntimePlanForNavigationPlan({
        kiriview::ReportContainerNavigationErrorEffect {
            containerUrl,
            kiriview::ContainerNavigationError::EmptyContainer,
            QStringLiteral("ignored"),
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<kiriview::FinishEmptyContainerNavigationOperation>()));
    QCOMPARE(operationAt<kiriview::FinishEmptyContainerNavigationOperation>(plan, 0).containerUrl,
        containerUrl);
}

void TestImageDocumentNavigationRuntimePlan::
    invalidComicMediaEntrySourceErrorMapsToLocalizedOpenError()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/broken.cbz"));
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentRuntimePlanForNavigationPlan({
        kiriview::ReportContainerNavigationErrorEffect {
            containerUrl,
            kiriview::ContainerNavigationError::InvalidComicBookArchive,
            QStringLiteral("ignored"),
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<kiriview::FinishContainerNavigationLoadWithErrorOperation>()));
    QCOMPARE(operationAt<kiriview::FinishContainerNavigationLoadWithErrorOperation>(plan, 0)
                 .containerUrl,
        containerUrl);
    QCOMPARE(
        operationAt<kiriview::FinishContainerNavigationLoadWithErrorOperation>(plan, 0).errorString,
        kiriview::imageErrorText(kiriview::ImageErrorTextId::OpenComicBookArchive));
}

void TestImageDocumentNavigationRuntimePlan::genericContainerErrorKeepsReportedErrorString()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/broken.zip"));
    const QString errorString = QStringLiteral("provider failure");
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentRuntimePlanForNavigationPlan({
        kiriview::ReportContainerNavigationErrorEffect {
            containerUrl,
            kiriview::ContainerNavigationError::Generic,
            errorString,
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<kiriview::FinishContainerNavigationLoadWithErrorOperation>()));
    QCOMPARE(operationAt<kiriview::FinishContainerNavigationLoadWithErrorOperation>(plan, 0)
                 .containerUrl,
        containerUrl);
    QCOMPARE(
        operationAt<kiriview::FinishContainerNavigationLoadWithErrorOperation>(plan, 0).errorString,
        errorString);
}

void TestImageDocumentNavigationRuntimePlan::containerBoundaryMapsToBoundaryOperation()
{
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentRuntimePlanForNavigationPlan({
        kiriview::ReportContainerNavigationBoundaryEffect {
            kiriview::NavigationDirection::Previous,
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<kiriview::ReportContainerNavigationBoundaryOperation>()));
    QCOMPARE(operationAt<kiriview::ReportContainerNavigationBoundaryOperation>(plan, 0).direction,
        kiriview::NavigationDirection::Previous);
}

void TestImageDocumentNavigationRuntimePlan::containerListErrorIsDiagnosticOnly()
{
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QString diagnostic = QStringLiteral("provider failure");
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentRuntimePlanForNavigationPlan({
        kiriview::ReportContainerNavigationListErrorEffect {
            kiriview::ContainerNavigationListFailure {
                currentContainerUrl,
                parentUrl,
                kiriview::NavigationDirection::Next,
                kiriview::ContainerNavigationListFailureKind::DirectoryListing,
                diagnostic,
                kiriview::ContainerNavigationListFailureSeverity::Diagnostic,
            },
        },
    });

    QVERIFY(hasOperationTypes(
        plan, operationTypes<kiriview::ReportContainerNavigationListFailureOperation>()));
    const kiriview::ContainerNavigationListFailure& failure
        = operationAt<kiriview::ReportContainerNavigationListFailureOperation>(plan, 0).failure;
    QCOMPARE(failure.currentContainerUrl, currentContainerUrl);
    QCOMPARE(failure.parentUrl, parentUrl);
    QCOMPARE(failure.direction, kiriview::NavigationDirection::Next);
    QCOMPARE(failure.kind, kiriview::ContainerNavigationListFailureKind::DirectoryListing);
    QCOMPARE(failure.diagnosticDetail, diagnostic);
    QCOMPARE(failure.severity, kiriview::ContainerNavigationListFailureSeverity::Diagnostic);
}

void TestImageDocumentNavigationRuntimePlan::
    clearCurrentImageDocumentPageNavigationExpandsDeletedImageClearPlan()
{
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentRuntimePlanForNavigationPlan({
        kiriview::ClearCurrentImageDocumentPageNavigationEffect {},
    });
    const ImageDocumentRuntimePlan expected = kiriview::imageDocumentClearDeletedImagePlan();

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
    const ImageDocumentRuntimePlan plan = kiriview::imageDocumentRuntimePlanForNavigationPlan({
        kiriview::OpenImageDocumentPageUrlEffect {
            kiriview::ImageDocumentPageTarget {
                firstUrl,
                kiriview::ImageDocumentPageKind::Image,
            },
        },
        kiriview::OpenContainerImageDocumentPageNavigationEffect {
            kiriview::ImageDocumentPageTarget {
                imageUrl,
                kiriview::ImageDocumentPageKind::Image,
            },
            containerUrl,
        },
        kiriview::ReportContainerNavigationErrorEffect {
            containerUrl,
            kiriview::ContainerNavigationError::EmptyContainer,
            QString(),
        },
        kiriview::ReportContainerNavigationBoundaryEffect {
            kiriview::NavigationDirection::Next,
        },
    });

    QVERIFY(hasOperationTypes(plan,
        operationTypes<kiriview::LoadUrlOperation, kiriview::LoadContainerImageOperation,
            kiriview::FinishEmptyContainerNavigationOperation,
            kiriview::ReportContainerNavigationBoundaryOperation>()));
    QCOMPARE(operationAt<kiriview::LoadUrlOperation>(plan, 0).target.url, firstUrl);
    QCOMPARE(operationAt<kiriview::LoadContainerImageOperation>(plan, 1).target.url, imageUrl);
    QCOMPARE(operationAt<kiriview::FinishEmptyContainerNavigationOperation>(plan, 2).containerUrl,
        containerUrl);
    QCOMPARE(operationAt<kiriview::ReportContainerNavigationBoundaryOperation>(plan, 3).direction,
        kiriview::NavigationDirection::Next);
}

QTEST_GUILESS_MAIN(TestImageDocumentNavigationRuntimePlan)

#include "test_imagedocumentnavigationruntimeplan.moc"

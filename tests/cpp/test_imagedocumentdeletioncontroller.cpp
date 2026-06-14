// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentdeletioncontroller.h"

#include "document/imagedocumentstate.h"
#include "image_test_support.h"
#include "presentation/imagepagesurfacecontroller.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace {
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::testImage;

kiriview::ImageCacheBudgets testCacheBudgets()
{
    return kiriview::ImageCacheBudgets {
        1024 * 1024,
        512 * 1024,
    };
}

struct ManualImageDocumentPageCandidateLoad {
    QObject *object = nullptr;
    QUrl url;
    kiriview::ImageDocumentPageCandidatesCallback callback;
    kiriview::ErrorCallback errorCallback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualDeletionCandidateProvider
{
public:
    kiriview::ImageDocumentPageCandidateProvider provider()
    {
        kiriview::ImageDocumentPageCandidateProvider provider;
        provider.directoryImageDocumentPages
            = [this](QObject *receiver, QUrl directoryUrl,
                  kiriview::ImageDocumentPageCandidatesCallback callback,
                  kiriview::ErrorCallback errorCallback) {
                  auto load = std::make_shared<ManualImageDocumentPageCandidateLoad>();
                  load->url = std::move(directoryUrl);
                  load->callback = std::move(callback);
                  load->errorCallback = std::move(errorCallback);

                  kiriview::ImageIoJob job
                      = kiriview::TestSupport::Detail::startManualIoJob(receiver, load);
                  m_imageLoads.push_back(load);
                  return job;
              };
        provider.directoryContainers
            = [](QObject *, QUrl, kiriview::ContainerCandidatesCallback, kiriview::ErrorCallback) {
                  return kiriview::ImageIoJob();
              };
        provider.openedCollectionCandidates
            = [](QObject *, kiriview::OpenedCollectionScopeLocation,
                  kiriview::ImageDocumentPageCandidatesCallback,
                  kiriview::ErrorCallback) { return kiriview::ImageIoJob(); };
        provider.directoryImageDocumentPageChanges
            = [](QObject *, QUrl, kiriview::ImageDocumentPageCandidatesCallback,
                  kiriview::ErrorCallback) { return kiriview::ImageIoJob(); };
        return provider;
    }

    std::size_t imageLoadCount() const { return m_imageLoads.size(); }

    ManualImageDocumentPageCandidateLoad &backImageLoad() { return *m_imageLoads.back(); }

    void deliverBackImageDocumentPageCandidatesIgnoringCancellation(
        std::vector<kiriview::ImageDocumentPageCandidate> candidates)
    {
        if (m_imageLoads.back()->callback) {
            m_imageLoads.back()->callback(std::move(candidates));
        }
    }

private:
    std::vector<std::shared_ptr<ManualImageDocumentPageCandidateLoad>> m_imageLoads;
};

template <typename Operation>
const Operation *findOperation(const kiriview::ImageDocumentRuntimePlan &plan)
{
    for (const kiriview::ImageDocumentRuntimeOperation &operation : plan) {
        if (const auto *payload = std::get_if<Operation>(&operation)) {
            return payload;
        }
    }

    return nullptr;
}
}

class TestImageDocumentDeletionController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loadingDocumentWithRetainedImageDoesNotStartFileOperation();
    void erroredDocumentWithRetainedImageDoesNotStartFileOperation();
    void canceledFileDeletionCompletionIsIgnored();
    void canceledFallbackCandidateCompletionIsIgnored();
};

void TestImageDocumentDeletionController::
    loadingDocumentWithRetainedImageDoesNotStartFileOperation()
{
    QObject parent;
    kiriview::ImageDocumentState state;
    kiriview::ImagePageSurfaceController pageSurface(&parent, {}, testCacheBudgets());
    kiriview::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    ManualDeletionCandidateProvider candidateProvider;
    std::vector<kiriview::ImageDocumentRuntimePlan> runtimePlans;
    std::vector<QString> failures;
    int inProgressChangeCount = 0;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    state.setStatus(kiriview::ImageDocumentStatus::Loading);
    state.setLoading(true);
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(imageUrl));
    pageSurface.setImage(testImage(2, 1), false);

    kiriview::ImageDocumentDeletionController controller(&parent, state, pageSurface,
        candidateProvider.provider(),
        kiriview::TestSupport::fileDeletionProviderFor(fileDeletionProvider),
        kiriview::ImageDocumentDeletionController::Callbacks {
            [&inProgressChangeCount]() { ++inProgressChangeCount; },
            [&runtimePlans](kiriview::ImageDocumentRuntimePlan plan) {
                runtimePlans.push_back(std::move(plan));
            },
            [&failures](const QString &errorString) { failures.push_back(errorString); },
        });

    controller.deleteDisplayedFile(kiriview::FileDeletionMode::MoveToTrash);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(0));
    QVERIFY(!controller.inProgress());
    QCOMPARE(inProgressChangeCount, 0);
    QVERIFY(runtimePlans.empty());
    QVERIFY(failures.empty());
}

void TestImageDocumentDeletionController::
    erroredDocumentWithRetainedImageDoesNotStartFileOperation()
{
    QObject parent;
    kiriview::ImageDocumentState state;
    kiriview::ImagePageSurfaceController pageSurface(&parent, {}, testCacheBudgets());
    kiriview::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    ManualDeletionCandidateProvider candidateProvider;
    std::vector<kiriview::ImageDocumentRuntimePlan> runtimePlans;
    std::vector<QString> failures;
    int inProgressChangeCount = 0;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    state.setStatus(kiriview::ImageDocumentStatus::Error);
    state.setErrorString(QStringLiteral("decode failed"));
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(imageUrl));
    pageSurface.setImage(testImage(2, 1), false);

    kiriview::ImageDocumentDeletionController controller(&parent, state, pageSurface,
        candidateProvider.provider(),
        kiriview::TestSupport::fileDeletionProviderFor(fileDeletionProvider),
        kiriview::ImageDocumentDeletionController::Callbacks {
            [&inProgressChangeCount]() { ++inProgressChangeCount; },
            [&runtimePlans](kiriview::ImageDocumentRuntimePlan plan) {
                runtimePlans.push_back(std::move(plan));
            },
            [&failures](const QString &errorString) { failures.push_back(errorString); },
        });

    controller.deleteDisplayedFile(kiriview::FileDeletionMode::DeletePermanently);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(0));
    QVERIFY(!controller.inProgress());
    QCOMPARE(inProgressChangeCount, 0);
    QVERIFY(runtimePlans.empty());
    QVERIFY(failures.empty());
}

void TestImageDocumentDeletionController::canceledFileDeletionCompletionIsIgnored()
{
    QObject parent;
    kiriview::ImageDocumentState state;
    kiriview::ImagePageSurfaceController pageSurface(&parent, {}, testCacheBudgets());
    kiriview::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    ManualDeletionCandidateProvider candidateProvider;
    std::vector<kiriview::ImageDocumentRuntimePlan> runtimePlans;
    std::vector<QString> failures;
    int inProgressChangeCount = 0;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    state.setStatus(kiriview::ImageDocumentStatus::Ready);
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(imageUrl));
    pageSurface.setImage(testImage(2, 1), false);

    kiriview::ImageDocumentDeletionController controller(&parent, state, pageSurface,
        candidateProvider.provider(),
        kiriview::TestSupport::fileDeletionProviderFor(fileDeletionProvider),
        kiriview::ImageDocumentDeletionController::Callbacks {
            [&inProgressChangeCount]() { ++inProgressChangeCount; },
            [&runtimePlans](kiriview::ImageDocumentRuntimePlan plan) {
                runtimePlans.push_back(std::move(plan));
            },
            [&failures](const QString &errorString) { failures.push_back(errorString); },
        });

    controller.deleteDisplayedFile(kiriview::FileDeletionMode::MoveToTrash);
    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QVERIFY(controller.inProgress());

    controller.cancel();
    QVERIFY(!controller.inProgress());
    QVERIFY(fileDeletionProvider.backOperation().canceled);

    fileDeletionProvider.backOperation().callback(kiriview::FileDeletionResult::Succeeded,
        kiriview::TestSupport::manualFileDeletionFailure(
            fileDeletionProvider.backOperation().request, kiriview::FileDeletionResult::Succeeded,
            QString()));

    QVERIFY(runtimePlans.empty());
    QVERIFY(failures.empty());
    QCOMPARE(inProgressChangeCount, 2);
}

void TestImageDocumentDeletionController::canceledFallbackCandidateCompletionIsIgnored()
{
    QObject parent;
    kiriview::ImageDocumentState state;
    kiriview::ImagePageSurfaceController pageSurface(&parent, {}, testCacheBudgets());
    kiriview::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    ManualDeletionCandidateProvider candidateProvider;
    std::vector<kiriview::ImageDocumentRuntimePlan> runtimePlans;
    std::vector<QString> failures;
    const QUrl currentUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/02.png"));
    state.setStatus(kiriview::ImageDocumentStatus::Ready);
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(currentUrl));
    pageSurface.setImage(testImage(2, 1), false);

    kiriview::ImageDocumentDeletionController controller(&parent, state, pageSurface,
        candidateProvider.provider(),
        kiriview::TestSupport::fileDeletionProviderFor(fileDeletionProvider),
        kiriview::ImageDocumentDeletionController::Callbacks {
            {},
            [&runtimePlans](kiriview::ImageDocumentRuntimePlan plan) {
                runtimePlans.push_back(std::move(plan));
            },
            [&failures](const QString &errorString) { failures.push_back(errorString); },
        });

    controller.deleteDisplayedFile(kiriview::FileDeletionMode::MoveToTrash);
    fileDeletionProvider.finishBackOperation(kiriview::FileDeletionResult::Succeeded);
    QCOMPARE(candidateProvider.imageLoadCount(), std::size_t(1));
    QCOMPARE(runtimePlans.size(), std::size_t(1));
    QVERIFY(
        findOperation<kiriview::FinishEmptySourceLoadOperation>(runtimePlans.front()) != nullptr);

    controller.cancel();
    QVERIFY(candidateProvider.backImageLoad().canceled);

    candidateProvider.deliverBackImageDocumentPageCandidatesIgnoringCancellation(
        { imageDocumentPageCandidate(nextUrl) });

    QCOMPARE(runtimePlans.size(), std::size_t(1));
    QVERIFY(findOperation<kiriview::LoadUrlOperation>(runtimePlans.front()) == nullptr);
    QVERIFY(failures.empty());
}

QTEST_GUILESS_MAIN(TestImageDocumentDeletionController)

#include "test_imagedocumentdeletioncontroller.moc"

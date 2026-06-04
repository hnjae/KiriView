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
using KiriView::TestSupport::imageDocumentPageCandidate;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::testImage;

KiriView::ImageCacheBudgets testCacheBudgets()
{
    return KiriView::ImageCacheBudgets {
        1024 * 1024,
        512 * 1024,
    };
}

struct ManualImageDocumentPageCandidateLoad {
    QObject *object = nullptr;
    QUrl url;
    KiriView::ImageDocumentPageCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualDeletionCandidateProvider
{
public:
    KiriView::ImageDocumentPageCandidateProvider provider()
    {
        KiriView::ImageDocumentPageCandidateProvider provider;
        provider.directoryImageDocumentPages
            = [this](QObject *receiver, QUrl directoryUrl,
                  KiriView::ImageDocumentPageCandidatesCallback callback,
                  KiriView::ErrorCallback errorCallback) {
                  auto load = std::make_shared<ManualImageDocumentPageCandidateLoad>();
                  load->url = std::move(directoryUrl);
                  load->callback = std::move(callback);
                  load->errorCallback = std::move(errorCallback);

                  KiriView::ImageIoJob job
                      = KiriView::TestSupport::Detail::startManualIoJob(receiver, load);
                  m_imageLoads.push_back(load);
                  return job;
              };
        provider.directoryContainers
            = [](QObject *, QUrl, KiriView::ContainerCandidatesCallback, KiriView::ErrorCallback) {
                  return KiriView::ImageIoJob();
              };
        provider.openedCollectionCandidates
            = [](QObject *, KiriView::OpenedCollectionScopeLocation,
                  KiriView::ImageDocumentPageCandidatesCallback,
                  KiriView::ErrorCallback) { return KiriView::ImageIoJob(); };
        provider.directoryImageDocumentPageChanges
            = [](QObject *, QUrl, KiriView::ImageDocumentPageCandidatesCallback,
                  KiriView::ErrorCallback) { return KiriView::ImageIoJob(); };
        return provider;
    }

    std::size_t imageLoadCount() const { return m_imageLoads.size(); }

    ManualImageDocumentPageCandidateLoad &backImageLoad() { return *m_imageLoads.back(); }

    void deliverBackImageDocumentPageCandidatesIgnoringCancellation(
        std::vector<KiriView::ImageDocumentPageCandidate> candidates)
    {
        if (m_imageLoads.back()->callback) {
            m_imageLoads.back()->callback(std::move(candidates));
        }
    }

private:
    std::vector<std::shared_ptr<ManualImageDocumentPageCandidateLoad>> m_imageLoads;
};

template <typename Operation>
const Operation *findOperation(const KiriView::ImageDocumentRuntimePlan &plan)
{
    for (const KiriView::ImageDocumentRuntimeOperation &operation : plan) {
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
    void canceledFileDeletionCompletionIsIgnored();
    void canceledFallbackCandidateCompletionIsIgnored();
};

void TestImageDocumentDeletionController::canceledFileDeletionCompletionIsIgnored()
{
    QObject parent;
    KiriView::ImageDocumentState state;
    KiriView::ImagePageSurfaceController pageSurface(&parent, {}, testCacheBudgets());
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    ManualDeletionCandidateProvider candidateProvider;
    std::vector<KiriView::ImageDocumentRuntimePlan> runtimePlans;
    std::vector<QString> failures;
    int inProgressChangeCount = 0;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(imageUrl));
    pageSurface.setImage(testImage(2, 1), false);

    KiriView::ImageDocumentDeletionController controller(&parent, state, pageSurface,
        candidateProvider.provider(),
        KiriView::TestSupport::fileDeletionProviderFor(fileDeletionProvider),
        KiriView::ImageDocumentDeletionController::Callbacks {
            [&inProgressChangeCount]() { ++inProgressChangeCount; },
            [&runtimePlans](KiriView::ImageDocumentRuntimePlan plan) {
                runtimePlans.push_back(std::move(plan));
            },
            [&failures](const QString &errorString) { failures.push_back(errorString); },
        });

    controller.deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QVERIFY(controller.inProgress());

    controller.cancel();
    QVERIFY(!controller.inProgress());
    QVERIFY(fileDeletionProvider.backOperation().canceled);

    fileDeletionProvider.backOperation().callback(
        KiriView::FileDeletionResult::Succeeded, QString());

    QVERIFY(runtimePlans.empty());
    QVERIFY(failures.empty());
    QCOMPARE(inProgressChangeCount, 2);
}

void TestImageDocumentDeletionController::canceledFallbackCandidateCompletionIsIgnored()
{
    QObject parent;
    KiriView::ImageDocumentState state;
    KiriView::ImagePageSurfaceController pageSurface(&parent, {}, testCacheBudgets());
    KiriView::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    ManualDeletionCandidateProvider candidateProvider;
    std::vector<KiriView::ImageDocumentRuntimePlan> runtimePlans;
    std::vector<QString> failures;
    const QUrl currentUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/02.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(currentUrl));
    pageSurface.setImage(testImage(2, 1), false);

    KiriView::ImageDocumentDeletionController controller(&parent, state, pageSurface,
        candidateProvider.provider(),
        KiriView::TestSupport::fileDeletionProviderFor(fileDeletionProvider),
        KiriView::ImageDocumentDeletionController::Callbacks {
            {},
            [&runtimePlans](KiriView::ImageDocumentRuntimePlan plan) {
                runtimePlans.push_back(std::move(plan));
            },
            [&failures](const QString &errorString) { failures.push_back(errorString); },
        });

    controller.deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    fileDeletionProvider.finishBackOperation(KiriView::FileDeletionResult::Succeeded);
    QCOMPARE(candidateProvider.imageLoadCount(), std::size_t(1));
    QCOMPARE(runtimePlans.size(), std::size_t(1));
    QVERIFY(
        findOperation<KiriView::FinishEmptySourceLoadOperation>(runtimePlans.front()) != nullptr);

    controller.cancel();
    QVERIFY(candidateProvider.backImageLoad().canceled);

    candidateProvider.deliverBackImageDocumentPageCandidatesIgnoringCancellation(
        { imageDocumentPageCandidate(nextUrl) });

    QCOMPARE(runtimePlans.size(), std::size_t(1));
    QVERIFY(findOperation<KiriView::LoadUrlOperation>(runtimePlans.front()) == nullptr);
    QVERIFY(failures.empty());
}

QTEST_GUILESS_MAIN(TestImageDocumentDeletionController)

#include "test_imagedocumentdeletioncontroller.moc"

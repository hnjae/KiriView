// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentdeletionfallbackcontroller.h"

#include "candidate_test_support.h"
#include "image_async_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace {
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::comicBookContainerCandidate;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::localUrl;

struct ManualImageDocumentPageCandidateLoad {
    QObject *object = nullptr;
    QUrl url;
    kiriview::ImageDocumentPageCandidatesCallback callback;
    kiriview::ErrorCallback errorCallback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualDeletionFallbackCandidateProvider
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

bool planLoadsUrl(const kiriview::ImageDocumentRuntimePlan &plan, const QUrl &url)
{
    const auto *operation = findOperation<kiriview::LoadUrlOperation>(plan);
    return operation != nullptr && operation->target.url == url;
}

bool planLoadsContainerImage(
    const kiriview::ImageDocumentRuntimePlan &plan, const QUrl &imageUrl, const QUrl &containerUrl)
{
    const auto *operation = findOperation<kiriview::LoadContainerImageOperation>(plan);
    return operation != nullptr && operation->target.url == imageUrl
        && operation->containerUrl == containerUrl;
}
}

class TestImageDocumentDeletionFallbackController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageFallbackOpensNextSibling();
    void canceledImageFallbackCompletionIsIgnored();
    void comicBookFallbackTriesPreviousContainerWhenPreferredIsEmpty();
};

void TestImageDocumentDeletionFallbackController::imageFallbackOpensNextSibling()
{
    QObject parent;
    kiriview::TestSupport::FakeImageDocumentPageCandidateProvider provider;
    std::vector<kiriview::ImageDocumentRuntimePlan> runtimePlans;
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/03.png"));
    provider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(localUrl(QStringLiteral("/images/01.png"))),
            imageDocumentPageCandidate(nextUrl),
        });

    kiriview::ImageDocumentDeletionFallbackController controller(
        &parent, provider.provider(), [&runtimePlans](kiriview::ImageDocumentRuntimePlan plan) {
            runtimePlans.push_back(std::move(plan));
        });

    controller.open(kiriview::ImageRemovalFallback {
        kiriview::ImageDocumentPageCandidateListContext::forDirectory(
            currentUrl, localUrl(QStringLiteral("/images/"))),
        currentUrl,
        QStringLiteral("02.png"),
    });

    QCOMPARE(runtimePlans.size(), std::size_t(1));
    QVERIFY(planLoadsUrl(runtimePlans.front(), nextUrl));
}

void TestImageDocumentDeletionFallbackController::canceledImageFallbackCompletionIsIgnored()
{
    QObject parent;
    ManualDeletionFallbackCandidateProvider provider;
    std::vector<kiriview::ImageDocumentRuntimePlan> runtimePlans;
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/03.png"));

    kiriview::ImageDocumentDeletionFallbackController controller(
        &parent, provider.provider(), [&runtimePlans](kiriview::ImageDocumentRuntimePlan plan) {
            runtimePlans.push_back(std::move(plan));
        });

    controller.open(kiriview::ImageRemovalFallback {
        kiriview::ImageDocumentPageCandidateListContext::forDirectory(
            currentUrl, localUrl(QStringLiteral("/images/"))),
        currentUrl,
        QStringLiteral("02.png"),
    });
    QCOMPARE(provider.imageLoadCount(), std::size_t(1));

    controller.cancel();
    QVERIFY(provider.backImageLoad().canceled);

    provider.deliverBackImageDocumentPageCandidatesIgnoringCancellation(
        { imageDocumentPageCandidate(nextUrl) });

    QVERIFY(runtimePlans.empty());
}

void TestImageDocumentDeletionFallbackController::
    comicBookFallbackTriesPreviousContainerWhenPreferredIsEmpty()
{
    QObject parent;
    kiriview::TestSupport::FakeImageDocumentPageCandidateProvider provider;
    std::vector<kiriview::ImageDocumentRuntimePlan> runtimePlans;
    const QUrl previousContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const QUrl nextContainerUrl = localUrl(QStringLiteral("/books/c.cbz"));
    const QUrl candidateDirectoryUrl = localUrl(QStringLiteral("/books/"));
    provider.setContainerCandidates(candidateDirectoryUrl,
        {
            comicBookContainerCandidate(previousContainerUrl),
            comicBookContainerCandidate(nextContainerUrl),
        });

    const std::optional<kiriview::OpenedCollectionScopeLocation> previousArchive
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(previousContainerUrl);
    QVERIFY(previousArchive.has_value());
    const std::optional<kiriview::OpenedCollectionScopeLocation> nextArchive
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(nextContainerUrl);
    QVERIFY(nextArchive.has_value());
    const QUrl previousPageUrl
        = archivePageUrl(previousArchive->rootUrl(), QStringLiteral("page.png"));
    provider.setOpenedCollectionCandidates(nextArchive->rootUrl(), {});
    provider.setOpenedCollectionCandidates(
        previousArchive->rootUrl(), { imageDocumentPageCandidate(previousPageUrl) });

    kiriview::ImageDocumentDeletionFallbackController controller(
        &parent, provider.provider(), [&runtimePlans](kiriview::ImageDocumentRuntimePlan plan) {
            runtimePlans.push_back(std::move(plan));
        });

    controller.open(kiriview::ComicBookRemovalFallback {
        currentContainerUrl,
        candidateDirectoryUrl,
        QStringLiteral("b.cbz"),
    });

    QCOMPARE(runtimePlans.size(), std::size_t(1));
    QVERIFY(planLoadsContainerImage(runtimePlans.front(), previousPageUrl, previousContainerUrl));
}

QTEST_GUILESS_MAIN(TestImageDocumentDeletionFallbackController)

#include "test_imagedocumentdeletionfallbackcontroller.moc"

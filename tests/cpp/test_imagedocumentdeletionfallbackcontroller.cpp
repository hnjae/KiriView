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
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::comicBookContainerCandidate;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;

struct ManualImageCandidateLoad {
    QObject *object = nullptr;
    QUrl url;
    KiriView::ImageCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualDeletionFallbackCandidateProvider
{
public:
    KiriView::ImageNavigationCandidateProvider provider()
    {
        KiriView::ImageNavigationCandidateProvider provider;
        provider.directoryImages = [this](QObject *receiver, QUrl directoryUrl,
                                       KiriView::ImageCandidatesCallback callback,
                                       KiriView::ErrorCallback errorCallback) {
            auto load = std::make_shared<ManualImageCandidateLoad>();
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
        provider.archiveImages
            = [](QObject *, KiriView::ImagePageScopeLocation, KiriView::ImageCandidatesCallback,
                  KiriView::ErrorCallback) { return KiriView::ImageIoJob(); };
        provider.directoryImageChanges
            = [](QObject *, QUrl, KiriView::ImageCandidatesCallback, KiriView::ErrorCallback) {
                  return KiriView::ImageIoJob();
              };
        return provider;
    }

    std::size_t imageLoadCount() const { return m_imageLoads.size(); }

    ManualImageCandidateLoad &backImageLoad() { return *m_imageLoads.back(); }

    void deliverBackImageCandidatesIgnoringCancellation(
        std::vector<KiriView::ImageNavigationCandidate> candidates)
    {
        if (m_imageLoads.back()->callback) {
            m_imageLoads.back()->callback(std::move(candidates));
        }
    }

private:
    std::vector<std::shared_ptr<ManualImageCandidateLoad>> m_imageLoads;
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

bool planLoadsUrl(const KiriView::ImageDocumentRuntimePlan &plan, const QUrl &url)
{
    const auto *operation = findOperation<KiriView::LoadUrlOperation>(plan);
    return operation != nullptr && operation->url == url;
}

bool planLoadsContainerImage(
    const KiriView::ImageDocumentRuntimePlan &plan, const QUrl &imageUrl, const QUrl &containerUrl)
{
    const auto *operation = findOperation<KiriView::LoadContainerImageOperation>(plan);
    return operation != nullptr && operation->imageUrl == imageUrl
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
    KiriView::TestSupport::FakeImageNavigationCandidateProvider provider;
    std::vector<KiriView::ImageDocumentRuntimePlan> runtimePlans;
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/03.png"));
    provider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(localUrl(QStringLiteral("/images/01.png"))),
            imageCandidate(nextUrl),
        });

    KiriView::ImageDocumentDeletionFallbackController controller(
        &parent, provider.provider(), [&runtimePlans](KiriView::ImageDocumentRuntimePlan plan) {
            runtimePlans.push_back(std::move(plan));
        });

    controller.open(KiriView::ImageRemovalFallback {
        KiriView::ImageCandidateListContext::forDirectory(
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
    std::vector<KiriView::ImageDocumentRuntimePlan> runtimePlans;
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/03.png"));

    KiriView::ImageDocumentDeletionFallbackController controller(
        &parent, provider.provider(), [&runtimePlans](KiriView::ImageDocumentRuntimePlan plan) {
            runtimePlans.push_back(std::move(plan));
        });

    controller.open(KiriView::ImageRemovalFallback {
        KiriView::ImageCandidateListContext::forDirectory(
            currentUrl, localUrl(QStringLiteral("/images/"))),
        currentUrl,
        QStringLiteral("02.png"),
    });
    QCOMPARE(provider.imageLoadCount(), std::size_t(1));

    controller.cancel();
    QVERIFY(provider.backImageLoad().canceled);

    provider.deliverBackImageCandidatesIgnoringCancellation({ imageCandidate(nextUrl) });

    QVERIFY(runtimePlans.empty());
}

void TestImageDocumentDeletionFallbackController::
    comicBookFallbackTriesPreviousContainerWhenPreferredIsEmpty()
{
    QObject parent;
    KiriView::TestSupport::FakeImageNavigationCandidateProvider provider;
    std::vector<KiriView::ImageDocumentRuntimePlan> runtimePlans;
    const QUrl previousContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const QUrl nextContainerUrl = localUrl(QStringLiteral("/books/c.cbz"));
    const QUrl candidateDirectoryUrl = localUrl(QStringLiteral("/books/"));
    provider.setContainerCandidates(candidateDirectoryUrl,
        {
            comicBookContainerCandidate(previousContainerUrl),
            comicBookContainerCandidate(nextContainerUrl),
        });

    const std::optional<KiriView::ImagePageScopeLocation> previousArchive
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(previousContainerUrl);
    QVERIFY(previousArchive.has_value());
    const std::optional<KiriView::ImagePageScopeLocation> nextArchive
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(nextContainerUrl);
    QVERIFY(nextArchive.has_value());
    const QUrl previousPageUrl
        = archivePageUrl(previousArchive->rootUrl(), QStringLiteral("page.png"));
    provider.setArchiveImages(nextArchive->rootUrl(), {});
    provider.setArchiveImages(previousArchive->rootUrl(), { imageCandidate(previousPageUrl) });

    KiriView::ImageDocumentDeletionFallbackController controller(
        &parent, provider.provider(), [&runtimePlans](KiriView::ImageDocumentRuntimePlan plan) {
            runtimePlans.push_back(std::move(plan));
        });

    controller.open(KiriView::ComicBookRemovalFallback {
        currentContainerUrl,
        candidateDirectoryUrl,
        QStringLiteral("b.cbz"),
    });

    QCOMPARE(runtimePlans.size(), std::size_t(1));
    QVERIFY(planLoadsContainerImage(runtimePlans.front(), previousPageUrl, previousContainerUrl));
}

QTEST_GUILESS_MAIN(TestImageDocumentDeletionFallbackController)

#include "test_imagedocumentdeletionfallbackcontroller.moc"

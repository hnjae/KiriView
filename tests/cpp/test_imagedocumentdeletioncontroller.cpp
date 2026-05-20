// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentdeletioncontroller.h"

#include "document/imagedocumentstate.h"
#include "image_test_support.h"
#include "presentation/imagepresentationcontroller.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace {
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::testImage;

struct ManualImageCandidateLoad {
    QObject *object = nullptr;
    QUrl url;
    KiriView::ImageCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualDeletionCandidateProvider
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
            = [](QObject *, KiriView::ArchiveDocumentLocation, KiriView::ImageCandidatesCallback,
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

bool isClearDeletedImageEffect(const KiriView::ImageDocumentEffect &effect)
{
    return std::holds_alternative<KiriView::ClearDeletedImageEffect>(effect.payload);
}

bool isOpenUrlEffect(const KiriView::ImageDocumentEffect &effect, const QUrl &url)
{
    const auto *payload = std::get_if<KiriView::OpenUrlEffect>(&effect.payload);
    return payload != nullptr && payload->url == url;
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
    KiriView::ImagePresentationController presentation(
        &parent, []() { return KiriView::ImageDocumentRenderContext {}; }, {});
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    ManualDeletionCandidateProvider candidateProvider;
    std::vector<KiriView::ImageDocumentEffect> effects;
    std::vector<QString> failures;
    int inProgressChangeCount = 0;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(imageUrl));
    presentation.setImage(testImage(2, 1), false);

    KiriView::ImageDocumentDeletionController controller(&parent, state, presentation,
        candidateProvider.provider(),
        KiriView::TestSupport::fileOperationProviderFor(fileOperations),
        KiriView::ImageDocumentDeletionController::Callbacks {
            [&inProgressChangeCount]() { ++inProgressChangeCount; },
            [&effects](
                KiriView::ImageDocumentEffect effect) { effects.push_back(std::move(effect)); },
            [&failures](const QString &errorString) { failures.push_back(errorString); },
        });

    controller.deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QVERIFY(controller.inProgress());

    controller.cancel();
    QVERIFY(!controller.inProgress());
    QVERIFY(fileOperations.backOperation().canceled);

    fileOperations.backOperation().callback(KiriView::FileDeletionResult::Succeeded, QString());

    QVERIFY(effects.empty());
    QVERIFY(failures.empty());
    QCOMPARE(inProgressChangeCount, 2);
}

void TestImageDocumentDeletionController::canceledFallbackCandidateCompletionIsIgnored()
{
    QObject parent;
    KiriView::ImageDocumentState state;
    KiriView::ImagePresentationController presentation(
        &parent, []() { return KiriView::ImageDocumentRenderContext {}; }, {});
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    ManualDeletionCandidateProvider candidateProvider;
    std::vector<KiriView::ImageDocumentEffect> effects;
    std::vector<QString> failures;
    const QUrl currentUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/02.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(currentUrl));
    presentation.setImage(testImage(2, 1), false);

    KiriView::ImageDocumentDeletionController controller(&parent, state, presentation,
        candidateProvider.provider(),
        KiriView::TestSupport::fileOperationProviderFor(fileOperations),
        KiriView::ImageDocumentDeletionController::Callbacks {
            {},
            [&effects](
                KiriView::ImageDocumentEffect effect) { effects.push_back(std::move(effect)); },
            [&failures](const QString &errorString) { failures.push_back(errorString); },
        });

    controller.deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);
    QCOMPARE(candidateProvider.imageLoadCount(), std::size_t(1));
    QCOMPARE(effects.size(), std::size_t(1));
    QVERIFY(isClearDeletedImageEffect(effects.front()));

    controller.cancel();
    QVERIFY(candidateProvider.backImageLoad().canceled);

    candidateProvider.deliverBackImageCandidatesIgnoringCancellation({ imageCandidate(nextUrl) });

    QCOMPARE(effects.size(), std::size_t(1));
    QVERIFY(!isOpenUrlEffect(effects.front(), nextUrl));
    QVERIFY(failures.empty());
}

QTEST_GUILESS_MAIN(TestImageDocumentDeletionController)

#include "test_imagedocumentdeletioncontroller.moc"

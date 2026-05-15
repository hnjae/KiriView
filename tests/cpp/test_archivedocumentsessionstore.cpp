// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivedocumentsessionstore.h"

#include "image_test_support.h"
#include "imagecallback.h"
#include "imagecontainer.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagenavigationservice.h"
#include "imagepredecodecoordinator.h"

#include <QTest>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::ImageNavigationCandidate;
using KiriView::NavigationDirection;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::keyForUrl;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

struct ArchiveFixture {
    std::vector<ImageNavigationCandidate> candidates;
    std::map<QString, QByteArray> dataByUrl;
};

struct InstrumentedArchiveSessionState {
    std::atomic<int> openCount = 0;
    std::atomic<int> candidateLoadCount = 0;
    std::atomic<int> dataLoadCount = 0;
    std::mutex mutex;
    std::map<QString, ArchiveFixture> fixturesByRootUrl;
};

class InstrumentedArchiveSession final : public KiriView::ArchiveDocumentSession
{
public:
    InstrumentedArchiveSession(KiriView::ArchiveDocumentLocation archiveDocument,
        std::shared_ptr<InstrumentedArchiveSessionState> state)
        : m_archiveDocument(std::move(archiveDocument))
        , m_state(std::move(state))
    {
    }

    KiriView::ArchiveImageCandidatesResult loadImageCandidates() override
    {
        ++m_state->candidateLoadCount;
        std::lock_guard<std::mutex> lock(m_state->mutex);
        return KiriView::ArchiveImageCandidates {
            fixture().candidates,
        };
    }

    KiriView::ArchiveImageDataResult loadImageData(const QUrl &imageUrl) override
    {
        ++m_state->dataLoadCount;
        std::lock_guard<std::mutex> lock(m_state->mutex);
        const auto data = fixture().dataByUrl.find(keyForUrl(imageUrl));
        if (data == fixture().dataByUrl.cend()) {
            return KiriView::ArchiveError { QStringLiteral("missing fake archive image data") };
        }

        return KiriView::ArchiveImageData { data->second };
    }

private:
    const ArchiveFixture &fixture() const
    {
        return m_state->fixturesByRootUrl.at(keyForUrl(m_archiveDocument.rootUrl()));
    }

    KiriView::ArchiveDocumentLocation m_archiveDocument;
    std::shared_ptr<InstrumentedArchiveSessionState> m_state;
};

KiriView::ArchiveDocumentSessionFactory instrumentedSessionFactory(
    std::shared_ptr<InstrumentedArchiveSessionState> state)
{
    return [state = std::move(state)](const KiriView::ArchiveDocumentLocation &archiveDocument)
               -> KiriView::ArchiveDocumentSessionOpenResult {
        ++state->openCount;
        std::lock_guard<std::mutex> lock(state->mutex);
        if (!state->fixturesByRootUrl.count(keyForUrl(archiveDocument.rootUrl()))) {
            return KiriView::ArchiveError { QStringLiteral("missing fake archive fixture") };
        }

        return KiriView::ArchiveDocumentSessionPtr(
            std::make_shared<InstrumentedArchiveSession>(archiveDocument, state));
    };
}

std::optional<KiriView::ArchiveDocumentLocation> archiveDocumentFor(const QUrl &archiveUrl)
{
    return KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
}

void addArchiveFixture(std::shared_ptr<InstrumentedArchiveSessionState> state,
    const KiriView::ArchiveDocumentLocation &archiveDocument,
    std::vector<ImageNavigationCandidate> candidates)
{
    ArchiveFixture fixture;
    fixture.candidates = std::move(candidates);
    for (const ImageNavigationCandidate &candidate : fixture.candidates) {
        fixture.dataByUrl[keyForUrl(candidate.url)] = QByteArrayLiteral("image");
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    state->fixturesByRootUrl[keyForUrl(archiveDocument.rootUrl())] = std::move(fixture);
}

KiriView::ImageNavigationCandidateProvider archiveOnlyProvider()
{
    return KiriView::ImageNavigationCandidateProvider {
        [](QObject *, QUrl, KiriView::ImageCandidatesCallback,
            KiriView::ErrorCallback errorCallback) {
            KiriView::invokeIfSet(errorCallback, QStringLiteral("unexpected directory listing"));
            return KiriView::ImageIoJob();
        },
        [](QObject *, QUrl, KiriView::ContainerCandidatesCallback,
            KiriView::ErrorCallback errorCallback) {
            KiriView::invokeIfSet(errorCallback, QStringLiteral("unexpected container listing"));
            return KiriView::ImageIoJob();
        },
        {},
        {},
    };
}

KiriView::ImageNavigationService::Callbacks navigationCallbacks(
    KiriView::ImageNavigationService::OpenUrlCallback openUrl = {},
    KiriView::ImageNavigationService::PageNavigationChangedCallback pageNavigationChanged = {})
{
    return KiriView::ImageNavigationService::Callbacks {
        std::move(openUrl),
        {},
        {},
        std::move(pageNavigationChanged),
        {},
        {},
    };
}
}

class TestArchiveDocumentSessionStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void candidateAndDataLoadsShareOneArchiveOpen();
    void simultaneousCandidateLoadsSharePendingSessionLoad();
    void navigationReusesCachedArchiveCandidates();
    void predecodeLoadsAdjacentArchiveImagesThroughSession();
    void lifecycleClearsSessionForDifferentArchiveNormalImageAndClear();
};

void TestArchiveDocumentSessionStore::candidateAndDataLoadsShareOneArchiveOpen()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentFor(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    addArchiveFixture(
        state, *archiveDocument, { imageCandidate(firstUrl), imageCandidate(secondUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedSessionFactory(state));
    std::vector<ImageNavigationCandidate> candidates;
    QByteArray firstData;
    QByteArray secondData;
    store.loadArchiveImages(nullptr, *archiveDocument,
        [&candidates](
            std::vector<ImageNavigationCandidate> loaded) { candidates = std::move(loaded); },
        {});
    store.loadArchiveImages(nullptr, *archiveDocument, [](auto) { }, {});
    store.loadArchiveImageData(nullptr,
        KiriView::ImageDecodeRequest::fromLocation(
            1, KiriView::DisplayedImageLocation::fromArchiveDocument(firstUrl, *archiveDocument)),
        [&firstData](QByteArray data) { firstData = std::move(data); }, {});
    store.loadArchiveImageData(nullptr,
        KiriView::ImageDecodeRequest::fromLocation(
            2, KiriView::DisplayedImageLocation::fromArchiveDocument(secondUrl, *archiveDocument)),
        [&secondData](QByteArray data) { secondData = std::move(data); }, {});

    QCOMPARE(candidates.size(), std::size_t(2));
    QCOMPARE(firstData, QByteArrayLiteral("image"));
    QCOMPARE(secondData, QByteArrayLiteral("image"));
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 2);
}

void TestArchiveDocumentSessionStore::simultaneousCandidateLoadsSharePendingSessionLoad()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentFor(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    addArchiveFixture(state, *archiveDocument, { imageCandidate(pageUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedSessionFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = store.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    KiriView::ImageIoJob secondJob = store.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});

    QTRY_COMPARE(callbackCount, 2);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestArchiveDocumentSessionStore::navigationReusesCachedArchiveCandidates()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentFor(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    addArchiveFixture(state, *archiveDocument,
        { imageCandidate(firstUrl), imageCandidate(secondUrl), imageCandidate(thirdUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedSessionFactory(state));
    QUrl openedUrl;
    KiriView::ImageNavigationService service(this,
        store.wrapCandidateProvider(archiveOnlyProvider()),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }));

    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromArchiveDocument(firstUrl, *archiveDocument),
    });
    QTRY_COMPARE(service.imageCount(), 3);
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    service.openAdjacentImage(
        KiriView::ImageNavigationService::DisplayContext {
            true,
            KiriView::DisplayedImageLocation::fromArchiveDocument(firstUrl, *archiveDocument),
        },
        NavigationDirection::Next);
    QCOMPARE(openedUrl, secondUrl);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromArchiveDocument(secondUrl, *archiveDocument),
    });
    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.imageCount(), 3);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestArchiveDocumentSessionStore::predecodeLoadsAdjacentArchiveImagesThroughSession()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentFor(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl displayedUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    addArchiveFixture(state, *archiveDocument,
        { imageCandidate(firstUrl), imageCandidate(displayedUrl), imageCandidate(thirdUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedSessionFactory(state));
    KiriView::ImagePredecodeCoordinator coordinator(this,
        store.wrapCandidateProvider(archiveOnlyProvider()),
        store.wrapDecodeDependencies(KiriView::ImageDecodeDependencies {
            {},
            staticImageDataDecoder(),
        }));

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromArchiveDocument(displayedUrl, *archiveDocument),
            true,
            staticTestImagePayload(testImage()),
        },
    });

    QTRY_VERIFY(coordinator.tryTake(thirdUrl).has_value());
    QTRY_VERIFY(coordinator.tryTake(firstUrl).has_value());
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 2);
}

void TestArchiveDocumentSessionStore::lifecycleClearsSessionForDifferentArchiveNormalImageAndClear()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl firstArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl secondArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> firstArchiveDocument
        = archiveDocumentFor(firstArchiveUrl);
    const std::optional<KiriView::ArchiveDocumentLocation> secondArchiveDocument
        = archiveDocumentFor(secondArchiveUrl);
    QVERIFY(firstArchiveDocument.has_value());
    QVERIFY(secondArchiveDocument.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    addArchiveFixture(state, *firstArchiveDocument, { imageCandidate(firstPageUrl) });
    addArchiveFixture(state, *secondArchiveDocument, { imageCandidate(secondPageUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedSessionFactory(state));
    store.loadArchiveImages(nullptr, *firstArchiveDocument, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 1);
    QVERIFY(store.hasCurrentArchiveDocument(*firstArchiveDocument));

    store.prepareForSourceLoad(
        KiriView::ImageDocumentSourceLoadRequest::fromUrl(secondArchiveUrl), *firstArchiveDocument);
    QVERIFY(store.hasCurrentArchiveDocument(*secondArchiveDocument));
    store.loadArchiveImages(nullptr, *secondArchiveDocument, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 2);

    store.prepareForSourceLoad(KiriView::ImageDocumentSourceLoadRequest::fromUrl(
                                   localUrl(QStringLiteral("/images/01.png"))),
        *secondArchiveDocument);
    QVERIFY(!store.hasCurrentArchiveDocument());

    store.loadArchiveImages(nullptr, *secondArchiveDocument, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 3);
    store.clear();
    QVERIFY(!store.hasCurrentArchiveDocument());
}

QTEST_GUILESS_MAIN(TestArchiveDocumentSessionStore)

#include "test_archivedocumentsessionstore.moc"

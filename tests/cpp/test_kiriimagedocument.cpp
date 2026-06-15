// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriimagedocument.h"

#include "facade/kiridocumentsession.h"
#include "facade/kiriimagedisplaysource.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QByteArray>
#include <QFile>
#include <QMetaProperty>
#include <QObject>
#include <QSignalSpy>
#include <QSizeF>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>
#include <memory>
#include <optional>

class TestKiriImageDocument : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceUrlPropertyIsReadOnlyObservation();
    void displaySourceFacadeObjectsAreStableReadOnlyObservations();
    void animatedFixturePlaybackUpdatesPrimaryDisplaySource_data();
    void animatedFixturePlaybackUpdatesPrimaryDisplaySource();
    void openedCollectionScopeActiveFollowsDisplayedLocation();
    void manualZoomAtCenterKeepsCenterAnchored();
    void viewportPanAndScanCommandsUpdateContentPosition();
    void nextDisplayedImageCanStartAtFinalScanPosition();
};

namespace {
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::imageDocumentRuntimeDependencyOverridesFor;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::staticImageDataDecoder;
using kiriview::TestSupport::testImage;

using FakeCandidateProvider = kiriview::TestSupport::FakeImageDocumentPageCandidateProvider;

std::unique_ptr<KiriDocumentSession> createSession(
    QObject *parent, FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    kiriview::KiriDocumentSessionDependencies dependencies;
    dependencies.imageDocument = imageDocumentRuntimeDependencyOverridesFor(
        candidateProvider, dataLoader, staticImageDataDecoder());
    auto session = std::make_unique<KiriDocumentSession>(std::move(dependencies), parent);
    session->imageDocument()->setViewportSize(QSizeF(400.0, 300.0));
    return session;
}

std::unique_ptr<KiriDocumentSession> createSession(QObject *parent,
    FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader,
    const QImage &decodedImage)
{
    kiriview::KiriDocumentSessionDependencies dependencies;
    dependencies.imageDocument = imageDocumentRuntimeDependencyOverridesFor(
        candidateProvider, dataLoader, staticImageDataDecoder(decodedImage));
    auto session = std::make_unique<KiriDocumentSession>(std::move(dependencies), parent);
    session->imageDocument()->setViewportSize(QSizeF(400.0, 300.0));
    return session;
}

std::unique_ptr<KiriDocumentSession> createRealDecodeSession(QObject *parent)
{
    auto session = std::make_unique<KiriDocumentSession>(parent);
    session->imageDocument()->setViewportSize(QSizeF(400.0, 300.0));
    return session;
}

QString fixturePath(const QString &fileName)
{
    return QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/") + fileName;
}

void loadReady(KiriDocumentSession &session, ManualImageDataLoader &dataLoader,
    const QUrl &sourceUrl, const QUrl &loadUrl)
{
    KiriImageDocument &document = *session.imageDocument();
    QSignalSpy scopeSpy(&document, &KiriImageDocument::imageDocumentSourceScopeChanged);

    session.setSourceUrl(sourceUrl);
    QVERIFY(dataLoader.finishOldestActiveLoadForUrl(loadUrl, QByteArrayLiteral("ok")));

    QTRY_COMPARE(document.status(), KiriImageDocument::Status::Ready);
    QVERIFY(scopeSpy.count() > 0);
}

bool approximatelyEqual(qreal left, qreal right) { return std::abs(left - right) < 0.001; }

void comparePoint(const QPointF &actual, const QPointF &expected)
{
    QVERIFY2(approximatelyEqual(actual.x(), expected.x()),
        qPrintable(QStringLiteral("x=%1 expected=%2").arg(actual.x()).arg(expected.x())));
    QVERIFY2(approximatelyEqual(actual.y(), expected.y()),
        qPrintable(QStringLiteral("y=%1 expected=%2").arg(actual.y()).arg(expected.y())));
}

std::unique_ptr<KiriDocumentSession> createReadyLargeImageSession(QObject *parent,
    FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader,
    const QUrl &imageUrl)
{
    candidateProvider.setDirectoryImages(
        localUrl(QStringLiteral("/images/")), { imageDocumentPageCandidate(imageUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(parent, candidateProvider, dataLoader, testImage(800, 800));
    loadReady(*session, dataLoader, imageUrl, imageUrl);
    return session;
}
}

void TestKiriImageDocument::sourceUrlPropertyIsReadOnlyObservation()
{
    const QMetaObject &metaObject = KiriImageDocument::staticMetaObject;
    const int sourceUrlIndex = metaObject.indexOfProperty("sourceUrl");
    QVERIFY(sourceUrlIndex >= 0);

    const QMetaProperty sourceUrlProperty = metaObject.property(sourceUrlIndex);
    QVERIFY(sourceUrlProperty.hasNotifySignal());
    QVERIFY(!sourceUrlProperty.isWritable());
}

void TestKiriImageDocument::displaySourceFacadeObjectsAreStableReadOnlyObservations()
{
    const QMetaObject &metaObject = KiriImageDocument::staticMetaObject;
    const int primaryIndex = metaObject.indexOfProperty("primaryDisplaySource");
    const int secondaryIndex = metaObject.indexOfProperty("secondaryDisplaySource");
    QVERIFY(primaryIndex >= 0);
    QVERIFY(secondaryIndex >= 0);

    const QMetaProperty primaryProperty = metaObject.property(primaryIndex);
    QVERIFY(primaryProperty.isConstant());
    QVERIFY(!primaryProperty.isWritable());

    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(
        localUrl(QStringLiteral("/images/")), { imageDocumentPageCandidate(imageUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(this, candidateProvider, dataLoader);

    KiriImageDocument &document = *session->imageDocument();
    KiriImageDisplaySource *primary = document.primaryDisplaySource();
    KiriImageDisplaySource *secondary = document.secondaryDisplaySource();
    QVERIFY(primary != nullptr);
    QVERIFY(secondary != nullptr);
    QCOMPARE(primary, document.primaryDisplaySource());
    QCOMPARE(secondary, document.secondaryDisplaySource());
    QCOMPARE(primary->status(), KiriImageDisplaySource::Status::Missing);
    QVERIFY(!primary->visible());

    QSignalSpy displaySourceSpy(&document, &KiriImageDocument::displaySourceChanged);
    QSignalSpy primarySpy(primary, &KiriImageDisplaySource::changed);

    loadReady(*session, dataLoader, imageUrl, imageUrl);

    QVERIFY(displaySourceSpy.count() > 0);
    QVERIFY(primarySpy.count() > 0);
    QVERIFY(primary->visible());
    QCOMPARE(primary->status(), KiriImageDisplaySource::Status::Ready);
    QVERIFY(!primary->providerUrl().isEmpty());
    QCOMPARE(primary->revisionToken(), QStringLiteral("1"));
    QCOMPARE(primary->sourceIdentity(), QStringLiteral("test-image"));
    QCOMPARE(primary->originalSize(), QSize(1, 1));
    QCOMPARE(primary->rasterSize(), QSize(1, 1));
    QCOMPARE(primary->pageRole(), KiriImageDisplaySource::PageRole::Primary);
    QCOMPARE(secondary->pageRole(), KiriImageDisplaySource::PageRole::Secondary);
}

void TestKiriImageDocument::animatedFixturePlaybackUpdatesPrimaryDisplaySource_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::newRow("apng") << QStringLiteral("animated-smoke.apng");
    QTest::newRow("jxl") << QStringLiteral("animated-smoke.jxl");
}

void TestKiriImageDocument::animatedFixturePlaybackUpdatesPrimaryDisplaySource()
{
    QFETCH(QString, fileName);

    const QString path = fixturePath(fileName);
    QVERIFY2(QFile::exists(path), qPrintable(path));

    std::unique_ptr<KiriDocumentSession> session = createRealDecodeSession(this);
    KiriImageDocument &document = *session->imageDocument();
    KiriImageDisplaySource *primary = document.primaryDisplaySource();
    QVERIFY(primary != nullptr);
    QSignalSpy primarySpy(primary, &KiriImageDisplaySource::changed);

    session->setSourceUrl(QUrl::fromLocalFile(path));

    QTRY_COMPARE(document.status(), KiriImageDocument::Status::Ready);
    QVERIFY(!primary->providerUrl().isEmpty());
    const QUrl firstProviderUrl = primary->providerUrl();
    const QString firstRevisionToken = primary->revisionToken();

    QTRY_VERIFY(primary->providerUrl() != firstProviderUrl
        || primary->revisionToken() != firstRevisionToken);
    QVERIFY(primarySpy.count() > 0);
}

void TestKiriImageDocument::openedCollectionScopeActiveFollowsDisplayedLocation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;

    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(
        localUrl(QStringLiteral("/images/")), { imageDocumentPageCandidate(imageUrl) });
    std::unique_ptr<KiriDocumentSession> directImageSession
        = createSession(this, candidateProvider, dataLoader);

    QVERIFY(!directImageSession->imageDocument()->openedCollectionScopeActive());
    loadReady(*directImageSession, dataLoader, imageUrl, imageUrl);
    QVERIFY(!directImageSession->imageDocument()->openedCollectionScopeActive());

    const QUrl comicArchiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> comicArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(comicArchiveUrl);
    QVERIFY(comicArchiveCollection.has_value());
    const QUrl comicArchivePage
        = archivePageUrl(comicArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(
        comicArchiveCollection->rootUrl(), { imageDocumentPageCandidate(comicArchivePage) });
    std::unique_ptr<KiriDocumentSession> comicSession
        = createSession(this, candidateProvider, dataLoader);

    loadReady(*comicSession, dataLoader, comicArchiveUrl, comicArchivePage);
    QVERIFY(comicSession->imageDocument()->openedCollectionScopeActive());

    const QUrl generalArchiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> generalArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(generalArchiveUrl);
    QVERIFY(generalArchiveCollection.has_value());
    const QUrl generalArchivePage
        = archivePageUrl(generalArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(
        generalArchiveCollection->rootUrl(), { imageDocumentPageCandidate(generalArchivePage) });
    std::unique_ptr<KiriDocumentSession> generalSession
        = createSession(this, candidateProvider, dataLoader);

    loadReady(*generalSession, dataLoader, generalArchiveUrl, generalArchivePage);
    QVERIFY(generalSession->imageDocument()->openedCollectionScopeActive());

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(directoryUrl);
    QVERIFY(directoryCollection.has_value());
    const QUrl directoryPage
        = archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(
        directoryCollection->rootUrl(), { imageDocumentPageCandidate(directoryPage) });
    std::unique_ptr<KiriDocumentSession> openedDirectoryCollection
        = createSession(this, candidateProvider, dataLoader);

    loadReady(*openedDirectoryCollection, dataLoader, directoryUrl, directoryPage);
    QVERIFY(openedDirectoryCollection->imageDocument()->openedCollectionScopeActive());

    const QUrl openedCollectionEntryUrl(QStringLiteral("zip:///books/book.zip!/page.png"));
    candidateProvider.setDirectoryImages(QUrl(QStringLiteral("zip:///books/book.zip!/")),
        { imageDocumentPageCandidate(openedCollectionEntryUrl) });
    std::unique_ptr<KiriDocumentSession> archiveEntrySession
        = createSession(this, candidateProvider, dataLoader);

    loadReady(*archiveEntrySession, dataLoader, openedCollectionEntryUrl, openedCollectionEntryUrl);
    QVERIFY(!archiveEntrySession->imageDocument()->openedCollectionScopeActive());
}

void TestKiriImageDocument::manualZoomAtCenterKeepsCenterAnchored()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    std::unique_ptr<KiriDocumentSession> session
        = createReadyLargeImageSession(this, candidateProvider, dataLoader, imageUrl);
    KiriImageDocument &document = *session->imageDocument();

    QCOMPARE(document.displaySize(), QSizeF(300.0, 300.0));
    QCOMPARE(document.viewportContentPosition(), QPointF());

    QVERIFY(document.requestManualZoomPercent(100.0));

    QCOMPARE(document.zoomMode(), KiriImageDocument::ZoomMode::Manual);
    QVERIFY(approximatelyEqual(document.zoomPercent(), 100.0));
    QCOMPARE(document.displaySize(), QSizeF(800.0, 800.0));
    comparePoint(document.viewportContentPosition(), QPointF(200.0, 250.0));
}

void TestKiriImageDocument::viewportPanAndScanCommandsUpdateContentPosition()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    std::unique_ptr<KiriDocumentSession> session
        = createReadyLargeImageSession(this, candidateProvider, dataLoader, imageUrl);
    KiriImageDocument &document = *session->imageDocument();

    QVERIFY(document.requestActualSizeAtCenter());
    QVERIFY(document.viewportPannable());
    QVERIFY(document.viewportHorizontallyPannable());
    QVERIFY(document.viewportVerticallyPannable());

    QVERIFY(document.requestViewportPanToInitialScanPosition());
    QCOMPARE(document.viewportContentPosition(), QPointF());

    QVERIFY(document.requestViewportPanBy(125.0, 75.0));
    comparePoint(document.viewportContentPosition(), QPointF(125.0, 75.0));

    QVERIFY(document.requestViewportPanToFinalScanPosition());
    comparePoint(document.viewportContentPosition(), QPointF(400.0, 500.0));
}

void TestKiriImageDocument::nextDisplayedImageCanStartAtFinalScanPosition()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { imageDocumentPageCandidate(firstImageUrl), imageDocumentPageCandidate(secondImageUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(this, candidateProvider, dataLoader, testImage(800, 800));
    KiriImageDocument &document = *session->imageDocument();

    loadReady(*session, dataLoader, archiveUrl, firstImageUrl);
    QVERIFY(document.requestActualSizeAtCenter());
    QVERIFY(document.requestViewportPanToInitialScanPosition());
    document.requestNextDisplayedImageStartToFinalScanPosition();

    document.openNextSinglePage();
    QVERIFY(dataLoader.finishOldestActiveLoadForUrl(secondImageUrl, QByteArrayLiteral("ok")));
    QTRY_COMPARE(document.status(), KiriImageDocument::Status::Ready);
    QCOMPARE(document.displayedUrl(), secondImageUrl);

    QVERIFY(document.requestDisplayedImageInitialContentPosition());

    QCOMPARE(document.zoomMode(), KiriImageDocument::ZoomMode::Manual);
    QVERIFY(approximatelyEqual(document.zoomPercent(), 100.0));
    comparePoint(document.viewportContentPosition(), QPointF(400.0, 500.0));
}

QTEST_GUILESS_MAIN(TestKiriImageDocument)

#include "test_kiriimagedocument.moc"

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionruntime.h"

#include <QBuffer>
#include <QByteArray>
#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <memory>

class TestDocumentSessionRuntimeLeafSnapshots : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directImageRoutePublishesImageLeafSnapshot();
    void imageSnapshotChangeRefreshesPublicProjection();
    void openedCollectionVideoLeafRoutesToSourceDeviceWithoutDirectMediaNavigation();
};

namespace {
QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }

class SnapshotChangeEmitter : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void imageSnapshotChanged();
};

kiriview::DocumentSessionDocumentSignalConnector imageSnapshotChangedConnector(
    SnapshotChangeEmitter& emitter)
{
    return [&emitter](QObject* context, kiriview::DocumentSessionDocumentChangeHandler handler) {
        std::vector<QMetaObject::Connection> connections;
        connections.push_back(
            QObject::connect(&emitter, &SnapshotChangeEmitter::imageSnapshotChanged, context,
                [handler = std::move(handler)]() {
                    if (handler) {
                        handler();
                    }
                }));
        return connections;
    };
}
}

void TestDocumentSessionRuntimeLeafSnapshots::directImageRoutePublishesImageLeafSnapshot()
{
    QObject owner;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/image.png"));
    kiriview::DocumentSessionImageDocumentSnapshot imageSnapshot;
    kiriview::DocumentSessionVideoDocumentSnapshot videoSnapshot;

    kiriview::DocumentSessionImageDocumentSnapshotPort imagePort;
    imagePort.snapshot = [&imageSnapshot]() { return imageSnapshot; };
    kiriview::DocumentSessionImageDocumentCommandPort imageCommands;
    imageCommands.source.setSourceUrl = [&imageSnapshot](const QUrl& url) {
        imageSnapshot.sourceUrl = url;
        imageSnapshot.displayedUrl = url;
        imageSnapshot.windowTitleFileName = url.fileName();
        imageSnapshot.primaryImageSize = QSize(320, 200);
        imageSnapshot.ready = !url.isEmpty();
        imageSnapshot.ordinaryDirectMediaScopeActive = !url.isEmpty();
        imageSnapshot.zoomPercentKnown = true;
        imageSnapshot.zoomPercent = 100.0;
    };
    imageCommands.pageNavigation.openPreviousPage = []() { };
    imageCommands.pageNavigation.openNextPage = []() { };
    imageCommands.pageNavigation.openImageAtPage = [](int) { };
    imageCommands.deletion.deleteDisplayedFile = [](kiriview::FileDeletionMode) { };

    kiriview::DocumentSessionVideoDocumentSnapshotPort videoPort;
    videoPort.snapshot = [&videoSnapshot]() { return videoSnapshot; };
    kiriview::DocumentSessionVideoDocumentCommandPort videoCommands;
    videoCommands.source.setSourceUrl
        = [&videoSnapshot](const QUrl& url) { videoSnapshot.sourceUrl = url; };
    videoCommands.output.videoOutput = []() -> QObject* { return nullptr; };
    videoCommands.playback.stop = []() { };
    videoCommands.output.setVideoOutput = [](QObject*) { };
    videoCommands.output.setVideoOutputGeometry = [](const QRectF&, const QRectF&) { };

    kiriview::DocumentSessionRuntime runtime(&owner, std::move(imagePort), std::move(imageCommands),
        std::move(videoPort), std::move(videoCommands));

    runtime.setSourceUrl(imageUrl);

    QCOMPARE(runtime.documentKind(), kiriview::DocumentSessionKind::Image);
    QCOMPARE(runtime.sourceUrl(), imageUrl);
    QVERIFY(runtime.activeImageReady());
    QVERIFY(runtime.windowTitleSubject().startsWith(QStringLiteral("image.png")));
    QVERIFY(runtime.windowTitleSubject().contains(QStringLiteral("320")));
    QVERIFY(runtime.windowTitleSubject().contains(QStringLiteral("200")));
    QVERIFY(runtime.activeZoomPercentKnown());
    QCOMPARE(runtime.activeZoomPercent(), 100.0);
}

void TestDocumentSessionRuntimeLeafSnapshots::imageSnapshotChangeRefreshesPublicProjection()
{
    QObject owner;
    SnapshotChangeEmitter emitter;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/image.png"));
    kiriview::DocumentSessionImageDocumentSnapshot imageSnapshot;
    kiriview::DocumentSessionVideoDocumentSnapshot videoSnapshot;

    kiriview::DocumentSessionImageDocumentSnapshotPort imagePort;
    imagePort.snapshot = [&imageSnapshot]() { return imageSnapshot; };
    imagePort.snapshotChanged = imageSnapshotChangedConnector(emitter);
    kiriview::DocumentSessionImageDocumentCommandPort imageCommands;
    imageCommands.source.setSourceUrl = [&imageSnapshot](const QUrl& url) {
        imageSnapshot.sourceUrl = url;
        imageSnapshot.displayedUrl = url;
        imageSnapshot.windowTitleFileName = url.fileName();
        imageSnapshot.primaryImageSize = QSize(320, 200);
        imageSnapshot.ready = !url.isEmpty();
        imageSnapshot.ordinaryDirectMediaScopeActive = !url.isEmpty();
    };
    imageCommands.pageNavigation.openPreviousPage = []() { };
    imageCommands.pageNavigation.openNextPage = []() { };
    imageCommands.pageNavigation.openImageAtPage = [](int) { };
    imageCommands.deletion.deleteDisplayedFile = [](kiriview::FileDeletionMode) { };

    kiriview::DocumentSessionVideoDocumentSnapshotPort videoPort;
    videoPort.snapshot = [&videoSnapshot]() { return videoSnapshot; };
    kiriview::DocumentSessionVideoDocumentCommandPort videoCommands;
    videoCommands.source.setSourceUrl
        = [&videoSnapshot](const QUrl& url) { videoSnapshot.sourceUrl = url; };
    videoCommands.output.videoOutput = []() -> QObject* { return nullptr; };
    videoCommands.playback.stop = []() { };
    videoCommands.output.setVideoOutput = [](QObject*) { };
    videoCommands.output.setVideoOutputGeometry = [](const QRectF&, const QRectF&) { };

    kiriview::DocumentSessionRuntime runtime(&owner, std::move(imagePort), std::move(imageCommands),
        std::move(videoPort), std::move(videoCommands));
    runtime.setSourceUrl(imageUrl);

    imageSnapshot.primaryImageSize = QSize(640, 480);
    Q_EMIT emitter.imageSnapshotChanged();

    QVERIFY(runtime.windowTitleSubject().contains(QStringLiteral("640")));
    QVERIFY(runtime.windowTitleSubject().contains(QStringLiteral("480")));
}

void TestDocumentSessionRuntimeLeafSnapshots::
    openedCollectionVideoLeafRoutesToSourceDeviceWithoutDirectMediaNavigation()
{
    QObject owner;
    SnapshotChangeEmitter emitter;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const QUrl archiveRootUrl(QStringLiteral("zip:///books/book.zip!/"));
    const QUrl firstImageUrl(QStringLiteral("zip:///books/book.zip!/01.png"));
    const QUrl videoUrl(QStringLiteral("zip:///books/book.zip!/02.mp4"));
    const kiriview::OpenedCollectionScopeLocation archiveCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            archiveUrl, archiveRootUrl, kiriview::OpenedCollectionScopeKind::GeneralArchive);
    kiriview::DocumentSessionImageDocumentSnapshot imageSnapshot;
    imageSnapshot.snapshotSourceKind = kiriview::ImageDocumentPageKind::Video;
    imageSnapshot.sourceUrl = videoUrl;
    imageSnapshot.displayedUrl = videoUrl;
    imageSnapshot.displayedOpenedCollectionScope = archiveCollection;
    imageSnapshot.windowTitleFileName = QStringLiteral("02.mp4");
    imageSnapshot.ready = true;
    imageSnapshot.openedCollectionScopeActive = true;
    imageSnapshot.pageNavigationSnapshot.state = kiriview::PageNavigationState(
        {
            kiriview::ImageDocumentPageTarget {
                firstImageUrl,
                kiriview::ImageDocumentPageKind::Image,
            },
            kiriview::ImageDocumentPageTarget {
                videoUrl,
                kiriview::ImageDocumentPageKind::Video,
            },
        },
        1);
    imageSnapshot.activeNavigationSnapshot = kiriview::ImageDocumentPageActiveNavigationSnapshot {
        true,
        true,
        false,
        false,
        true,
        2,
        2,
    };
    kiriview::DocumentSessionVideoDocumentSnapshot videoSnapshot;

    int playbackDeviceLoadCount = 0;
    QUrl loadedPlaybackScopeRootUrl;
    QUrl loadedPlaybackVideoUrl;
    kiriview::DocumentSessionImageDocumentSnapshotPort imagePort;
    imagePort.snapshot = [&imageSnapshot]() { return imageSnapshot; };
    imagePort.snapshotChanged = imageSnapshotChangedConnector(emitter);
    kiriview::DocumentSessionImageDocumentCommandPort imageCommands;
    imageCommands.source.setSourceUrl = [&imageSnapshot](const QUrl& url) {
        imageSnapshot.sourceUrl = url;
        imageSnapshot.displayedUrl = url;
    };
    imageCommands.source.loadOpenedCollectionVideoPlaybackDevice =
        [&playbackDeviceLoadCount, &loadedPlaybackScopeRootUrl, &loadedPlaybackVideoUrl](
            const kiriview::OpenedCollectionScopeLocation& openedCollectionScope, const QUrl& url) {
            ++playbackDeviceLoadCount;
            loadedPlaybackScopeRootUrl = openedCollectionScope.rootUrl();
            loadedPlaybackVideoUrl = url;
            auto device = std::make_unique<QBuffer>();
            device->setData(QByteArrayLiteral("collection-video-bytes"));
            device->open(QIODevice::ReadOnly);
            return kiriview::MediaEntrySourceVideoPlaybackDevice {
                {},
                std::move(device),
            };
        };
    imageCommands.pageNavigation.openPreviousPage = []() { };
    imageCommands.pageNavigation.openNextPage = []() { };
    imageCommands.pageNavigation.openImageAtPage = [](int) { };
    imageCommands.deletion.deleteDisplayedFile = [](kiriview::FileDeletionMode) { };

    int setSourceUrlCount = 0;
    int setSourceDeviceCount = 0;
    QUrl videoSourceUrl;
    QByteArray videoDeviceBytes;
    kiriview::DocumentSessionVideoDocumentSnapshotPort videoPort;
    videoPort.snapshot = [&videoSnapshot]() { return videoSnapshot; };
    kiriview::DocumentSessionVideoDocumentCommandPort videoCommands;
    videoCommands.source.setSourceUrl = [&videoSnapshot, &setSourceUrlCount](const QUrl& url) {
        ++setSourceUrlCount;
        videoSnapshot.sourceUrl = url;
    };
    videoCommands.source.setSourceDevice
        = [&videoSnapshot, &setSourceDeviceCount, &videoSourceUrl, &videoDeviceBytes](
              const QUrl& url, kiriview::VideoPlaybackSourceDevice sourceDevice) {
              ++setSourceDeviceCount;
              videoSourceUrl = url;
              videoSnapshot.sourceUrl = url;
              videoSnapshot.windowTitleFileName = url.fileName();
              videoSnapshot.ready = true;
              videoSnapshot.hasVideo = true;
              if (sourceDevice.device != nullptr) {
                  videoDeviceBytes = sourceDevice.device->readAll();
              }
          };
    videoCommands.output.videoOutput = []() -> QObject* { return nullptr; };
    videoCommands.playback.stop = []() { };
    videoCommands.output.setVideoOutput = [](QObject*) { };
    videoCommands.output.setVideoOutputGeometry = [](const QRectF&, const QRectF&) { };

    kiriview::DocumentSessionRuntime runtime(&owner, std::move(imagePort), std::move(imageCommands),
        std::move(videoPort), std::move(videoCommands));

    Q_EMIT emitter.imageSnapshotChanged();

    QCOMPARE(playbackDeviceLoadCount, 1);
    QCOMPARE(loadedPlaybackScopeRootUrl, archiveCollection.rootUrl());
    QCOMPARE(loadedPlaybackVideoUrl, videoUrl);
    QCOMPARE(setSourceUrlCount, 0);
    QCOMPARE(setSourceDeviceCount, 1);
    QCOMPARE(videoSourceUrl, videoUrl);
    QCOMPARE(videoDeviceBytes, QByteArrayLiteral("collection-video-bytes"));
    QCOMPARE(runtime.documentKind(), kiriview::DocumentSessionKind::Video);
    QCOMPARE(runtime.sourceUrl(), videoUrl);
    QVERIFY(runtime.activeVideoReady());
    QCOMPARE(runtime.activeNavigationBoundaryScope(),
        kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage);
    QVERIFY(!runtime.directMediaNavigationBoundaryActive());
    QCOMPARE(runtime.activeNavigationCurrentNumber(), 2);
    QCOMPARE(runtime.activeNavigationCount(), 2);
}

QTEST_GUILESS_MAIN(TestDocumentSessionRuntimeLeafSnapshots)

#include "test_documentsessionruntimeleafsnapshots.moc"

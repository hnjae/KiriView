// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionruntime.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>

class TestDocumentSessionRuntimeLeafSnapshots : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directImageRoutePublishesImageLeafSnapshot();
    void imageSnapshotChangeRefreshesPublicProjection();
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

QTEST_GUILESS_MAIN(TestDocumentSessionRuntimeLeafSnapshots)

#include "test_documentsessionruntimeleafsnapshots.moc"

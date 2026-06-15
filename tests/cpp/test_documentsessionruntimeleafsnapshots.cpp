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
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }
}

void TestDocumentSessionRuntimeLeafSnapshots::directImageRoutePublishesImageLeafSnapshot()
{
    QObject owner;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/image.png"));
    kiriview::DocumentSessionImageDocumentSnapshot imageSnapshot;
    kiriview::DocumentSessionVideoDocumentSnapshot videoSnapshot;

    kiriview::DocumentSessionImageDocumentPort imagePort;
    imagePort.snapshot = [&imageSnapshot]() { return imageSnapshot; };
    imagePort.setSourceUrl = [&imageSnapshot](const QUrl &url) {
        imageSnapshot.sourceUrl = url;
        imageSnapshot.displayedUrl = url;
        imageSnapshot.windowTitleFileName = url.fileName();
        imageSnapshot.primaryImageSize = QSize(320, 200);
        imageSnapshot.ready = !url.isEmpty();
        imageSnapshot.ordinaryDirectMediaScopeActive = !url.isEmpty();
        imageSnapshot.zoomPercentKnown = true;
        imageSnapshot.zoomPercent = 100.0;
    };
    imagePort.openPreviousPage = []() { };
    imagePort.openNextPage = []() { };
    imagePort.openImageAtPage = [](int) { };
    imagePort.deleteDisplayedFile = [](kiriview::FileDeletionMode) { };

    kiriview::DocumentSessionVideoDocumentPort videoPort;
    videoPort.snapshot = [&videoSnapshot]() { return videoSnapshot; };
    videoPort.setSourceUrl = [&videoSnapshot](const QUrl &url) { videoSnapshot.sourceUrl = url; };
    videoPort.videoOutput = []() -> QObject * { return nullptr; };
    videoPort.stop = []() { };
    videoPort.setVideoOutput = [](QObject *) { };
    videoPort.setVideoOutputGeometry = [](const QRectF &, const QRectF &) { };

    kiriview::DocumentSessionRuntime runtime(&owner, std::move(imagePort), std::move(videoPort));

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
    const QUrl imageUrl = localUrl(QStringLiteral("/media/image.png"));
    kiriview::DocumentSessionImageDocumentSnapshot imageSnapshot;
    kiriview::DocumentSessionVideoDocumentSnapshot videoSnapshot;
    kiriview::DocumentSessionDocumentChangeHandler imageSnapshotChanged;

    kiriview::DocumentSessionImageDocumentPort imagePort;
    imagePort.snapshot = [&imageSnapshot]() { return imageSnapshot; };
    imagePort.snapshotChanged
        = [&](QObject *, kiriview::DocumentSessionDocumentChangeHandler handler) {
              imageSnapshotChanged = std::move(handler);
              return QMetaObject::Connection {};
          };
    imagePort.setSourceUrl = [&imageSnapshot](const QUrl &url) {
        imageSnapshot.sourceUrl = url;
        imageSnapshot.displayedUrl = url;
        imageSnapshot.windowTitleFileName = url.fileName();
        imageSnapshot.primaryImageSize = QSize(320, 200);
        imageSnapshot.ready = !url.isEmpty();
        imageSnapshot.ordinaryDirectMediaScopeActive = !url.isEmpty();
        imageSnapshot.zoomPercentKnown = true;
        imageSnapshot.zoomPercent = 100.0;
    };
    imagePort.openPreviousPage = []() { };
    imagePort.openNextPage = []() { };
    imagePort.openImageAtPage = [](int) { };
    imagePort.deleteDisplayedFile = [](kiriview::FileDeletionMode) { };

    kiriview::DocumentSessionVideoDocumentPort videoPort;
    videoPort.snapshot = [&videoSnapshot]() { return videoSnapshot; };
    videoPort.setSourceUrl = [&videoSnapshot](const QUrl &url) { videoSnapshot.sourceUrl = url; };
    videoPort.videoOutput = []() -> QObject * { return nullptr; };
    videoPort.stop = []() { };
    videoPort.setVideoOutput = [](QObject *) { };
    videoPort.setVideoOutputGeometry = [](const QRectF &, const QRectF &) { };

    kiriview::DocumentSessionRuntime runtime(&owner, std::move(imagePort), std::move(videoPort));
    runtime.setSourceUrl(imageUrl);
    QVERIFY(imageSnapshotChanged);

    imageSnapshot.windowTitleFileName = QStringLiteral("renamed.png");
    imageSnapshot.primaryImageSize = QSize(640, 480);
    imageSnapshotChanged();

    QVERIFY(runtime.windowTitleSubject().startsWith(QStringLiteral("renamed.png")));
    QVERIFY(runtime.windowTitleSubject().contains(QStringLiteral("640")));
    QVERIFY(runtime.windowTitleSubject().contains(QStringLiteral("480")));
}

QTEST_GUILESS_MAIN(TestDocumentSessionRuntimeLeafSnapshots)

#include "test_documentsessionruntimeleafsnapshots.moc"

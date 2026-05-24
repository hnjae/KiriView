// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentruntime.h"

#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRectF>
#include <QSize>
#include <QTest>
#include <memory>
#include <utility>

class TestVideoDocumentRuntimeZoom : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void remainsUnknownWithoutRenderContext();
    void calculatesZoomWhenVideoOutputHasWindow();
};

namespace {
class FakeVideoMediaBackend final : public KiriView::VideoMediaBackend
{
public:
    void setCallbacks(KiriView::VideoMediaBackendCallbacks nextCallbacks) override
    {
        callbacks = std::move(nextCallbacks);
    }

    void setSource(const QUrl &nextSourceUrl) override { sourceUrl = nextSourceUrl; }
    void play() override
    {
        isPlaying = true;
        callbacks.playingChanged();
    }
    void pause() override
    {
        isPlaying = false;
        callbacks.playingChanged();
    }
    void stop() override
    {
        isPlaying = false;
        callbacks.playingChanged();
    }
    void setPosition(qint64 nextPosition) override
    {
        currentPosition = nextPosition;
        callbacks.positionChanged();
    }
    void setVideoOutput(QObject *nextVideoOutput) override { output = nextVideoOutput; }
    QObject *videoOutput() const override { return output.data(); }
    KiriView::VideoMediaStatus mediaStatus() const override { return currentStatus; }
    QString errorString() const override { return {}; }
    qint64 duration() const override { return 0; }
    qint64 position() const override { return currentPosition; }
    bool playing() const override { return isPlaying; }
    bool seekable() const override { return false; }
    bool hasVideo() const override { return videoAvailable; }
    bool hasAudio() const override { return false; }
    QSize videoSize() const override { return {}; }

    void emitStatus(KiriView::VideoMediaStatus status)
    {
        currentStatus = status;
        callbacks.mediaStatusChanged();
    }

    void emitHasVideo(bool hasVideo)
    {
        videoAvailable = hasVideo;
        callbacks.hasVideoChanged();
    }

    KiriView::VideoMediaBackendCallbacks callbacks;
    QUrl sourceUrl;
    QPointer<QObject> output;
    KiriView::VideoMediaStatus currentStatus = KiriView::VideoMediaStatus::Null;
    qint64 currentPosition = 0;
    bool isPlaying = false;
    bool videoAvailable = false;
};

class ImmediateVideoPlaybackUrlResolver final : public KiriView::VideoPlaybackUrlResolver
{
public:
    void resolve(quint64 operationId, const QUrl &sourceUrl, QObject *,
        KiriView::VideoPlaybackUrlResolvedCallback resolvedCallback,
        KiriView::VideoPlaybackUrlFailedCallback) override
    {
        resolvedCallback(KiriView::VideoPlaybackUrlResolution {
            operationId,
            sourceUrl,
            sourceUrl,
        });
    }

    void cancel() override { }
    void cleanup() override { }
};

struct RuntimeFixture {
    QObject documentObject;
    FakeVideoMediaBackend *backend = nullptr;
    std::unique_ptr<KiriView::VideoDocumentRuntime> runtime;

    RuntimeFixture()
    {
        auto mediaBackend = std::make_unique<FakeVideoMediaBackend>();
        backend = mediaBackend.get();
        runtime = std::make_unique<KiriView::VideoDocumentRuntime>(&documentObject,
            KiriView::VideoDocumentRuntime::ChangeCallback(), std::move(mediaBackend),
            std::make_unique<ImmediateVideoPlaybackUrlResolver>());
        runtime->setSourceUrl(QUrl::fromLocalFile(QStringLiteral("/videos/clip.mp4")));
        backend->emitHasVideo(true);
        backend->emitStatus(KiriView::VideoMediaStatus::Buffered);
    }
};
}

void TestVideoDocumentRuntimeZoom::remainsUnknownWithoutRenderContext()
{
    RuntimeFixture fixture;

    fixture.runtime->setVideoOutputGeometry(
        QRectF(0.0, 0.0, 1280.0, 720.0), QRectF(0.0, 0.0, 1280.0, 720.0));

    QVERIFY(!fixture.runtime->zoomPercentKnown());
    QCOMPARE(fixture.runtime->zoomPercent(), 0);
}

void TestVideoDocumentRuntimeZoom::calculatesZoomWhenVideoOutputHasWindow()
{
    RuntimeFixture fixture;
    QQuickWindow window;
    QQuickItem output;
    output.setParentItem(window.contentItem());

    fixture.runtime->setVideoOutput(&output);
    fixture.runtime->setVideoOutputGeometry(
        QRectF(0.0, 0.0, 1280.0, 720.0), QRectF(0.0, 0.0, 1280.0, 720.0));

    QVERIFY(fixture.runtime->zoomPercentKnown());
    QCOMPARE(fixture.runtime->zoomPercent(), 100);
}

QTEST_MAIN(TestVideoDocumentRuntimeZoom)

#include "test_videodocumentruntimezoom.moc"

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videoplaybackurlresolver.h"

#include <KProtocolInfo>
#include <KZip>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QEventLoop>
#include <QObject>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <memory>

class TestVideoPlaybackUrlResolver : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directBackendUrlsResolveToThemselves();
    void nonLocalKioProtocolsFailWithoutPlaybackUrl();
    void zipArchiveEntriesResolveToBackendConsumablePlaybackUrls();
};

namespace {
struct ResolverResult
{
    bool finished = false;
    bool resolved = false;
    quint64 operationId = 0;
    QUrl sourceUrl;
    QUrl playbackUrl;
    QString errorString;
};

bool kioFuseServiceMayResolveUrls()
{
    QDBusConnectionInterface* interface = QDBusConnection::sessionBus().interface();
    if (interface == nullptr) {
        return false;
    }

    const QDBusReply<QStringList> registeredServices = interface->registeredServiceNames();
    if (registeredServices.isValid()
        && registeredServices.value().contains(QStringLiteral("org.kde.KIOFuse"))) {
        return true;
    }

    const QDBusReply<QStringList> activatableServices = interface->activatableServiceNames();
    return activatableServices.isValid()
        && activatableServices.value().contains(QStringLiteral("org.kde.KIOFuse"));
}

ResolverResult resolvePlaybackUrl(const QUrl& sourceUrl, int timeoutMilliseconds = 10000)
{
    QObject receiver;
    QEventLoop loop;
    QTimer timeout;
    ResolverResult result;
    std::unique_ptr<kiriview::VideoPlaybackUrlResolver> resolver
        = kiriview::createDefaultVideoPlaybackUrlResolver();

    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
        result.finished = true;
        result.errorString = QStringLiteral("Timed out resolving playback URL.");
        resolver->cancel();
        loop.quit();
    });

    const auto finish = [&]() {
        result.finished = true;
        timeout.stop();
        loop.quit();
    };

    timeout.start(timeoutMilliseconds);
    resolver->resolve(
        1, sourceUrl, &receiver,
        [&](kiriview::VideoPlaybackUrlResolution resolution) {
            result.resolved = true;
            result.operationId = resolution.operationId;
            result.sourceUrl = resolution.sourceUrl;
            result.playbackUrl = resolution.playbackUrl;
            finish();
        },
        [&](quint64 operationId, QUrl failedSourceUrl, QString errorString) {
            result.resolved = false;
            result.operationId = operationId;
            result.sourceUrl = failedSourceUrl;
            result.errorString = errorString;
            finish();
        });

    if (!result.finished) {
        loop.exec();
    }

    resolver->cleanup();
    return result;
}
}

void TestVideoPlaybackUrlResolver::directBackendUrlsResolveToThemselves()
{
    const QUrl sourceUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/kiriview/clip.mp4"));

    const ResolverResult result = resolvePlaybackUrl(sourceUrl);

    QVERIFY(result.finished);
    QVERIFY(result.resolved);
    QCOMPARE(result.operationId, quint64(1));
    QCOMPARE(result.sourceUrl, sourceUrl);
    QCOMPARE(result.playbackUrl, sourceUrl);
}

void TestVideoPlaybackUrlResolver::nonLocalKioProtocolsFailWithoutPlaybackUrl()
{
    const QUrl sourceUrl(QStringLiteral("kiriview-unresolved:/share/clip.mp4"));
    QVERIFY(!kiriview::videoPlaybackBackendCanConsumeUrl(sourceUrl));
    QVERIFY(KProtocolInfo::protocolClass(sourceUrl.scheme()) != QLatin1String(":local"));

    const ResolverResult result = resolvePlaybackUrl(sourceUrl);

    QVERIFY(result.finished);
    QVERIFY(!result.resolved);
    QCOMPARE(result.operationId, quint64(1));
    QCOMPARE(result.sourceUrl, sourceUrl);
    QVERIFY(result.playbackUrl.isEmpty());
    QVERIFY(!result.errorString.isEmpty());
}

void TestVideoPlaybackUrlResolver::zipArchiveEntriesResolveToBackendConsumablePlaybackUrls()
{
    if (KProtocolInfo::protocolClass(QStringLiteral("zip")) != QLatin1String(":local")) {
        QSKIP("The zip KIO worker is not available as a local protocol.");
    }
    if (!kioFuseServiceMayResolveUrls()) {
        QSKIP("KIOFuse is not registered or activatable on the session bus.");
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString archivePath = directory.filePath(QStringLiteral("_vid_raw_mixed.zip"));
    KZip archive(archivePath);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    QVERIFY(archive.writeFile(QStringLiteral("foo.mp4"), QByteArrayLiteral("not-a-real-video")));
    archive.close();

    const QUrl sourceUrl(QStringLiteral("zip:%1/foo.mp4").arg(archivePath));
    QVERIFY(sourceUrl.isValid());
    QVERIFY(!kiriview::videoPlaybackBackendCanConsumeUrl(sourceUrl));

    const ResolverResult result = resolvePlaybackUrl(sourceUrl);

    QVERIFY2(result.finished, qPrintable(result.errorString));
    QVERIFY2(result.resolved, qPrintable(result.errorString));
    QCOMPARE(result.operationId, quint64(1));
    QCOMPARE(result.sourceUrl, sourceUrl);
    QVERIFY(kiriview::videoPlaybackBackendCanConsumeUrl(result.playbackUrl));
    QVERIFY(result.playbackUrl.isLocalFile());
}

QTEST_GUILESS_MAIN(TestVideoPlaybackUrlResolver)

#include "test_videoplaybackurlresolver.moc"

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediapredecoderuntime.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

class TestDocumentSessionMediaPredecodeRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void activeDirectMediaScheduleCachesDisplayedAndLoadsAdjacentImage();
    void inactiveScheduleDoesNotStartPredecode();
    void cacheDisplayedImagesUsesOnlyReadyDirectImageSourceScope();
};

namespace {
using kiriview::TestSupport::imageDecodeDependenciesFor;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::staticImageDataDecoder;
using kiriview::TestSupport::testImage;

QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl& url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

kiriview::DisplayedPredecodeImage displayedImage(const QUrl& url)
{
    return kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(url),
        true,
        staticDisplayTestImagePayload(testImage()),
    };
}

kiriview::DocumentSessionMediaPredecodeInput activeImageInput(const QUrl& currentUrl)
{
    return kiriview::DocumentSessionMediaPredecodeInput {
        true,
        kiriview::DocumentSessionKind::Image,
        true,
        true,
        currentUrl,
        displayedImage(currentUrl),
        {},
    };
}

kiriview::DocumentSessionMediaPredecodeInput inactiveImageInput(const QUrl& currentUrl)
{
    kiriview::DocumentSessionMediaPredecodeInput input = activeImageInput(currentUrl);
    input.directMediaNavigationActive = false;
    return input;
}

kiriview::MediaPredecodeDependencyOverrides predecodeDependencies(ManualImageDataLoader& dataLoader)
{
    kiriview::MediaPredecodeDependencyOverrides dependencies;
    dependencies.imageDecode = imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder());
    dependencies.cacheBudgetRequest.predecodeCacheByteBudget = 1024 * 1024;
    return dependencies;
}
}

void TestDocumentSessionMediaPredecodeRuntime::
    activeDirectMediaScheduleCachesDisplayedAndLoadsAdjacentImage()
{
    ManualImageDataLoader dataLoader;
    kiriview::DocumentSessionMediaPredecodeRuntime runtime(this, predecodeDependencies(dataLoader));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/current.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/next.png"));

    runtime.schedule(activeImageInput(currentUrl),
        { directMediaNavigationCandidate(currentUrl), directMediaNavigationCandidate(nextUrl) });

    QVERIFY(runtime.findPredecodedImage(currentUrl).has_value());
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
}

void TestDocumentSessionMediaPredecodeRuntime::inactiveScheduleDoesNotStartPredecode()
{
    ManualImageDataLoader dataLoader;
    kiriview::DocumentSessionMediaPredecodeRuntime runtime(this, predecodeDependencies(dataLoader));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/current.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/next.png"));

    runtime.schedule(inactiveImageInput(currentUrl),
        { directMediaNavigationCandidate(currentUrl), directMediaNavigationCandidate(nextUrl) });

    QCOMPARE(dataLoader.loadCount(), std::size_t(0));
    QVERIFY(!runtime.findPredecodedImage(currentUrl).has_value());
}

void TestDocumentSessionMediaPredecodeRuntime::
    cacheDisplayedImagesUsesOnlyReadyDirectImageSourceScope()
{
    ManualImageDataLoader dataLoader;
    kiriview::DocumentSessionMediaPredecodeRuntime runtime(this, predecodeDependencies(dataLoader));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/current.png"));

    runtime.cacheDisplayedImages(inactiveImageInput(currentUrl));

    QVERIFY(!runtime.findPredecodedImage(currentUrl).has_value());

    runtime.cacheDisplayedImages(activeImageInput(currentUrl));

    QVERIFY(runtime.findPredecodedImage(currentUrl).has_value());
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaPredecodeRuntime)

#include "test_documentsessionmediapredecoderuntime.moc"

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imageformatregistry.h"
#include "decoding/kiriimagedecoder.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>

class TestRawDecodeIntegration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void smallDngFixtureDecodesThroughPublicDecodePath();
    void smallDngFixtureDecodesWhenRequestUrlLacksRawExtension();
};

namespace {
QByteArray rawFixtureData(QFileInfo *fixtureInfo)
{
    *fixtureInfo
        = QFileInfo(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/raw-cfa-smoke.dng"));
    QFile file(fixtureInfo->filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return file.readAll();
}

void verifyDecodedRawFixture(
    const QByteArray &imageData, const KiriView::ImageDecodeRequest &request)
{
    const KiriView::DecodedImageResult result = KiriView::decodeImageData(imageData, request);

    const auto *decoded = KiriView::decodedImageResultImageAs<KiriView::StaticDecodedImage>(result);
    const auto *failure = KiriView::decodedImageResultFailure(result);
    QVERIFY2(decoded != nullptr,
        qPrintable(failure != nullptr ? failure->errorString
                                      : QStringLiteral("RAW fixture did not decode.")));

    QVERIFY(decoded->staticImage.source != nullptr);
    QCOMPARE(decoded->staticImage.source->imageSize(), QSize(32, 32));
    QVERIFY(!decoded->staticImage.preview.isNull());
    QVERIFY(!decoded->staticImage.preview.size().isEmpty());
}
}

void TestRawDecodeIntegration::smallDngFixtureDecodesThroughPublicDecodePath()
{
    QFileInfo fixtureInfo;
    const QByteArray imageData = rawFixtureData(&fixtureInfo);
    QVERIFY2(fixtureInfo.exists(), qPrintable(fixtureInfo.filePath()));
    QVERIFY(!imageData.isEmpty());

    const KiriView::ImageDecodeRequest request
        = KiriView::ImageDecodeRequest::fromUrl(1, QUrl::fromLocalFile(fixtureInfo.filePath()));
    verifyDecodedRawFixture(imageData, request);
}

void TestRawDecodeIntegration::smallDngFixtureDecodesWhenRequestUrlLacksRawExtension()
{
    const QFileInfo fixtureInfo(
        QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/raw-cfa-smoke.dng"));
    QFileInfo loadedFixtureInfo;
    const QByteArray imageData = rawFixtureData(&loadedFixtureInfo);
    QVERIFY2(loadedFixtureInfo.exists(), qPrintable(loadedFixtureInfo.filePath()));
    QVERIFY(!imageData.isEmpty());

    const QUrl extensionlessUrl
        = QUrl::fromLocalFile(fixtureInfo.absolutePath() + QStringLiteral("/raw-cfa-smoke"));
    const KiriView::ImageDecodeRequest request
        = KiriView::ImageDecodeRequest::fromUrl(1, extensionlessUrl);
    QVERIFY(!KiriView::isSupportedRawImageFileName(request.imageUrl().fileName()));
    verifyDecodedRawFixture(imageData, request);
}

QTEST_GUILESS_MAIN(TestRawDecodeIntegration)

#include "test_rawdecodeintegration.moc"

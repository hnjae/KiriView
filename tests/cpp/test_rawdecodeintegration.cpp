// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imageformatregistry.h"
#include "decoding/kiriimagedecoder.h"
#include "decoding/rawdecoder.h"

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
    void invalidRawDataPreservesBackendFailureDiagnostics();
};

namespace {
QByteArray rawFixtureData(QFileInfo* fixtureInfo)
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
    const QByteArray& imageData, const kiriview::ImageDecodeRequest& request)
{
    const kiriview::DecodedImageResult result = kiriview::decodeImageData(imageData, request);

    const auto* decoded = kiriview::decodedImageResultImageAs<kiriview::StaticDecodedImage>(result);
    const auto* failure = kiriview::decodedImageResultFailure(result);
    QVERIFY2(decoded != nullptr,
        qPrintable(failure != nullptr ? failure->errorString
                                      : QStringLiteral("RAW fixture did not decode.")));

    QVERIFY(decoded->displayImage.refinementSource != nullptr);
    QCOMPARE(decoded->displayImage.originalSize, QSize(32, 32));
    QVERIFY(!decoded->displayImage.image.isNull());
    QVERIFY(!decoded->displayImage.image.size().isEmpty());
}
}

void TestRawDecodeIntegration::smallDngFixtureDecodesThroughPublicDecodePath()
{
    QFileInfo fixtureInfo;
    const QByteArray imageData = rawFixtureData(&fixtureInfo);
    QVERIFY2(fixtureInfo.exists(), qPrintable(fixtureInfo.filePath()));
    QVERIFY(!imageData.isEmpty());

    const kiriview::ImageDecodeRequest request
        = kiriview::ImageDecodeRequest::fromUrl(1, QUrl::fromLocalFile(fixtureInfo.filePath()));
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
    const kiriview::ImageDecodeRequest request
        = kiriview::ImageDecodeRequest::fromUrl(1, extensionlessUrl);
    QVERIFY(!kiriview::isSupportedRawImageFileName(request.imageUrl().fileName()));
    verifyDecodedRawFixture(imageData, request);
}

void TestRawDecodeIntegration::invalidRawDataPreservesBackendFailureDiagnostics()
{
    const kiriview::DecodedImageResult result = kiriview::decodeRawImageData(
        QByteArrayLiteral("not raw image data"), kiriview::ImageDecodeRequest {});

    const kiriview::DecodedImageFailure* failure = kiriview::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->route, kiriview::DecodedImageFailureRoute::Raw);
    QVERIFY(failure->operation != kiriview::DecodedImageFailureOperation::Unknown);
    QVERIFY(!failure->diagnosticDetail.isEmpty());
    QVERIFY(failure->diagnosticDetail != failure->errorString);
    QVERIFY(kiriview::decodedImageResultImage(result) == nullptr);
}

QTEST_GUILESS_MAIN(TestRawDecodeIntegration)

#include "test_rawdecodeintegration.moc"

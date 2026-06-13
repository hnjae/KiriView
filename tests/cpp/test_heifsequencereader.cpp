// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/heifsequencereader.h"

#include "image_test_support.h"
#include "localization/imageerrortext.h"

#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QSize>
#include <QTest>
#include <optional>

class TestHeifSequenceReader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rejectsNonHeifData();
    void readsFramesFromStreamingSequence();
    void closeClearsTheActiveSequence();
};

namespace {
using kiriview::TestSupport::heifFtypBox;

QByteArray fixtureData()
{
    QFile file(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/heif-sequence-alpha.heics"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}
}

void TestHeifSequenceReader::rejectsNonHeifData()
{
    kiriview::HeifSequenceReader reader;

    const kiriview::HeifSequenceOpenResult emptyResult = reader.open({});
    QCOMPARE(emptyResult.status, kiriview::HeifSequenceOpenStatus::NotHeif);

    const kiriview::HeifSequenceOpenResult nonHeifResult = reader.open(heifFtypBox("png ", {}));
    QCOMPARE(nonHeifResult.status, kiriview::HeifSequenceOpenStatus::NotHeif);
}

void TestHeifSequenceReader::readsFramesFromStreamingSequence()
{
    const QByteArray imageData = fixtureData();
    QVERIFY(!imageData.isEmpty());

    kiriview::HeifSequenceReader reader;
    const kiriview::HeifSequenceOpenResult openResult = reader.open(imageData);
    QCOMPARE(openResult.status, kiriview::HeifSequenceOpenStatus::Success);

    QString errorString;
    const std::optional<kiriview::AnimationFrame> firstFrame = reader.readNextFrame(&errorString);
    QVERIFY2(firstFrame.has_value(), qPrintable(errorString));
    QCOMPARE(firstFrame->image.size(), QSize(64, 64));
    QVERIFY(firstFrame->delay > 0);
    QVERIFY(qAlpha(firstFrame->image.pixel(16, 32)) > 0);
    QVERIFY(qAlpha(firstFrame->image.pixel(48, 32)) < 255);

    const std::optional<kiriview::AnimationFrame> secondFrame = reader.readNextFrame(&errorString);
    QVERIFY2(secondFrame.has_value(), qPrintable(errorString));
    QCOMPARE(secondFrame->image.size(), QSize(64, 64));
    QVERIFY(secondFrame->delay > 0);
    QVERIFY(qAlpha(secondFrame->image.pixel(16, 32)) < 255);
    QVERIFY(qAlpha(secondFrame->image.pixel(48, 32)) > 0);
}

void TestHeifSequenceReader::closeClearsTheActiveSequence()
{
    const QByteArray imageData = fixtureData();
    QVERIFY(!imageData.isEmpty());

    kiriview::HeifSequenceReader reader;
    const kiriview::HeifSequenceOpenResult openResult = reader.open(imageData);
    QCOMPARE(openResult.status, kiriview::HeifSequenceOpenStatus::Success);

    reader.close();

    QString errorString;
    const std::optional<kiriview::AnimationFrame> frame = reader.readNextFrame(&errorString);
    QVERIFY(!frame.has_value());
    QCOMPARE(errorString,
        kiriview::imageErrorText(kiriview::ImageErrorTextId::HeifSequenceTrackMissing));
}

QTEST_GUILESS_MAIN(TestHeifSequenceReader)

#include "test_heifsequencereader.moc"

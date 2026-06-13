// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "localization/imageerrortext.h"

#include <QObject>
#include <QString>
#include <QTest>

class TestImageErrorText : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void plainTextIdsExposeCanonicalMessages();
    void actionTextIdsExposeCanonicalFragments();
    void formattedDecodeErrorsPreserveActionAndDetail();
};

void TestImageErrorText::plainTextIdsExposeCanonicalMessages()
{
    QCOMPARE(kiriview::imageErrorText(kiriview::ImageErrorTextId::ReadImageData),
        QStringLiteral("Could not read the selected image data."));
    QCOMPARE(kiriview::imageErrorText(kiriview::ImageErrorTextId::OpenVideo),
        QStringLiteral("Could not open the selected video."));
    QCOMPARE(kiriview::imageErrorText(kiriview::ImageErrorTextId::HeifSequenceTrackMissing),
        QStringLiteral("Could not decode the selected HEIF image: sequence track is missing."));
    QCOMPARE(kiriview::imageErrorText(kiriview::ImageErrorTextId::RawFullDecodeTooLarge),
        QStringLiteral("The selected RAW image is too large for full-image decoding."));
}

void TestImageErrorText::actionTextIdsExposeCanonicalFragments()
{
    QCOMPARE(kiriview::imageErrorActionText(kiriview::ImageErrorActionTextId::ReadHeifContainer),
        QStringLiteral("reading the HEIF container"));
    QCOMPARE(kiriview::imageErrorActionText(kiriview::ImageErrorActionTextId::CreateDisplayImage),
        QStringLiteral("creating the display image"));
}

void TestImageErrorText::formattedDecodeErrorsPreserveActionAndDetail()
{
    QCOMPARE(kiriview::heifDecodeErrorText(QStringLiteral("reading"), QStringLiteral("truncated")),
        QStringLiteral("Could not decode the selected HEIF image: reading: truncated"));
    QCOMPARE(kiriview::rawDecodeErrorText(QStringLiteral("processing"), QStringLiteral("failed")),
        QStringLiteral("Could not decode the selected RAW image: processing: failed"));
}

QTEST_GUILESS_MAIN(TestImageErrorText)

#include "test_imageerrortext.moc"

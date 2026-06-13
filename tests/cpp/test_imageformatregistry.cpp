// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imageformatregistry.h"

#include <QObject>
#include <QStringList>
#include <QTest>

namespace {
QStringList expectedRawExtensions()
{
    return {
        QStringLiteral("3fr"),
        QStringLiteral("arw"),
        QStringLiteral("bay"),
        QStringLiteral("bmq"),
        QStringLiteral("cr2"),
        QStringLiteral("cr3"),
        QStringLiteral("crw"),
        QStringLiteral("cs1"),
        QStringLiteral("cs2"),
        QStringLiteral("dcr"),
        QStringLiteral("dng"),
        QStringLiteral("erf"),
        QStringLiteral("fff"),
        QStringLiteral("iiq"),
        QStringLiteral("k25"),
        QStringLiteral("kdc"),
        QStringLiteral("mdc"),
        QStringLiteral("mef"),
        QStringLiteral("mos"),
        QStringLiteral("mrw"),
        QStringLiteral("nef"),
        QStringLiteral("nrw"),
        QStringLiteral("orf"),
        QStringLiteral("pef"),
        QStringLiteral("raf"),
        QStringLiteral("raw"),
        QStringLiteral("rdc"),
        QStringLiteral("rwl"),
        QStringLiteral("rw2"),
        QStringLiteral("sr2"),
        QStringLiteral("srf"),
        QStringLiteral("srw"),
        QStringLiteral("x3f"),
    };
}

QStringList expectedRawMimeTypes()
{
    return {
        QStringLiteral("image/x-adobe-dng"),
        QStringLiteral("image/x-canon-cr2"),
        QStringLiteral("image/x-canon-cr3"),
        QStringLiteral("image/x-canon-crw"),
        QStringLiteral("image/x-dcraw"),
        QStringLiteral("image/x-fuji-raf"),
        QStringLiteral("image/x-kde-raw"),
        QStringLiteral("image/x-kodak-dcr"),
        QStringLiteral("image/x-kodak-k25"),
        QStringLiteral("image/x-kodak-kdc"),
        QStringLiteral("image/x-minolta-mrw"),
        QStringLiteral("image/x-nikon-nef"),
        QStringLiteral("image/x-nikon-nrw"),
        QStringLiteral("image/x-olympus-orf"),
        QStringLiteral("image/x-panasonic-raw"),
        QStringLiteral("image/x-panasonic-raw2"),
        QStringLiteral("image/x-panasonic-rw"),
        QStringLiteral("image/x-panasonic-rw2"),
        QStringLiteral("image/x-pentax-pef"),
        QStringLiteral("image/x-sigma-x3f"),
        QStringLiteral("image/x-sony-arw"),
        QStringLiteral("image/x-sony-sr2"),
        QStringLiteral("image/x-sony-srf"),
    };
}
}

class TestImageFormatRegistry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void supportedImageExtensionsIncludeAdvertisedFormats();
    void supportedImageExtensionsIncludeRawFormats();
    void supportedImageMimeTypesIncludeRawFormats();
    void supportedOpenExtensionsIncludeComicBookArchives();
    void supportedOpenExtensionsDoNotAdvertiseGeneralArchives();
};

void TestImageFormatRegistry::supportedImageExtensionsIncludeAdvertisedFormats()
{
    const QStringList extensions = kiriview::supportedImageExtensions();

    QVERIFY(extensions.contains(QStringLiteral("avci")));
    QVERIFY(extensions.contains(QStringLiteral("hej2")));
    QVERIFY(extensions.contains(QStringLiteral("heics")));
    QVERIFY(extensions.contains(QStringLiteral("heifs")));
    QVERIFY(extensions.contains(QStringLiteral("tif")));
    QVERIFY(extensions.contains(QStringLiteral("tiff")));
    QVERIFY(kiriview::isSupportedImageFileName(QStringLiteral("still.AVCI")));
    QVERIFY(kiriview::isSupportedImageFileName(QStringLiteral("still.HEJ2")));
    QVERIFY(kiriview::isSupportedImageFileName(QStringLiteral("sequence.HEICS")));
    QVERIFY(kiriview::isSupportedImageFileName(QStringLiteral("sequence.HEIFS")));
    QVERIFY(kiriview::isSupportedImageFileName(QStringLiteral("scan.TIF")));
    QVERIFY(kiriview::isSupportedImageFileName(QStringLiteral("scan.TIFF")));
}

void TestImageFormatRegistry::supportedImageExtensionsIncludeRawFormats()
{
    const QStringList extensions = kiriview::supportedImageExtensions();
    for (const QString &extension : expectedRawExtensions()) {
        QVERIFY2(extensions.contains(extension), qPrintable(extension));
        QVERIFY2(kiriview::isSupportedImageFileName(QStringLiteral("image.%1").arg(extension)),
            qPrintable(extension));
        QVERIFY2(kiriview::isSupportedRawImageFileName(QStringLiteral("image.%1").arg(extension)),
            qPrintable(extension));
        QVERIFY2(kiriview::isSupportedRawImageFileName(
                     QStringLiteral("image.%1").arg(extension.toUpper())),
            qPrintable(extension));
    }

    QVERIFY(!kiriview::isSupportedRawImageFileName(QStringLiteral("scan.tif")));
    QVERIFY(!kiriview::isSupportedRawImageFileName(QStringLiteral("image.png")));
}

void TestImageFormatRegistry::supportedImageMimeTypesIncludeRawFormats()
{
    const QStringList mimeTypes = kiriview::supportedImageMimeTypes();
    for (const QString &mimeType : expectedRawMimeTypes()) {
        QVERIFY2(mimeTypes.contains(mimeType), qPrintable(mimeType));
    }
}

void TestImageFormatRegistry::supportedOpenExtensionsIncludeComicBookArchives()
{
    const QStringList extensions = kiriview::supportedOpenExtensions();

    QVERIFY(extensions.contains(QStringLiteral("cbz")));
    QVERIFY(extensions.contains(QStringLiteral("cbt")));
    QVERIFY(extensions.contains(QStringLiteral("cb7")));
    QVERIFY(extensions.contains(QStringLiteral("cbr")));
}

void TestImageFormatRegistry::supportedOpenExtensionsDoNotAdvertiseGeneralArchives()
{
    const QStringList extensions = kiriview::supportedOpenExtensions();

    QVERIFY(!extensions.contains(QStringLiteral("zip")));
    QVERIFY(!extensions.contains(QStringLiteral("tar")));
    QVERIFY(!extensions.contains(QStringLiteral("7z")));
    QVERIFY(!extensions.contains(QStringLiteral("rar")));
}

QTEST_GUILESS_MAIN(TestImageFormatRegistry)

#include "test_imageformatregistry.moc"

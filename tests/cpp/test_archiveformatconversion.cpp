// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/archiveformatconversion.h"

#include <QObject>
#include <QTest>
#include <string>

namespace {
rust::String rustString(const QString &text)
{
    const QByteArray utf8Text = text.toUtf8();
    return rust::String(
        std::string(utf8Text.constData(), static_cast<std::size_t>(utf8Text.size())));
}
}

class TestArchiveFormatConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void archiveStorageBackendProjectsBridgeEnums();
    void archiveOpenMatchProjectsAbsentAndFoundBridgeValues();
};

void TestArchiveFormatConversion::archiveStorageBackendProjectsBridgeEnums()
{
    QCOMPARE(kiriview::archiveStorageBackendFromBridge(kiriview::RustArchiveStorageBackend::None),
        kiriview::ArchiveStorageBackend::None);
    QCOMPARE(kiriview::archiveStorageBackendFromBridge(kiriview::RustArchiveStorageBackend::KZip),
        kiriview::ArchiveStorageBackend::KZip);
    QCOMPARE(kiriview::archiveStorageBackendFromBridge(kiriview::RustArchiveStorageBackend::KTar),
        kiriview::ArchiveStorageBackend::KTar);
    QCOMPARE(kiriview::archiveStorageBackendFromBridge(kiriview::RustArchiveStorageBackend::K7Zip),
        kiriview::ArchiveStorageBackend::K7Zip);
    QCOMPARE(
        kiriview::archiveStorageBackendFromBridge(kiriview::RustArchiveStorageBackend::LibArchive),
        kiriview::ArchiveStorageBackend::LibArchive);
}

void TestArchiveFormatConversion::archiveOpenMatchProjectsAbsentAndFoundBridgeValues()
{
    QVERIFY(!kiriview::archiveOpenMatchFromBridge(kiriview::RustArchiveOpenMatch {
                                                      false,
                                                      rustString(QStringLiteral("zip")),
                                                      kiriview::RustArchiveOpenMatchKind::ComicBook,
                                                  })
            .has_value());

    const std::optional<kiriview::ArchiveOpenMatch> comicBookMatch
        = kiriview::archiveOpenMatchFromBridge(kiriview::RustArchiveOpenMatch {
            true,
            rustString(QStringLiteral("zip")),
            kiriview::RustArchiveOpenMatchKind::ComicBook,
        });
    QVERIFY(comicBookMatch.has_value());
    QCOMPARE(comicBookMatch->scheme, QStringLiteral("zip"));
    QVERIFY(comicBookMatch->kind == kiriview::ArchiveOpenMatchKind::ComicBook);

    const std::optional<kiriview::ArchiveOpenMatch> archiveMatch
        = kiriview::archiveOpenMatchFromBridge(kiriview::RustArchiveOpenMatch {
            true,
            rustString(QStringLiteral("rar")),
            kiriview::RustArchiveOpenMatchKind::GeneralArchive,
        });
    QVERIFY(archiveMatch.has_value());
    QCOMPARE(archiveMatch->scheme, QStringLiteral("rar"));
    QVERIFY(archiveMatch->kind == kiriview::ArchiveOpenMatchKind::GeneralArchive);
}

QTEST_GUILESS_MAIN(TestArchiveFormatConversion)

#include "test_archiveformatconversion.moc"

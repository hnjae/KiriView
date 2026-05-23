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
    QCOMPARE(KiriView::archiveStorageBackendFromBridge(KiriView::RustArchiveStorageBackend::None),
        KiriView::ArchiveStorageBackend::None);
    QCOMPARE(KiriView::archiveStorageBackendFromBridge(KiriView::RustArchiveStorageBackend::KZip),
        KiriView::ArchiveStorageBackend::KZip);
    QCOMPARE(KiriView::archiveStorageBackendFromBridge(KiriView::RustArchiveStorageBackend::KTar),
        KiriView::ArchiveStorageBackend::KTar);
    QCOMPARE(KiriView::archiveStorageBackendFromBridge(KiriView::RustArchiveStorageBackend::K7Zip),
        KiriView::ArchiveStorageBackend::K7Zip);
    QCOMPARE(
        KiriView::archiveStorageBackendFromBridge(KiriView::RustArchiveStorageBackend::LibArchive),
        KiriView::ArchiveStorageBackend::LibArchive);
}

void TestArchiveFormatConversion::archiveOpenMatchProjectsAbsentAndFoundBridgeValues()
{
    QVERIFY(!KiriView::archiveOpenMatchFromBridge(KiriView::RustArchiveOpenMatch {
                                                      false,
                                                      rustString(QStringLiteral("zip")),
                                                      KiriView::RustArchiveOpenMatchKind::ComicBook,
                                                  })
            .has_value());

    const std::optional<KiriView::ArchiveOpenMatch> comicBookMatch
        = KiriView::archiveOpenMatchFromBridge(KiriView::RustArchiveOpenMatch {
            true,
            rustString(QStringLiteral("zip")),
            KiriView::RustArchiveOpenMatchKind::ComicBook,
        });
    QVERIFY(comicBookMatch.has_value());
    QCOMPARE(comicBookMatch->scheme, QStringLiteral("zip"));
    QVERIFY(comicBookMatch->kind == KiriView::ArchiveOpenMatchKind::ComicBook);

    const std::optional<KiriView::ArchiveOpenMatch> archiveMatch
        = KiriView::archiveOpenMatchFromBridge(KiriView::RustArchiveOpenMatch {
            true,
            rustString(QStringLiteral("rar")),
            KiriView::RustArchiveOpenMatchKind::GeneralArchive,
        });
    QVERIFY(archiveMatch.has_value());
    QCOMPARE(archiveMatch->scheme, QStringLiteral("rar"));
    QVERIFY(archiveMatch->kind == KiriView::ArchiveOpenMatchKind::GeneralArchive);
}

QTEST_GUILESS_MAIN(TestArchiveFormatConversion)

#include "test_archiveformatconversion.moc"

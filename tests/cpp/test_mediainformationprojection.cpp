// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/mediainformationprojection.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>

namespace {
QString valueForLabel(
    const std::vector<KiriView::MediaInformationProjectionRow> &rows, const QString &label)
{
    for (const KiriView::MediaInformationProjectionRow &row : rows) {
        if (row.label == label) {
            return row.value;
        }
    }

    return {};
}

KiriView::EmbeddedMetadata cameraMetadata()
{
    KiriView::EmbeddedMetadata metadata;
    metadata.cameraMake = QStringLiteral("Kiri Camera Co.");
    metadata.cameraModel = QStringLiteral("KiriCam 1");
    metadata.taken = QStringLiteral("2026-05-31 12:34:56");
    metadata.location = QStringLiteral("37.7749, -122.4194");
    metadata.lens = QStringLiteral("35mm");
    metadata.exposure = QStringLiteral("1/125s f/2.8");
    metadata.iso = QStringLiteral("200");
    metadata.focalLength = QStringLiteral("35mm");
    metadata.software = QStringLiteral("Kiri Metadata");
    metadata.advancedRows.push_back({ QStringLiteral("Artist"), QStringLiteral("Kiri Tester") });
    return metadata;
}
}

class TestMediaInformationProjection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptySessionProjectsUnavailableInformation();
    void readyImageProjectsTargetMetadataAndFileActions();
    void directVideoProjectsVideoSectionFromSessionSource();
    void unsupportedArchiveImageTargetClearsInformation();
};

void TestMediaInformationProjection::emptySessionProjectsUnavailableInformation()
{
    KiriView::MediaInformationProjectionInput input;
    input.inputRevision = 4;

    const KiriView::MediaInformationProjectionSnapshot snapshot
        = KiriView::projectMediaInformation(input, 2);

    QCOMPARE(snapshot.revision, quint64(2));
    QCOMPARE(snapshot.inputRevision, quint64(4));
    QVERIFY(!snapshot.available);
    QVERIFY(!snapshot.canCopyFilePath);
    QVERIFY(!snapshot.canOpenContainingFolder);
    QCOMPARE(snapshot.kind, KiriView::MediaInformationKind::Empty);
    QCOMPARE(snapshot.title, QString());
    QCOMPARE(snapshot.summary, QStringLiteral("No media selected"));
    QCOMPARE(snapshot.mediaSectionTitle, QStringLiteral("Media"));
    QVERIFY(snapshot.targetUrl.isEmpty());
    QVERIFY(snapshot.generalRows.empty());
    QVERIFY(snapshot.mediaRows.empty());
    QVERIFY(snapshot.cameraRows.empty());
    QVERIFY(snapshot.advancedRows.empty());
}

void TestMediaInformationProjection::readyImageProjectsTargetMetadataAndFileActions()
{
    KiriView::MediaInformationProjectionInput input;
    input.inputRevision = 7;
    input.documentKind = KiriView::DocumentSessionKind::Image;
    input.imageReady = true;
    input.imageDisplayedUrl = QUrl::fromLocalFile(QStringLiteral("/media/photo.png"));
    input.imageSize = QSize(640, 480);
    input.imageEmbeddedMetadata = cameraMetadata();

    const KiriView::MediaInformationProjectionSnapshot snapshot
        = KiriView::projectMediaInformation(input, 3);

    QCOMPARE(snapshot.kind, KiriView::MediaInformationKind::Image);
    QVERIFY(snapshot.available);
    QCOMPARE(snapshot.targetUrl, input.imageDisplayedUrl);
    QCOMPARE(snapshot.title, QStringLiteral("photo.png"));
    QCOMPARE(snapshot.summary, QStringLiteral("Image, 640×480 px"));
    QCOMPARE(snapshot.mediaSectionTitle, QStringLiteral("Image"));
    QVERIFY(snapshot.canCopyFilePath);
    QVERIFY(snapshot.canOpenContainingFolder);
    QCOMPARE(valueForLabel(snapshot.generalRows, QStringLiteral("Type")), QStringLiteral("Image"));
    QCOMPARE(valueForLabel(snapshot.generalRows, QStringLiteral("Path")),
        QStringLiteral("/media/photo.png"));
    QCOMPARE(valueForLabel(snapshot.mediaRows, QStringLiteral("Dimensions")),
        QStringLiteral("640×480 px"));
    QCOMPARE(valueForLabel(snapshot.cameraRows, QStringLiteral("Camera")),
        QStringLiteral("Kiri Camera Co. KiriCam 1"));
    QCOMPARE(valueForLabel(snapshot.cameraRows, QStringLiteral("Taken")),
        QStringLiteral("2026-05-31 12:34:56"));
    QCOMPARE(valueForLabel(snapshot.advancedRows, QStringLiteral("Artist")),
        QStringLiteral("Kiri Tester"));
}

void TestMediaInformationProjection::directVideoProjectsVideoSectionFromSessionSource()
{
    KiriView::MediaInformationProjectionInput input;
    input.documentKind = KiriView::DocumentSessionKind::Video;
    input.videoSourceUrl = QUrl(QStringLiteral("zip:///books/book.zip!/chapter/clip.mp4"));
    input.videoSize = QSize(1920, 1080);
    input.videoEmbeddedMetadata.duration = QStringLiteral("00:01:23");

    const KiriView::MediaInformationProjectionSnapshot snapshot
        = KiriView::projectMediaInformation(input, 5);

    QCOMPARE(snapshot.kind, KiriView::MediaInformationKind::Video);
    QVERIFY(snapshot.available);
    QCOMPARE(snapshot.targetUrl, input.videoSourceUrl);
    QCOMPARE(snapshot.title, QStringLiteral("clip.mp4"));
    QCOMPARE(snapshot.summary, QStringLiteral("Video, 1920×1080 px"));
    QCOMPARE(snapshot.mediaSectionTitle, QStringLiteral("Video"));
    QCOMPARE(valueForLabel(snapshot.generalRows, QStringLiteral("Type")), QStringLiteral("Video"));
    QCOMPARE(valueForLabel(snapshot.generalRows, QStringLiteral("Path")),
        QStringLiteral("zip:///books/book.zip!/chapter/clip.mp4"));
    QCOMPARE(
        valueForLabel(snapshot.mediaRows, QStringLiteral("Duration")), QStringLiteral("00:01:23"));
    QCOMPARE(valueForLabel(snapshot.mediaRows, QStringLiteral("Frame Size")),
        QStringLiteral("1920×1080 px"));
    QVERIFY(snapshot.canCopyFilePath);
    QVERIFY(snapshot.canOpenContainingFolder);
}

void TestMediaInformationProjection::unsupportedArchiveImageTargetClearsInformation()
{
    KiriView::MediaInformationProjectionInput input;
    input.documentKind = KiriView::DocumentSessionKind::Image;
    input.imageReady = true;
    input.imageDisplayedUrl = QUrl(QStringLiteral("rar:///books/book.rar!/page.png"));
    input.imageDisplayedOpenedCollectionScope = KiriView::OpenedCollectionScopeLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.rar")),
        QUrl(QStringLiteral("rar:///books/book.rar!/")),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    input.imageSize = QSize(640, 480);

    const KiriView::MediaInformationProjectionSnapshot snapshot
        = KiriView::projectMediaInformation(input, 8);

    QVERIFY(!snapshot.available);
    QVERIFY(!snapshot.canCopyFilePath);
    QVERIFY(!snapshot.canOpenContainingFolder);
    QVERIFY(snapshot.targetUrl.isEmpty());
    QCOMPARE(snapshot.summary, QStringLiteral("No media selected"));
    QVERIFY(snapshot.generalRows.empty());
}

QTEST_GUILESS_MAIN(TestMediaInformationProjection)

#include "test_mediainformationprojection.moc"

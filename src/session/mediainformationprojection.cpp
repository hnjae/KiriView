// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediainformationprojection.h"

#include "archive/archiveformat.h"
#include "session/documentsessiontypes.h"

#include <KLocalizedString>
#include <QDir>
#include <utility>

namespace {
bool hasValidDimensions(const QSize &size) { return size.width() > 0 && size.height() > 0; }

QString dimensionsText(const QSize &size)
{
    if (!hasValidDimensions(size)) {
        return {};
    }

    return QStringLiteral("%1×%2 px").arg(size.width()).arg(size.height());
}

QString fileNameForUrl(const QUrl &url)
{
    QString fileName = url.fileName(QUrl::PrettyDecoded);
    if (!fileName.isEmpty()) {
        return fileName;
    }

    return KiriView::mediaInformationDisplayPathForUrl(url);
}

void appendRowIfValue(
    std::vector<KiriView::MediaInformationProjectionRow> &rows, QString label, const QString &value)
{
    if (!value.isEmpty()) {
        rows.push_back({ std::move(label), value });
    }
}

bool openedCollectionScopeInformationAvailable(const KiriView::OpenedCollectionScopeLocation &scope)
{
    if (scope.isEmpty() || scope.isDirectory()) {
        return true;
    }

    return KiriView::archiveRootSchemeUsesKioFuse(scope.rootUrl().scheme());
}

QUrl imageInformationTargetUrl(const KiriView::MediaInformationProjectionInput &input)
{
    if (input.documentKind != KiriView::DocumentSessionKind::Image || !input.imageReady) {
        return {};
    }

    if (!openedCollectionScopeInformationAvailable(input.imageDisplayedOpenedCollectionScope)) {
        return {};
    }

    return input.imageDisplayedUrl;
}

QUrl videoInformationTargetUrl(const KiriView::MediaInformationProjectionInput &input)
{
    if (input.documentKind != KiriView::DocumentSessionKind::Video) {
        return {};
    }

    return input.videoSourceUrl;
}

QUrl informationTargetUrl(const KiriView::MediaInformationProjectionInput &input)
{
    switch (input.documentKind) {
    case KiriView::DocumentSessionKind::Image:
        return imageInformationTargetUrl(input);
    case KiriView::DocumentSessionKind::Video:
        return videoInformationTargetUrl(input);
    case KiriView::DocumentSessionKind::Empty:
        break;
    }

    return {};
}

bool canOpenContainingLocation(const QUrl &url) { return !url.isEmpty() && !url.path().isEmpty(); }

std::vector<KiriView::MediaInformationProjectionRow> generalRows(
    KiriView::MediaInformationKind kind, const QUrl &targetUrl)
{
    QString typeValue;
    switch (kind) {
    case KiriView::MediaInformationKind::Image:
        typeValue = i18nc("@info:metadata value", "Image");
        break;
    case KiriView::MediaInformationKind::Video:
        typeValue = i18nc("@info:metadata value", "Video");
        break;
    case KiriView::MediaInformationKind::Empty:
        break;
    }

    std::vector<KiriView::MediaInformationProjectionRow> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Type"), typeValue);
    appendRowIfValue(rows, i18nc("@label:metadata", "Path"),
        KiriView::mediaInformationDisplayPathForUrl(targetUrl));
    return rows;
}

std::vector<KiriView::MediaInformationProjectionRow> imageRows(const QSize &size)
{
    std::vector<KiriView::MediaInformationProjectionRow> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Dimensions"), dimensionsText(size));
    return rows;
}

std::vector<KiriView::MediaInformationProjectionRow> videoRows(
    const QSize &size, const KiriView::EmbeddedMetadata &metadata)
{
    std::vector<KiriView::MediaInformationProjectionRow> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Duration"), metadata.duration);
    appendRowIfValue(rows, i18nc("@label:metadata", "Frame Size"),
        !metadata.frameSize.isEmpty() ? metadata.frameSize : dimensionsText(size));
    return rows;
}

QString cameraText(const KiriView::EmbeddedMetadata &metadata)
{
    if (!metadata.cameraMake.isEmpty() && !metadata.cameraModel.isEmpty()) {
        return QStringLiteral("%1 %2").arg(metadata.cameraMake, metadata.cameraModel);
    }
    if (!metadata.cameraMake.isEmpty()) {
        return metadata.cameraMake;
    }
    return metadata.cameraModel;
}

std::vector<KiriView::MediaInformationProjectionRow> cameraRows(
    const KiriView::EmbeddedMetadata &metadata)
{
    std::vector<KiriView::MediaInformationProjectionRow> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Camera"), cameraText(metadata));
    appendRowIfValue(rows, i18nc("@label:metadata", "Taken"), metadata.taken);
    appendRowIfValue(rows, i18nc("@label:metadata", "Location"), metadata.location);
    appendRowIfValue(rows, i18nc("@label:metadata", "Lens"), metadata.lens);
    appendRowIfValue(rows, i18nc("@label:metadata", "Exposure"), metadata.exposure);
    appendRowIfValue(rows, i18nc("@label:metadata", "ISO"), metadata.iso);
    appendRowIfValue(rows, i18nc("@label:metadata", "Focal Length"), metadata.focalLength);
    appendRowIfValue(rows, i18nc("@label:metadata", "Software"), metadata.software);
    return rows;
}

std::vector<KiriView::MediaInformationProjectionRow> advancedRows(
    const KiriView::EmbeddedMetadata &metadata)
{
    std::vector<KiriView::MediaInformationProjectionRow> rows;
    rows.reserve(metadata.advancedRows.size());
    for (const KiriView::EmbeddedMetadataRow &row : metadata.advancedRows) {
        appendRowIfValue(rows, row.label, row.value);
    }
    return rows;
}

KiriView::MediaInformationProjectionSnapshot unavailableSnapshot(
    const KiriView::MediaInformationProjectionInput &input, quint64 revision)
{
    KiriView::MediaInformationProjectionSnapshot snapshot;
    snapshot.revision = revision;
    snapshot.inputRevision = input.inputRevision;
    snapshot.summary = i18nc("@info:metadata unavailable", "No media selected");
    snapshot.mediaSectionTitle = i18nc("@title:group", "Media");
    return snapshot;
}
}

namespace KiriView {
QString mediaInformationDisplayPathForUrl(const QUrl &url)
{
    if (url.isLocalFile()) {
        return QDir::toNativeSeparators(url.toLocalFile());
    }

    return url.toDisplayString(QUrl::PreferLocalFile);
}

MediaInformationProjectionSnapshot projectMediaInformation(
    const MediaInformationProjectionInput &input, quint64 revision)
{
    MediaInformationProjectionSnapshot snapshot = unavailableSnapshot(input, revision);
    snapshot.targetUrl = informationTargetUrl(input);
    if (snapshot.targetUrl.isEmpty()) {
        return snapshot;
    }

    snapshot.available = true;
    snapshot.title = fileNameForUrl(snapshot.targetUrl);
    snapshot.canCopyFilePath = true;
    snapshot.canOpenContainingFolder = canOpenContainingLocation(snapshot.targetUrl);

    switch (input.documentKind) {
    case DocumentSessionKind::Image:
        snapshot.kind = MediaInformationKind::Image;
        snapshot.mediaSectionTitle = i18nc("@title:group", "Image");
        snapshot.summary = hasValidDimensions(input.imageSize)
            ? i18nc("@info:metadata summary", "Image, %1", dimensionsText(input.imageSize))
            : i18nc("@info:metadata summary", "Image");
        snapshot.generalRows = generalRows(snapshot.kind, snapshot.targetUrl);
        snapshot.mediaRows = imageRows(input.imageSize);
        snapshot.cameraRows = cameraRows(input.imageEmbeddedMetadata);
        snapshot.advancedRows = advancedRows(input.imageEmbeddedMetadata);
        break;
    case DocumentSessionKind::Video:
        snapshot.kind = MediaInformationKind::Video;
        snapshot.mediaSectionTitle = i18nc("@title:group", "Video");
        snapshot.summary = hasValidDimensions(input.videoSize)
            ? i18nc("@info:metadata summary", "Video, %1", dimensionsText(input.videoSize))
            : i18nc("@info:metadata summary", "Video");
        snapshot.generalRows = generalRows(snapshot.kind, snapshot.targetUrl);
        snapshot.mediaRows = videoRows(input.videoSize, input.videoEmbeddedMetadata);
        snapshot.cameraRows = cameraRows(input.videoEmbeddedMetadata);
        snapshot.advancedRows = advancedRows(input.videoEmbeddedMetadata);
        break;
    case DocumentSessionKind::Empty:
        break;
    }

    return snapshot;
}

bool sameMediaInformationProjectionSnapshot(
    const MediaInformationProjectionSnapshot &left, const MediaInformationProjectionSnapshot &right)
{
    return left.available == right.available && left.kind == right.kind
        && left.targetUrl == right.targetUrl && left.title == right.title
        && left.summary == right.summary && left.mediaSectionTitle == right.mediaSectionTitle
        && left.canCopyFilePath == right.canCopyFilePath
        && left.canOpenContainingFolder == right.canOpenContainingFolder
        && left.generalRows == right.generalRows && left.mediaRows == right.mediaRows
        && left.cameraRows == right.cameraRows && left.advancedRows == right.advancedRows;
}
}

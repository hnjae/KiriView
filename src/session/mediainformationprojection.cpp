// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediainformationprojection.h"

#include "archive/archiveformat.h"
#include "archive/archivepath.h"
#include "session/documentsessiontypes.h"

#include <KLocalizedString>
#include <QDir>
#include <utility>

namespace {
bool hasValidDimensions(QSize size) { return size.width() > 0 && size.height() > 0; }

QString dimensionsText(QSize size)
{
    if (!hasValidDimensions(size)) {
        return {};
    }

    return QStringLiteral("%1×%2 px").arg(size.width()).arg(size.height());
}

QString fileNameForUrl(const QUrl& url)
{
    QString fileName = url.fileName(QUrl::FullyDecoded);
    if (!fileName.isEmpty()) {
        return fileName;
    }

    return kiriview::mediaInformationDisplayPathForUrl(url);
}

void appendRowIfValue(
    std::vector<kiriview::MediaInformationProjectionRow>& rows, QString label, const QString& value)
{
    if (!value.isEmpty()) {
        rows.push_back({ std::move(label), value });
    }
}

bool openedCollectionScopeInformationAvailable(const kiriview::OpenedCollectionScopeLocation& scope)
{
    if (scope.isEmpty() || scope.isDirectory()) {
        return true;
    }

    return kiriview::archiveRootSchemeUsesKioFuse(scope.rootUrl().scheme());
}

QUrl imageInformationTargetUrl(const kiriview::MediaInformationProjectionInput& input)
{
    if (input.documentKind != kiriview::DocumentSessionKind::Image || !input.imageReady) {
        return {};
    }

    if (!openedCollectionScopeInformationAvailable(input.imageDisplayedOpenedCollectionScope)) {
        return {};
    }

    return input.imageDisplayedUrl;
}

QUrl videoInformationTargetUrl(const kiriview::MediaInformationProjectionInput& input)
{
    if (input.documentKind != kiriview::DocumentSessionKind::Video) {
        return {};
    }

    return input.videoSourceUrl;
}

QUrl informationTargetUrl(const kiriview::MediaInformationProjectionInput& input)
{
    switch (input.documentKind) {
    case kiriview::DocumentSessionKind::Image:
        return imageInformationTargetUrl(input);
    case kiriview::DocumentSessionKind::Video:
        return videoInformationTargetUrl(input);
    case kiriview::DocumentSessionKind::Empty:
        break;
    }

    return {};
}

bool canOpenContainingLocation(const QUrl& url) { return !url.isEmpty() && !url.path().isEmpty(); }

QString generalPathValue(
    const QUrl& targetUrl, const kiriview::OpenedCollectionScopeLocation& openedCollectionScope)
{
    if (!openedCollectionScope.isEmpty()) {
        const QString entryPath
            = kiriview::openedCollectionEntryPathForUrl(openedCollectionScope, targetUrl);
        if (!entryPath.isEmpty()) {
            return entryPath;
        }
    }

    return kiriview::mediaInformationDisplayPathForUrl(targetUrl);
}

std::vector<kiriview::MediaInformationProjectionRow> generalRows(
    kiriview::MediaInformationKind kind, const QUrl& targetUrl,
    const kiriview::OpenedCollectionScopeLocation& openedCollectionScope)
{
    QString typeValue;
    switch (kind) {
    case kiriview::MediaInformationKind::Image:
        typeValue = i18nc("@info:metadata value", "Image");
        break;
    case kiriview::MediaInformationKind::Video:
        typeValue = i18nc("@info:metadata value", "Video");
        break;
    case kiriview::MediaInformationKind::Empty:
        break;
    }

    std::vector<kiriview::MediaInformationProjectionRow> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Type"), typeValue);
    appendRowIfValue(
        rows, i18nc("@label:metadata", "Path"), generalPathValue(targetUrl, openedCollectionScope));
    return rows;
}

std::vector<kiriview::MediaInformationProjectionRow> imageRows(QSize size)
{
    std::vector<kiriview::MediaInformationProjectionRow> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Dimensions"), dimensionsText(size));
    return rows;
}

std::vector<kiriview::MediaInformationProjectionRow> videoRows(
    QSize size, const kiriview::EmbeddedMetadata& metadata)
{
    std::vector<kiriview::MediaInformationProjectionRow> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Duration"), metadata.duration);
    appendRowIfValue(rows, i18nc("@label:metadata", "Frame Size"),
        !metadata.frameSize.isEmpty() ? metadata.frameSize : dimensionsText(size));
    return rows;
}

QString cameraText(const kiriview::EmbeddedMetadata& metadata)
{
    if (!metadata.cameraMake.isEmpty() && !metadata.cameraModel.isEmpty()) {
        return QStringLiteral("%1 %2").arg(metadata.cameraMake, metadata.cameraModel);
    }
    if (!metadata.cameraMake.isEmpty()) {
        return metadata.cameraMake;
    }
    return metadata.cameraModel;
}

std::vector<kiriview::MediaInformationProjectionRow> cameraRows(
    const kiriview::EmbeddedMetadata& metadata)
{
    std::vector<kiriview::MediaInformationProjectionRow> rows;
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

std::vector<kiriview::MediaInformationProjectionRow> advancedRows(
    const kiriview::EmbeddedMetadata& metadata)
{
    std::vector<kiriview::MediaInformationProjectionRow> rows;
    rows.reserve(metadata.advancedRows.size());
    for (const kiriview::EmbeddedMetadataRow& row : metadata.advancedRows) {
        appendRowIfValue(rows, row.label, row.value);
    }
    return rows;
}

kiriview::MediaInformationProjectionSnapshot unavailableSnapshot(
    const kiriview::MediaInformationProjectionInput& input, quint64 revision)
{
    kiriview::MediaInformationProjectionSnapshot snapshot;
    snapshot.revision = revision;
    snapshot.inputRevision = input.inputRevision;
    snapshot.summary = i18nc("@info:metadata unavailable", "No media selected");
    snapshot.mediaSectionTitle = i18nc("@title:group", "Media");
    return snapshot;
}
}

namespace kiriview {
QString mediaInformationDisplayPathForUrl(const QUrl& url)
{
    if (url.isLocalFile()) {
        return QDir::toNativeSeparators(url.toLocalFile());
    }

    return QUrl::fromPercentEncoding(
        url.toDisplayString(QUrl::PreferLocalFile | QUrl::FullyEncoded).toUtf8());
}

MediaInformationProjectionSnapshot projectMediaInformation(
    const MediaInformationProjectionInput& input, quint64 revision)
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
        if (input.imageUnsupportedOpenedCollectionVideo) {
            snapshot.kind = MediaInformationKind::Video;
            snapshot.mediaSectionTitle = i18nc("@title:group", "Video");
            snapshot.summary = i18nc("@info:metadata summary", "Video");
            snapshot.generalRows = generalRows(
                snapshot.kind, snapshot.targetUrl, input.imageDisplayedOpenedCollectionScope);
            break;
        }
        snapshot.kind = MediaInformationKind::Image;
        snapshot.mediaSectionTitle = i18nc("@title:group", "Image");
        snapshot.summary = hasValidDimensions(input.imageSize)
            ? i18nc("@info:metadata summary", "Image, %1", dimensionsText(input.imageSize))
            : i18nc("@info:metadata summary", "Image");
        snapshot.generalRows = generalRows(
            snapshot.kind, snapshot.targetUrl, input.imageDisplayedOpenedCollectionScope);
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
        snapshot.generalRows
            = generalRows(snapshot.kind, snapshot.targetUrl, OpenedCollectionScopeLocation::none());
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
    const MediaInformationProjectionSnapshot& left, const MediaInformationProjectionSnapshot& right)
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

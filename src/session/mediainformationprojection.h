// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAINFORMATIONPROJECTION_H
#define KIRIVIEW_MEDIAINFORMATIONPROJECTION_H

#include "location/imagelocation.h"
#include "metadata/embeddedmetadata.h"

#include <QSize>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <vector>

namespace kiriview {
enum class DocumentSessionKind;

enum class MediaInformationKind {
    Empty,
    Image,
    Video,
};

struct MediaInformationProjectionRow {
    QString label;
    QString value;

    friend bool operator==(
        const MediaInformationProjectionRow &left, const MediaInformationProjectionRow &right)
    {
        return left.label == right.label && left.value == right.value;
    }
    friend bool operator!=(
        const MediaInformationProjectionRow &left, const MediaInformationProjectionRow &right)
    {
        return !(left == right);
    }
};

struct MediaInformationProjectionSnapshot {
    quint64 revision = 0;
    quint64 inputRevision = 0;
    bool available = false;
    MediaInformationKind kind = MediaInformationKind::Empty;
    QUrl targetUrl;
    QString title;
    QString summary;
    QString mediaSectionTitle;
    bool canCopyFilePath = false;
    bool canOpenContainingFolder = false;
    std::vector<MediaInformationProjectionRow> generalRows;
    std::vector<MediaInformationProjectionRow> mediaRows;
    std::vector<MediaInformationProjectionRow> cameraRows;
    std::vector<MediaInformationProjectionRow> advancedRows;
};

struct MediaInformationProjectionInput {
    quint64 inputRevision = 0;
    DocumentSessionKind documentKind {};
    bool imageReady = false;
    bool imageUnsupportedOpenedCollectionVideo = false;
    QUrl imageDisplayedUrl;
    OpenedCollectionScopeLocation imageDisplayedOpenedCollectionScope;
    QSize imageSize;
    EmbeddedMetadata imageEmbeddedMetadata;
    QUrl videoSourceUrl;
    QSize videoSize;
    EmbeddedMetadata videoEmbeddedMetadata;
};

MediaInformationProjectionSnapshot projectMediaInformation(
    const MediaInformationProjectionInput &input, quint64 revision);
bool sameMediaInformationProjectionSnapshot(const MediaInformationProjectionSnapshot &left,
    const MediaInformationProjectionSnapshot &right);
QString mediaInformationDisplayPathForUrl(const QUrl &url);
}

#endif

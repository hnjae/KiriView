// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_EMBEDDEDMETADATA_H
#define KIRIVIEW_EMBEDDEDMETADATA_H

#include <QByteArray>
#include <QString>
#include <vector>

namespace kiriview {
struct EmbeddedMetadataRow
{
    QString label;
    QString value;
};

struct EmbeddedMetadata
{
    QString cameraMake;
    QString cameraModel;
    QString taken;
    QString location;
    QString lens;
    QString exposure;
    QString iso;
    QString focalLength;
    QString software;
    QString duration;
    QString frameSize;
    std::vector<EmbeddedMetadataRow> advancedRows;

    bool isEmpty() const;
};

EmbeddedMetadata parseImageEmbeddedMetadata(const QByteArray& data);
EmbeddedMetadata parsePathEmbeddedMetadata(const QString& path);
}

#endif

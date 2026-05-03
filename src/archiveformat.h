// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEFORMAT_H
#define KIRIVIEW_ARCHIVEFORMAT_H

#include <QString>
#include <QStringList>

namespace KiriView {
enum class ArchiveStorageBackend {
    None,
    KZip,
    KTar,
    K7Zip,
    LibArchive,
};

bool isSupportedArchiveRootScheme(const QString &scheme);
ArchiveStorageBackend archiveStorageBackendForRootScheme(const QString &scheme);
QString comicBookArchiveMarkerForRootScheme(const QString &scheme);
QStringList directArchiveOpenMarkersForRootScheme(const QString &scheme);
}

#endif

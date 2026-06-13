// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAFORMATREGISTRY_H
#define KIRIVIEW_MEDIAFORMATREGISTRY_H

#include <QString>
#include <QStringList>

class QUrl;

namespace kiriview {
struct DirectMediaNavigationCandidate;

QStringList supportedOrdinaryMediaExtensions();
QStringList supportedOrdinaryMediaMimeTypes();
bool isSupportedOrdinaryMediaFileName(const QString &name);
bool isSupportedDirectVideoFileName(const QString &name);
bool isSupportedDirectImageUrl(const QUrl &url);
bool isSupportedDirectVideoUrl(const QUrl &url);
bool isSupportedStillImageDirectMediaNavigationCandidate(
    const DirectMediaNavigationCandidate &candidate);
QStringList ordinaryMediaOpenDialogNameFilters();
}

#endif

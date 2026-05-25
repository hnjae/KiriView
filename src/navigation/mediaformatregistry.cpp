// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaformatregistry.h"

#include "archive/archiveformat.h"
#include "bridge/rustqtconversion.h"
#include "decoding/imageformatregistry.h"
#include "kiriview/src/policy/mediaformatregistry.cxx.h"
#include "navigation/medianavigationmodel.h"

#include <KLocalizedString>
#include <QUrl>

namespace {
template <typename Predicate> bool matchesUrlFileNameOrString(const QUrl &url, Predicate predicate)
{
    const QString fileName = url.fileName(QUrl::PrettyDecoded);
    if (predicate(fileName)) {
        return true;
    }

    return predicate(url.toString(QUrl::PrettyDecoded));
}
}

namespace KiriView {
QStringList supportedOrdinaryMediaExtensions()
{
    return Bridge::qtStringList(rustSupportedOrdinaryMediaExtensions());
}

QStringList supportedOrdinaryMediaMimeTypes()
{
    return Bridge::qtStringList(rustSupportedOrdinaryMediaMimeTypes());
}

bool isSupportedOrdinaryMediaFileName(const QString &name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedOrdinaryMediaFileName);
}

bool isSupportedDirectVideoFileName(const QString &name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedDirectVideoFileName);
}

bool isSupportedDirectImageUrl(const QUrl &url)
{
    return matchesUrlFileNameOrString(url, KiriView::isSupportedImageFileName);
}

bool isSupportedDirectVideoUrl(const QUrl &url)
{
    return matchesUrlFileNameOrString(url, KiriView::isSupportedDirectVideoFileName);
}

bool isSupportedStillImageMediaCandidate(const MediaNavigationCandidate &candidate)
{
    return isSupportedImageFileName(candidate.name) || isSupportedDirectImageUrl(candidate.url);
}

QStringList ordinaryMediaOpenDialogNameFilters()
{
    QStringList extensions = supportedOrdinaryMediaExtensions();
    extensions.append(supportedComicBookArchiveExtensions());
    extensions.sort();
    extensions.removeDuplicates();

    const QString extensionFilter = QStringLiteral("*.") + extensions.join(" *.");
    return {
        ki18nc("@item:inlistbox", "Image, video, and comic book files (%1)")
            .subs(extensionFilter)
            .toString(),
        i18nc("@item:inlistbox", "All files (*)"),
    };
}
}

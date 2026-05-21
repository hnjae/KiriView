// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaformatregistry.h"

#include "archive/archiveformat.h"
#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/mediaformatregistry.cxx.h"
#include "kiriview/src/policy/videoformatregistry.cxx.h"

#include <KLocalizedString>

namespace KiriView {
QStringList supportedOrdinaryMediaExtensions()
{
    return Bridge::qtStringList(rustSupportedOrdinaryMediaExtensions());
}

bool isSupportedOrdinaryMediaFileName(const QString &name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedOrdinaryMediaFileName);
}

bool isSupportedDirectVideoFileName(const QString &name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedDirectVideoFileName);
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

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaopendialogfilters.h"

#include "archive/archiveformat.h"
#include "navigation/mediaformatregistry.h"

#include <KLocalizedString>

namespace kiriview {
QStringList ordinaryMediaOpenDialogNameFilters()
{
    QStringList extensions = supportedOrdinaryMediaExtensions();
    extensions.append(supportedComicBookArchiveExtensions());
    extensions.sort();
    extensions.removeDuplicates();

    const QString extensionFilter = QStringLiteral("*.") + extensions.join(QStringLiteral(" *."));
    return {
        ki18nc("@item:inlistbox", "Image, video, and comic book files (%1)")
            .subs(extensionFilter)
            .toString(),
        i18nc("@item:inlistbox", "All files (*)"),
    };
}
}

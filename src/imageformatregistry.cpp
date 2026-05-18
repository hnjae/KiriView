// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageformatregistry.h"

#include "kiriview/src/imageformatregistry.cxx.h"
#include "rustqtconversion.h"

#include <KLocalizedString>

namespace KiriView {
QStringList supportedImageExtensions()
{
    return Bridge::qtStringList(rustSupportedImageExtensions());
}

QStringList supportedImageMimeTypes()
{
    return Bridge::qtStringList(rustSupportedImageMimeTypes());
}

QStringList supportedOpenExtensions()
{
    return Bridge::qtStringList(rustSupportedOpenExtensions());
}

bool isSupportedImageFileName(const QString &name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedImageFileName);
}

bool isSupportedRawImageFileName(const QString &name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedRawImageFileName);
}

QStringList openDialogNameFilters()
{
    const QString extensionFilter = QStringLiteral("*.") + supportedOpenExtensions().join(" *.");
    return {
        ki18nc("@item:inlistbox", "Image and comic book files (%1)")
            .subs(extensionFilter)
            .toString(),
        i18nc("@item:inlistbox", "All files (*)"),
    };
}
}

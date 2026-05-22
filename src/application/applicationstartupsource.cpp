// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationstartupsource.h"

namespace {
QUrl validInitialSourceUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isValid()) {
        return QUrl();
    }

    return url;
}
}

namespace KiriView {
QUrl initialSourceUrlFromStartupSource(const ApplicationInitialSource &source)
{
    switch (source.kind) {
    case ApplicationInitialSourceKind::None:
        return QUrl();
    case ApplicationInitialSourceKind::LocalFilePath:
        return validInitialSourceUrl(QUrl::fromLocalFile(source.text));
    case ApplicationInitialSourceKind::UrlText:
        return validInitialSourceUrl(QUrl(source.text));
    }

    return QUrl();
}
}

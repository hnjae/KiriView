// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationstartupsource.h"

#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/applicationruntime.cxx.h"

namespace {
QString startupSourceText(const kiriview::ApplicationStartupSource &source)
{
    return kiriview::Bridge::qtString(source.text);
}

QUrl validInitialSourceUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isValid()) {
        return QUrl();
    }

    return url;
}
}

namespace kiriview {
QUrl initialSourceUrlFromStartupSource(const ApplicationStartupSource &source)
{
    switch (source.kind) {
    case ApplicationStartupSourceKind::None:
        return QUrl();
    case ApplicationStartupSourceKind::LocalFilePath:
        return validInitialSourceUrl(QUrl::fromLocalFile(startupSourceText(source)));
    case ApplicationStartupSourceKind::UrlText:
        return validInitialSourceUrl(QUrl(startupSourceText(source)));
    }

    return QUrl();
}
}

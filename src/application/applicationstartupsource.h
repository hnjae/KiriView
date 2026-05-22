// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONSTARTUPSOURCE_H
#define KIRIVIEW_APPLICATIONSTARTUPSOURCE_H

#include <QString>
#include <QUrl>

namespace KiriView {
enum class ApplicationInitialSourceKind {
    None,
    LocalFilePath,
    UrlText,
};

struct ApplicationInitialSource {
    ApplicationInitialSourceKind kind = ApplicationInitialSourceKind::None;
    QString text;
};

QUrl initialSourceUrlFromStartupSource(const ApplicationInitialSource &source);
}

#endif

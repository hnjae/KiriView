// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONSTARTUPSOURCE_H
#define KIRIVIEW_APPLICATIONSTARTUPSOURCE_H

#include <QUrl>

namespace KiriView {
struct ApplicationStartupSource;

QUrl initialSourceUrlFromStartupSource(const ApplicationStartupSource &source);
}

#endif

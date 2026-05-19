// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONRUNTIME_H
#define KIRIVIEW_APPLICATIONRUNTIME_H

class QQmlApplicationEngine;
class QUrl;

namespace KiriView {
struct ApplicationStartupSource;

void initializeApplicationRuntime();
QUrl initialSourceUrlFromStartupSource(const ApplicationStartupSource &source);
void loadApplicationMainQml(
    QQmlApplicationEngine &engine, const ApplicationStartupSource &startupSource);
int runApplication(const ApplicationStartupSource &startupSource);
}

#endif

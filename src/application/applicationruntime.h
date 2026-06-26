// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONRUNTIME_H
#define KIRIVIEW_APPLICATIONRUNTIME_H

class QQmlApplicationEngine;
class QQmlEngine;

namespace kiriview {
struct ApplicationStartupSource;

void initializeApplicationRuntime();
void configureApplicationRuntimeDiagnostics(const ApplicationStartupSource& startupSource);
void registerApplicationImageProviders(QQmlEngine& engine);
void loadApplicationMainQml(
    QQmlApplicationEngine& engine, const ApplicationStartupSource& startupSource);
int runApplication(const ApplicationStartupSource& startupSource);
}

#endif

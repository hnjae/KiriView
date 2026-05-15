// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONRUNTIME_H
#define KIRIVIEW_APPLICATIONRUNTIME_H

class QQmlApplicationEngine;
class QString;
class QUrl;

namespace KiriView {
void initializeApplicationRuntime();
QUrl initialSourceUrlFromLocalFilePath(const QString &path);
QUrl initialSourceUrlFromUrlText(const QString &urlText);
void loadApplicationMainQml(QQmlApplicationEngine &engine, const QUrl &initialSourceUrl);
}

#endif

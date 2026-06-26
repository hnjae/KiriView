// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_LOCALIZATION_H
#define KIRIVIEW_LOCALIZATION_H

class QQmlApplicationEngine;

namespace kiriview {
void initializeLocalization();
void setupLocalizedContext(QQmlApplicationEngine& engine);
}

#endif

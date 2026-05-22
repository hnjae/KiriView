// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONSTARTUPSOURCECONVERSION_H
#define KIRIVIEW_APPLICATIONSTARTUPSOURCECONVERSION_H

namespace KiriView {
struct ApplicationInitialSource;
struct ApplicationStartupSource;

ApplicationInitialSource applicationInitialSourceFromBridge(const ApplicationStartupSource &source);
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONRUNTIMEBRIDGE_H
#define KIRIVIEW_APPLICATIONRUNTIMEBRIDGE_H

namespace KiriView {
struct ApplicationStartupSource;

int runApplicationFromBridge(const ApplicationStartupSource &startupSource);
}

#endif

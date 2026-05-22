// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationruntimebridge.h"

#include "application/applicationruntime.h"
#include "bridge/applicationstartupsourceconversion.h"

namespace KiriView {
int runApplicationFromBridge(const ApplicationStartupSource &startupSource)
{
    return runApplication(applicationInitialSourceFromBridge(startupSource));
}
}

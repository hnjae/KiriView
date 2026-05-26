// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODECACHEBUDGET_H
#define KIRIVIEW_PREDECODECACHEBUDGET_H

#include "system/systemmemory.h"

#include <QtGlobal>

namespace KiriView {
qsizetype defaultPredecodeCacheByteBudget();
qsizetype defaultPredecodeCacheByteBudget(SystemMemorySnapshot systemMemory);
qsizetype resolvedPredecodeCacheByteBudget(qsizetype requestedByteBudget);
qsizetype resolvedPredecodeCacheByteBudget(
    qsizetype requestedByteBudget, SystemMemorySnapshot systemMemory);
}

#endif

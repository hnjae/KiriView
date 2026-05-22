// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SYSTEMMEMORY_H
#define KIRIVIEW_SYSTEMMEMORY_H

#include <QtGlobal>
#include <functional>
#include <optional>

namespace KiriView {
using PhysicalSystemMemoryReader = std::function<std::optional<qsizetype>()>;

struct SystemMemoryRuntime {
    PhysicalSystemMemoryReader readPhysicalSystemMemory;
};

struct SystemMemorySnapshot {
    qsizetype physicalByteSize = 0;

    bool hasPhysicalByteSize() const { return physicalByteSize > 0; }
};

std::optional<qsizetype> physicalSystemMemoryByteSize();
SystemMemoryRuntime defaultSystemMemoryRuntime();
SystemMemoryRuntime systemMemoryRuntimeWithDefaults(SystemMemoryRuntime runtime);
SystemMemorySnapshot systemMemorySnapshot(SystemMemoryRuntime runtime = {});
}

#endif

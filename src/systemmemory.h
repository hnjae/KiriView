// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SYSTEMMEMORY_H
#define KIRIVIEW_SYSTEMMEMORY_H

#include <QtGlobal>
#include <optional>

namespace KiriView {
std::optional<qsizetype> physicalSystemMemoryByteSize();
}

#endif

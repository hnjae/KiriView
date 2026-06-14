// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODERUNTIMEFACTS_H
#define KIRIVIEW_PREDECODERUNTIMEFACTS_H

#include <functional>

namespace kiriview {
using PredecodeThreadCountProvider = std::function<int()>;
}

#endif

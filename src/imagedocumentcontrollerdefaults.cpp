// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include <utility>

namespace KiriView {
// clang-format off
ImageDocumentController::ImageDocumentController(
    QObject *parent, RenderContextProvider renderContextProvider, ChangeCallback changeCallback)
    : ImageDocumentController(parent, std::move(renderContextProvider), std::move(changeCallback),
        defaultImageAsyncDependencies())
{
}
// clang-format on
}

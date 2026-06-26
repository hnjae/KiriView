// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewrendercontextbinding.h"

namespace kiriview {
ImageViewRenderContextBindingAction ImageViewRenderContextBinding::synchronize(
    ImageViewRenderContextBindingInput input)
{
    const bool install = shouldInstallProvider(input);
    if (install == m_providerInstalled) {
        return ImageViewRenderContextBindingAction::None;
    }

    m_providerInstalled = install;
    return install ? ImageViewRenderContextBindingAction::InstallProvider
                   : ImageViewRenderContextBindingAction::ClearProvider;
}

bool ImageViewRenderContextBinding::providerInstalled() const { return m_providerInstalled; }

bool ImageViewRenderContextBinding::shouldInstallProvider(ImageViewRenderContextBindingInput input)
{
    return input.componentComplete && input.documentAttached && !input.secondaryPage;
}
}

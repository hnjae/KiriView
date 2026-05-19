// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewrendercontextbinding.h"

namespace KiriView {
ImageViewRenderContextBindingAction ImageViewRenderContextBinding::setDocumentAttached(
    bool attached)
{
    m_documentAttached = attached;
    return synchronizeProvider();
}

ImageViewRenderContextBindingAction ImageViewRenderContextBinding::setSecondaryPage(
    bool secondaryPage)
{
    m_secondaryPage = secondaryPage;
    return synchronizeProvider();
}

ImageViewRenderContextBindingAction ImageViewRenderContextBinding::setComponentComplete(
    bool complete)
{
    m_componentComplete = complete;
    return synchronizeProvider();
}

ImageViewRenderContextBindingAction ImageViewRenderContextBinding::reset()
{
    m_documentAttached = false;
    m_secondaryPage = false;
    return synchronizeProvider();
}

bool ImageViewRenderContextBinding::providerInstalled() const { return m_providerInstalled; }

bool ImageViewRenderContextBinding::shouldInstallProvider() const
{
    return m_componentComplete && m_documentAttached && !m_secondaryPage;
}

ImageViewRenderContextBindingAction ImageViewRenderContextBinding::synchronizeProvider()
{
    const bool install = shouldInstallProvider();
    if (install == m_providerInstalled) {
        return ImageViewRenderContextBindingAction::None;
    }

    m_providerInstalled = install;
    return install ? ImageViewRenderContextBindingAction::InstallProvider
                   : ImageViewRenderContextBindingAction::ClearProvider;
}
}

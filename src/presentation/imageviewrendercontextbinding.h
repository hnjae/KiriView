// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEVIEWRENDERCONTEXTBINDING_H
#define KIRIVIEW_IMAGEVIEWRENDERCONTEXTBINDING_H

namespace KiriView {
enum class ImageViewRenderContextBindingAction {
    None,
    InstallProvider,
    ClearProvider,
};

class ImageViewRenderContextBinding final
{
public:
    ImageViewRenderContextBindingAction setDocumentAttached(bool attached);
    ImageViewRenderContextBindingAction setSecondaryPage(bool secondaryPage);
    ImageViewRenderContextBindingAction setComponentComplete(bool complete);
    ImageViewRenderContextBindingAction reset();
    bool providerInstalled() const;

private:
    bool shouldInstallProvider() const;
    ImageViewRenderContextBindingAction synchronizeProvider();

    bool m_documentAttached = false;
    bool m_secondaryPage = false;
    bool m_componentComplete = true;
    bool m_providerInstalled = false;
};
}

#endif

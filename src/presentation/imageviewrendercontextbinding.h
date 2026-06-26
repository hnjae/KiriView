// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEVIEWRENDERCONTEXTBINDING_H
#define KIRIVIEW_IMAGEVIEWRENDERCONTEXTBINDING_H

namespace kiriview {
enum class ImageViewRenderContextBindingAction {
    None,
    InstallProvider,
    ClearProvider,
};

struct ImageViewRenderContextBindingInput
{
    bool documentAttached = false;
    bool secondaryPage = false;
    bool componentComplete = true;
};

class ImageViewRenderContextBinding final
{
public:
    ImageViewRenderContextBindingAction synchronize(ImageViewRenderContextBindingInput input);
    bool providerInstalled() const;

private:
    static bool shouldInstallProvider(ImageViewRenderContextBindingInput input);

    bool m_providerInstalled = false;
};
}

#endif

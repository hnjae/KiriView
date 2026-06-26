// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONCOMMANDPORTSOURCE_H
#define KIRIVIEW_APPLICATIONCOMMANDPORTSOURCE_H

#include <QtGlobal>

namespace kiriview::ApplicationActions {
struct ApplicationCommandRouterPorts;
struct ApplicationCommandRouterShellPorts;
struct ApplicationCommandRouterSessionPorts;
struct ApplicationCommandRouterImageDocumentPorts;
struct ApplicationCommandRouterImagePresentationPorts;
struct ApplicationCommandRouterPanelPorts;
struct ApplicationCommandRouterWindowPorts;
struct ApplicationCommandRouterHelpPorts;
struct ApplicationCommandRouterVideoPorts;

class ApplicationCommandPortSource
{
public:
    ApplicationCommandPortSource() = default;

public:
    virtual ~ApplicationCommandPortSource();

    virtual ApplicationCommandRouterShellPorts commandRouterShellPorts();
    virtual ApplicationCommandRouterSessionPorts commandRouterSessionPorts();
    virtual ApplicationCommandRouterImageDocumentPorts commandRouterImageDocumentPorts();
    virtual ApplicationCommandRouterImagePresentationPorts commandRouterImagePresentationPorts();
    virtual ApplicationCommandRouterPanelPorts commandRouterPanelPorts();
    virtual ApplicationCommandRouterWindowPorts commandRouterWindowPorts();
    virtual ApplicationCommandRouterHelpPorts commandRouterHelpPorts();
    virtual ApplicationCommandRouterVideoPorts commandRouterVideoPorts();
    Q_DISABLE_COPY(ApplicationCommandPortSource)
};

ApplicationCommandRouterPorts applicationCommandRouterPorts(ApplicationCommandPortSource& source);
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationcommandportsource.h"

#include "applicationcommandrouter.h"

namespace kiriview::ApplicationActions {
ApplicationCommandPortSource::~ApplicationCommandPortSource() = default;

ApplicationCommandRouterShellPorts ApplicationCommandPortSource::commandRouterShellPorts()
{
    return {};
}

ApplicationCommandRouterSessionPorts ApplicationCommandPortSource::commandRouterSessionPorts()
{
    return {};
}

ApplicationCommandRouterImageDocumentPorts
ApplicationCommandPortSource::commandRouterImageDocumentPorts()
{
    return {};
}

ApplicationCommandRouterImagePresentationPorts
ApplicationCommandPortSource::commandRouterImagePresentationPorts()
{
    return {};
}

ApplicationCommandRouterPanelPorts ApplicationCommandPortSource::commandRouterPanelPorts()
{
    return {};
}

ApplicationCommandRouterWindowPorts ApplicationCommandPortSource::commandRouterWindowPorts()
{
    return {};
}

ApplicationCommandRouterHelpPorts ApplicationCommandPortSource::commandRouterHelpPorts()
{
    return {};
}

ApplicationCommandRouterVideoPorts ApplicationCommandPortSource::commandRouterVideoPorts()
{
    return {};
}

ApplicationCommandRouterPorts applicationCommandRouterPorts(ApplicationCommandPortSource& source)
{
    ApplicationCommandRouterPorts ports;
    ports.shell = source.commandRouterShellPorts();
    ports.session = source.commandRouterSessionPorts();
    ports.imageDocument = source.commandRouterImageDocumentPorts();
    ports.imagePresentation = source.commandRouterImagePresentationPorts();
    ports.panel = source.commandRouterPanelPorts();
    ports.window = source.commandRouterWindowPorts();
    ports.help = source.commandRouterHelpPorts();
    ports.video = source.commandRouterVideoPorts();
    return ports;
}
}

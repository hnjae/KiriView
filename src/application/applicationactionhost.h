// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONACTIONHOST_H
#define KIRIVIEW_APPLICATIONACTIONHOST_H

#include <QtGlobal>

class QAction;
class KirigamiActionCollection;
class QObject;
class QString;

namespace kiriview::ApplicationActions {
class ApplicationActionHost
{
public:
    ApplicationActionHost() = default;

public:
    virtual ~ApplicationActionHost() = default;

    virtual QObject* actionContext() = 0;
    virtual KirigamiActionCollection* mainActionCollection() = 0;
    virtual QAction* inheritedAction(const QString& actionName) = 0;
    virtual void readActionSettings() = 0;
    Q_DISABLE_COPY(ApplicationActionHost)
};
}

#endif

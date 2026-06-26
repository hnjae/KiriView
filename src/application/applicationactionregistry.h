// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONACTIONREGISTRY_H
#define KIRIVIEW_APPLICATIONACTIONREGISTRY_H

#include "applicationactionhost.h"
#include "kiriviewapplicationactions.h"

#include <QAction>
#include <QHash>
#include <QList>
#include <QString>
#include <array>
#include <cstddef>

namespace kiriview::ApplicationActions {
struct RegisteredApplicationAction
{
    QAction* action = nullptr;
    ActionId actionId = ActionId::ActionCount;
    QString actionName;
};

}

namespace kiriview::ApplicationActions {

class ApplicationActionRegistry final
{
public:
    explicit ApplicationActionRegistry(ApplicationActionHost& host);

    QAction* collectionAction(const QString& actionName) const;
    QAction* collectionAction(const ActionDefinition& definition) const;
    void registerAction(const ActionDefinition& definition, QAction* action);
    QAction* action(const QString& actionName) const;
    QAction* actionForId(ActionId actionId) const;
    QString actionName(ActionId actionId) const;
    QList<RegisteredApplicationAction> registeredActions() const;

private:
    static QString definitionName(const ActionDefinition& definition);
    static std::size_t actionIndex(ActionId actionId);

    ApplicationActionHost& m_host;
    std::array<QAction*, actionDefinitionCount> m_actionsById {};
    QHash<QString, QAction*> m_actionsByName;
};
}

#endif

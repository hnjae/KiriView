// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionregistry.h"

#include <cstddef>

namespace KiriView::ApplicationActions {
ApplicationActionRegistry::ApplicationActionRegistry(KiriViewApplication &application)
    : m_application(application)
{
}

QAction *ApplicationActionRegistry::collectionAction(const QString &actionName) const
{
    return const_cast<KiriViewApplication &>(m_application)
        .AbstractKirigamiApplication::action(actionName);
}

QAction *ApplicationActionRegistry::collectionAction(const ActionDefinition &definition) const
{
    return collectionAction(definitionName(definition));
}

void ApplicationActionRegistry::registerAction(const ActionDefinition &definition, QAction *action)
{
    if (action == nullptr) {
        return;
    }

    m_actionsById[actionIndex(definition.actionId)] = action;
    m_actionsByName.insert(definitionName(definition), action);
}

QAction *ApplicationActionRegistry::action(const QString &actionName) const
{
    return m_actionsByName.value(actionName, nullptr);
}

QAction *ApplicationActionRegistry::actionForId(KiriViewApplication::ActionId actionId) const
{
    const ActionDefinition *definition = definitionForId(actionId);
    if (definition == nullptr) {
        return nullptr;
    }

    return m_actionsById[actionIndex(definition->actionId)];
}

QString ApplicationActionRegistry::actionName(KiriViewApplication::ActionId actionId) const
{
    return KiriView::ApplicationActions::actionName(actionId);
}

QList<RegisteredApplicationAction> ApplicationActionRegistry::registeredActions() const
{
    QList<RegisteredApplicationAction> actions;
    actions.reserve(static_cast<qsizetype>(definitions().size()));

    for (const ActionDefinition &definition : definitions()) {
        QAction *registeredAction = actionForId(definition.actionId);
        if (registeredAction == nullptr) {
            continue;
        }

        actions.push_back(RegisteredApplicationAction {
            registeredAction,
            definition.actionId,
            definitionName(definition),
        });
    }

    return actions;
}

QString ApplicationActionRegistry::definitionName(const ActionDefinition &definition)
{
    return QString::fromLatin1(definition.name);
}

std::size_t ApplicationActionRegistry::actionIndex(KiriViewApplication::ActionId actionId)
{
    return static_cast<std::size_t>(actionId);
}
}

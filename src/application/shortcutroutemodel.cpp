// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "shortcutroutemodel.h"

#include "applicationshortcutpolicy.h"

#include <QByteArray>
#include <QHash>
#include <QVariantList>

namespace {
QVariantList actionIdVariants(const QList<kiriview::ApplicationActions::ActionId> &actionIds)
{
    QVariantList variants;
    variants.reserve(actionIds.size());

    for (kiriview::ApplicationActions::ActionId actionId : actionIds) {
        variants.push_back(static_cast<int>(actionId));
    }

    return variants;
}
}

namespace kiriview::ApplicationActions {
ShortcutRouteModel::ShortcutRouteModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ShortcutRouteModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(shortcutRoutes().size());
}

QVariant ShortcutRouteModel::data(const QModelIndex &index, int role) const
{
    const QList<ApplicationShortcutRoute> &routes = shortcutRoutes();
    if (!index.isValid() || index.row() < 0 || index.row() >= routes.size()) {
        return {};
    }

    const ApplicationShortcutRoute &route = routes.at(index.row());
    switch (role) {
    case ActionIdsRole:
        return actionIdVariants(route.actionIds);
    case ActivationScopeRole:
        return static_cast<int>(route.activationScope);
    case ShortcutScopeRole:
        return static_cast<int>(route.shortcutScope);
    default:
        return {};
    }
}

QHash<int, QByteArray> ShortcutRouteModel::roleNames() const
{
    return {
        { ActionIdsRole, QByteArrayLiteral("actionIds") },
        { ActivationScopeRole, QByteArrayLiteral("activationScope") },
        { ShortcutScopeRole, QByteArrayLiteral("shortcutScope") },
    };
}
}

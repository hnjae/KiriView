// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SHORTCUTROUTEMODEL_H
#define KIRIVIEW_SHORTCUTROUTEMODEL_H

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QVariant>

namespace KiriView::ApplicationActions {
class ShortcutRouteModel : public QAbstractListModel
{
public:
    enum Role {
        ActionIdsRole = Qt::UserRole + 1,
        ActivationScopeRole,
        ShortcutScopeRole,
    };

    explicit ShortcutRouteModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};
}

#endif

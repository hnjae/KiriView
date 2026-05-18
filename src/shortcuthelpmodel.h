// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SHORTCUTHELPMODEL_H
#define KIRIVIEW_SHORTCUTHELPMODEL_H

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>
#include <functional>

class QAction;

namespace KiriView::ApplicationActions {
struct ShortcutHelpRow {
    QAction *action = nullptr;
    int actionId = -1;
    QString actionName;
};

using ShortcutHelpRowsProvider = std::function<QList<ShortcutHelpRow>()>;

class ShortcutHelpModel : public QAbstractListModel
{
public:
    enum Role {
        ActionIdRole = Qt::UserRole + 1,
        ActionNameRole,
        ActionTextRole,
        ShortcutTextRole,
    };

    explicit ShortcutHelpModel(ShortcutHelpRowsProvider rowsProvider, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    void handleActionChanged(QAction *changedAction);

private:
    QList<ShortcutHelpRow> collectRows() const;

    static bool sameRows(const QList<ShortcutHelpRow> &left, const QList<ShortcutHelpRow> &right);
    static QString actionDisplayText(const QAction *action);
    static QString shortcutDisplayText(const QAction *action);

    ShortcutHelpRowsProvider m_rowsProvider;
    QList<ShortcutHelpRow> m_rows;
};
}

#endif

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

class QAction;
class KiriViewApplication;

namespace KiriView::ApplicationActions {
class ShortcutHelpModel : public QAbstractListModel
{
public:
    enum Role {
        ActionIdRole = Qt::UserRole + 1,
        ActionNameRole,
        ActionTextRole,
        ShortcutTextRole,
    };

    explicit ShortcutHelpModel(KiriViewApplication &application, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    void handleActionChanged(QAction *changedAction);

private:
    struct Row {
        QAction *action = nullptr;
        int actionId = -1;
        QString actionName;
    };

    QList<Row> collectRows() const;

    static bool sameRows(const QList<Row> &left, const QList<Row> &right);
    static QString actionDisplayText(const QAction *action);
    static QString shortcutDisplayText(const QAction *action);

    KiriViewApplication &m_application;
    QList<Row> m_rows;
};
}

#endif

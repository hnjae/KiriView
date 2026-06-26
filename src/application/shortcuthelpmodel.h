// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SHORTCUTHELPMODEL_H
#define KIRIVIEW_SHORTCUTHELPMODEL_H

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <functional>

namespace kiriview::ApplicationActions {
struct ShortcutHelpRow
{
    int actionId = -1;
    QString actionName;
    QString actionText;
    QString shortcutText;
    QString categoryKey;
    QString categoryText;
    QString scopeText;
    QStringList shortcutKeyTexts;
    bool categoryFirst = false;
    bool categoryLast = false;
};

using ShortcutHelpRowsProvider = std::function<QList<ShortcutHelpRow>()>;

class ShortcutHelpModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        ActionIdRole = Qt::UserRole + 1,
        ActionNameRole,
        ActionTextRole,
        ShortcutTextRole,
        CategoryKeyRole,
        CategoryTextRole,
        CategoryFirstRole,
        CategoryLastRole,
        ShortcutKeyTextsRole,
        ScopeTextRole,
    };

    explicit ShortcutHelpModel(ShortcutHelpRowsProvider rowsProvider, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    void handleRowsChanged();

private:
    QList<ShortcutHelpRow> collectRows() const;

    static bool sameRowIdentities(
        const QList<ShortcutHelpRow>& left, const QList<ShortcutHelpRow>& right);
    static bool sameRowData(const ShortcutHelpRow& left, const ShortcutHelpRow& right);

    ShortcutHelpRowsProvider m_rowsProvider;
    QList<ShortcutHelpRow> m_rows;
};
}

#endif

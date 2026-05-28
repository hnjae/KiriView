// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "shortcuthelpmodel.h"

#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QVariant>
#include <utility>

namespace KiriView::ApplicationActions {
ShortcutHelpModel::ShortcutHelpModel(ShortcutHelpRowsProvider rowsProvider, QObject *parent)
    : QAbstractListModel(parent)
    , m_rowsProvider(std::move(rowsProvider))
    , m_rows(collectRows())
{
}

int ShortcutHelpModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant ShortcutHelpModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }

    const ShortcutHelpRow &row = m_rows.at(index.row());
    switch (role) {
    case ActionIdRole:
        return row.actionId;
    case ActionNameRole:
        return row.actionName;
    case ActionTextRole:
        return row.actionText;
    case ShortcutTextRole:
        return row.shortcutText;
    case CategoryKeyRole:
        return row.categoryKey;
    case CategoryTextRole:
        return row.categoryText;
    case CategoryFirstRole:
        return row.categoryFirst;
    case CategoryLastRole:
        return row.categoryLast;
    case ShortcutKeyTextsRole:
        return row.shortcutKeyTexts;
    default:
        return {};
    }
}

QHash<int, QByteArray> ShortcutHelpModel::roleNames() const
{
    return {
        { ActionIdRole, QByteArrayLiteral("actionId") },
        { ActionNameRole, QByteArrayLiteral("actionName") },
        { ActionTextRole, QByteArrayLiteral("actionText") },
        { ShortcutTextRole, QByteArrayLiteral("shortcutText") },
        { CategoryKeyRole, QByteArrayLiteral("categoryKey") },
        { CategoryTextRole, QByteArrayLiteral("categoryText") },
        { CategoryFirstRole, QByteArrayLiteral("categoryFirst") },
        { CategoryLastRole, QByteArrayLiteral("categoryLast") },
        { ShortcutKeyTextsRole, QByteArrayLiteral("shortcutKeyTexts") },
    };
}

void ShortcutHelpModel::handleRowsChanged()
{
    const QList<ShortcutHelpRow> rows = collectRows();
    if (!sameRowIdentities(rows, m_rows)) {
        beginResetModel();
        m_rows = rows;
        endResetModel();
        return;
    }

    QList<int> changedRows;
    for (int row = 0; row < m_rows.size(); ++row) {
        if (!sameRowData(rows.at(row), m_rows.at(row))) {
            changedRows.push_back(row);
        }
    }

    m_rows = rows;
    for (int row : changedRows) {
        Q_EMIT dataChanged(index(row, 0), index(row, 0),
            { ActionTextRole, ShortcutTextRole, CategoryKeyRole, CategoryTextRole,
                CategoryFirstRole, CategoryLastRole, ShortcutKeyTextsRole });
    }
}

QList<ShortcutHelpRow> ShortcutHelpModel::collectRows() const
{
    return m_rowsProvider ? m_rowsProvider() : QList<ShortcutHelpRow>();
}

bool ShortcutHelpModel::sameRowIdentities(
    const QList<ShortcutHelpRow> &left, const QList<ShortcutHelpRow> &right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (int index = 0; index < left.size(); ++index) {
        if (left.at(index).actionId != right.at(index).actionId
            || left.at(index).actionName != right.at(index).actionName) {
            return false;
        }
    }

    return true;
}

bool ShortcutHelpModel::sameRowData(const ShortcutHelpRow &left, const ShortcutHelpRow &right)
{
    return left.actionText == right.actionText && left.shortcutText == right.shortcutText
        && left.categoryKey == right.categoryKey && left.categoryText == right.categoryText
        && left.shortcutKeyTexts == right.shortcutKeyTexts
        && left.categoryFirst == right.categoryFirst && left.categoryLast == right.categoryLast;
}
}

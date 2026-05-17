// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "shortcuthelpmodel.h"

#include "applicationshortcutpolicy.h"
#include "kiriviewapplication.h"
#include "kiriviewapplicationactions.h"

#include <KLocalizedString>
#include <KirigamiActionCollection>
#include <QAction>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QVariant>

namespace {
QString actionDefinitionName(const KiriView::ApplicationActions::ActionDefinition &definition)
{
    return QString::fromLatin1(definition.name);
}
}

namespace KiriView::ApplicationActions {
ShortcutHelpModel::ShortcutHelpModel(KiriViewApplication &application, QObject *parent)
    : QAbstractListModel(parent)
    , m_application(application)
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

    const Row &row = m_rows.at(index.row());
    switch (role) {
    case ActionIdRole:
        return row.actionId;
    case ActionNameRole:
        return row.actionName;
    case ActionTextRole:
        return actionDisplayText(row.action);
    case ShortcutTextRole:
        return shortcutDisplayText(row.action);
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
    };
}

void ShortcutHelpModel::handleActionChanged(QAction *changedAction)
{
    const QList<Row> rows = collectRows();
    if (!sameRows(rows, m_rows)) {
        beginResetModel();
        m_rows = rows;
        endResetModel();
        return;
    }

    if (changedAction != nullptr) {
        for (int row = 0; row < m_rows.size(); ++row) {
            if (m_rows.at(row).action == changedAction) {
                Q_EMIT dataChanged(
                    index(row, 0), index(row, 0), { ActionTextRole, ShortcutTextRole });
                return;
            }
        }
    }

    if (!m_rows.isEmpty()) {
        Q_EMIT dataChanged(
            index(0, 0), index(m_rows.size() - 1, 0), { ActionTextRole, ShortcutTextRole });
    }
}

QList<ShortcutHelpModel::Row> ShortcutHelpModel::collectRows() const
{
    QList<Row> rows;
    rows.reserve(static_cast<qsizetype>(definitions().size()));

    for (const ActionDefinition &definition : definitions()) {
        const QString name = actionDefinitionName(definition);
        QAction *action = m_application.action(name);
        if (action == nullptr || !KirigamiActionCollection::isShortcutsConfigurable(action)) {
            continue;
        }

        rows.push_back(Row { action, static_cast<int>(definition.actionId), name });
    }

    return rows;
}

bool ShortcutHelpModel::sameRows(const QList<Row> &left, const QList<Row> &right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (int index = 0; index < left.size(); ++index) {
        if (left.at(index).action != right.at(index).action
            || left.at(index).actionId != right.at(index).actionId
            || left.at(index).actionName != right.at(index).actionName) {
            return false;
        }
    }

    return true;
}

QString ShortcutHelpModel::actionDisplayText(const QAction *action)
{
    if (action == nullptr) {
        return {};
    }

    const QString text = action->text();
    QString displayText;
    displayText.reserve(text.size());
    for (qsizetype index = 0; index < text.size(); ++index) {
        if (text.at(index) != QLatin1Char('&')) {
            displayText.append(text.at(index));
            continue;
        }

        if (index + 1 < text.size() && text.at(index + 1) == QLatin1Char('&')) {
            displayText.append(QLatin1Char('&'));
            ++index;
        }
    }

    return displayText;
}

QString ShortcutHelpModel::shortcutDisplayText(const QAction *action)
{
    if (action == nullptr) {
        return {};
    }

    const QString text = shortcutListText(action->shortcuts());
    return text.isEmpty() ? i18nc("@info:keyboard shortcut", "Unassigned") : text;
}
}

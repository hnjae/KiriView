// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "activenavigationthumbnailmodel.h"

#include <QModelIndex>
#include <cstddef>
#include <utility>

namespace KiriView {
ActiveNavigationThumbnailModel::ActiveNavigationThumbnailModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ActiveNavigationThumbnailModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant ActiveNavigationThumbnailModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || static_cast<std::size_t>(index.row()) >= m_rows.size()) {
        return {};
    }

    const ActiveNavigationThumbnailRow &row = m_rows.at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case NumberRole:
        return row.number;
    case UrlRole:
        return row.url;
    case LabelRole:
        return row.label;
    case IconNameRole:
        return iconName(row.kind);
    case CurrentRole:
        return row.current;
    default:
        return {};
    }
}

QHash<int, QByteArray> ActiveNavigationThumbnailModel::roleNames() const
{
    return {
        { NumberRole, QByteArrayLiteral("number") },
        { UrlRole, QByteArrayLiteral("url") },
        { LabelRole, QByteArrayLiteral("label") },
        { IconNameRole, QByteArrayLiteral("iconName") },
        { CurrentRole, QByteArrayLiteral("current") },
    };
}

void ActiveNavigationThumbnailModel::setRows(std::vector<ActiveNavigationThumbnailRow> rows)
{
    if (sameRows(m_rows, rows)) {
        return;
    }

    if (!sameRowIdentities(m_rows, rows)) {
        beginResetModel();
        m_rows = std::move(rows);
        endResetModel();
        return;
    }

    std::vector<int> changedRows;
    for (std::size_t row = 0; row < m_rows.size(); ++row) {
        if (m_rows.at(row).current != rows.at(row).current) {
            changedRows.push_back(static_cast<int>(row));
        }
    }

    m_rows = std::move(rows);
    for (int row : changedRows) {
        Q_EMIT dataChanged(index(row, 0), index(row, 0), { CurrentRole });
    }
}

void ActiveNavigationThumbnailModel::clear() { setRows({}); }

QString ActiveNavigationThumbnailModel::iconName(ActiveNavigationThumbnailKind kind)
{
    switch (kind) {
    case ActiveNavigationThumbnailKind::Image:
        return QStringLiteral("image-x-generic-symbolic");
    case ActiveNavigationThumbnailKind::Video:
        return QStringLiteral("video-x-generic-symbolic");
    }

    return QStringLiteral("image-x-generic-symbolic");
}

bool ActiveNavigationThumbnailModel::sameRowIdentity(
    const ActiveNavigationThumbnailRow &left, const ActiveNavigationThumbnailRow &right)
{
    return left.number == right.number && left.url == right.url && left.label == right.label
        && left.kind == right.kind;
}

bool ActiveNavigationThumbnailModel::sameRows(const std::vector<ActiveNavigationThumbnailRow> &left,
    const std::vector<ActiveNavigationThumbnailRow> &right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t row = 0; row < left.size(); ++row) {
        if (!sameRowIdentity(left.at(row), right.at(row))
            || left.at(row).current != right.at(row).current) {
            return false;
        }
    }

    return true;
}

bool ActiveNavigationThumbnailModel::sameRowIdentities(
    const std::vector<ActiveNavigationThumbnailRow> &left,
    const std::vector<ActiveNavigationThumbnailRow> &right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t row = 0; row < left.size(); ++row) {
        if (!sameRowIdentity(left.at(row), right.at(row))) {
            return false;
        }
    }

    return true;
}
}

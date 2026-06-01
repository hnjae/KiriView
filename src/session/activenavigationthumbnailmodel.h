// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILMODEL_H
#define KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILMODEL_H

#include "session/activenavigationthumbnailprojection.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVariant>
#include <vector>

namespace KiriView {
class ActiveNavigationThumbnailModel final : public QAbstractListModel
{
public:
    enum Role {
        NumberRole = Qt::UserRole + 1,
        UrlRole,
        LabelRole,
        IconNameRole,
        CurrentRole,
        NavigationGenerationRole,
    };

    explicit ActiveNavigationThumbnailModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(std::vector<ActiveNavigationThumbnailRow> rows);
    void clear();
    bool containsRowIdentity(int number, const QUrl &url, quint64 navigationGeneration) const;

private:
    static QString iconName(ActiveNavigationThumbnailKind kind);
    static bool sameRowIdentity(
        const ActiveNavigationThumbnailRow &left, const ActiveNavigationThumbnailRow &right);
    static bool sameRows(const std::vector<ActiveNavigationThumbnailRow> &left,
        const std::vector<ActiveNavigationThumbnailRow> &right);
    static bool sameRowIdentities(const std::vector<ActiveNavigationThumbnailRow> &left,
        const std::vector<ActiveNavigationThumbnailRow> &right);

    std::vector<ActiveNavigationThumbnailRow> m_rows;
    quint64 m_navigationGeneration = 0;
};
}

#endif

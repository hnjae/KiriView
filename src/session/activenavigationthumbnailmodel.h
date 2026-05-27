// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILMODEL_H
#define KIRIVIEW_ACTIVENAVIGATIONTHUMBNAILMODEL_H

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <vector>

namespace KiriView {
enum class ActiveNavigationThumbnailKind {
    Image,
    Video,
};

struct ActiveNavigationThumbnailRow {
    int number = 0;
    QUrl url;
    QString label;
    ActiveNavigationThumbnailKind kind = ActiveNavigationThumbnailKind::Image;
    bool current = false;
};

class ActiveNavigationThumbnailModel final : public QAbstractListModel
{
public:
    enum Role {
        NumberRole = Qt::UserRole + 1,
        UrlRole,
        LabelRole,
        IconNameRole,
        CurrentRole,
    };

    explicit ActiveNavigationThumbnailModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(std::vector<ActiveNavigationThumbnailRow> rows);
    void clear();

private:
    static QString iconName(ActiveNavigationThumbnailKind kind);
    static bool sameRowIdentity(
        const ActiveNavigationThumbnailRow &left, const ActiveNavigationThumbnailRow &right);
    static bool sameRows(const std::vector<ActiveNavigationThumbnailRow> &left,
        const std::vector<ActiveNavigationThumbnailRow> &right);
    static bool sameRowIdentities(const std::vector<ActiveNavigationThumbnailRow> &left,
        const std::vector<ActiveNavigationThumbnailRow> &right);

    std::vector<ActiveNavigationThumbnailRow> m_rows;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIMEDIAINFORMATION_H
#define KIRIVIEW_KIRIMEDIAINFORMATION_H

#include <QAbstractListModel>
#include <QObject>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <vector>

class KiriDocumentSession;

class KiriMediaInformationRowModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        ValueRole,
    };
    Q_ENUM(Role)

    struct Row {
        QString label;
        QString value;
    };

    explicit KiriMediaInformationRowModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(std::vector<Row> rows);

private:
    std::vector<Row> m_rows;
};

class KiriMediaInformation : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
    Q_PROPERTY(QString mediaSectionTitle READ mediaSectionTitle NOTIFY changed)
    Q_PROPERTY(bool hasCameraSection READ hasCameraSection NOTIFY changed)
    Q_PROPERTY(QAbstractListModel *generalRows READ generalRows CONSTANT)
    Q_PROPERTY(QAbstractListModel *mediaRows READ mediaRows CONSTANT)
    Q_PROPERTY(QAbstractListModel *cameraRows READ cameraRows CONSTANT)
    Q_PROPERTY(bool canCopyFilePath READ canCopyFilePath NOTIFY changed)
    Q_PROPERTY(bool canOpenContainingFolder READ canOpenContainingFolder NOTIFY changed)

public:
    enum class MediaKind {
        Empty,
        Image,
        Video,
    };

    explicit KiriMediaInformation(KiriDocumentSession &session, QObject *parent = nullptr);

    bool available() const;
    QString title() const;
    QString summary() const;
    QString mediaSectionTitle() const;
    bool hasCameraSection() const;
    QAbstractListModel *generalRows();
    QAbstractListModel *mediaRows();
    QAbstractListModel *cameraRows();
    bool canCopyFilePath() const;
    bool canOpenContainingFolder() const;

    Q_INVOKABLE void copyFilePath();
    Q_INVOKABLE void openContainingFolder();

Q_SIGNALS:
    void changed();

private:
    struct Snapshot {
        bool available = false;
        MediaKind kind = MediaKind::Empty;
        QUrl targetUrl;
        QString title;
        QString summary;
        QString mediaSectionTitle;
        std::vector<KiriMediaInformationRowModel::Row> generalRows;
        std::vector<KiriMediaInformationRowModel::Row> mediaRows;
        std::vector<KiriMediaInformationRowModel::Row> cameraRows;
    };

    void refresh();
    Snapshot buildSnapshot() const;
    QUrl targetUrl() const;
    QString copiedFilePath() const;

    KiriDocumentSession &m_session;
    KiriMediaInformationRowModel m_generalRows;
    KiriMediaInformationRowModel m_mediaRows;
    KiriMediaInformationRowModel m_cameraRows;
    bool m_available = false;
    QUrl m_targetUrl;
    QString m_title;
    QString m_summary;
    QString m_mediaSectionTitle;
};

#endif

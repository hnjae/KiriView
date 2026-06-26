// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kirimediainformation.h"

#include "facade/kiridocumentsession.h"

#include <KIO/OpenFileManagerWindowJob>
#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QModelIndex>
#include <utility>

KiriMediaInformationRowModel::KiriMediaInformationRowModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int KiriMediaInformationRowModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_rows.size());
}

QVariant KiriMediaInformationRowModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || static_cast<std::size_t>(index.row()) >= m_rows.size()) {
        return {};
    }

    const Row& row = m_rows.at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case LabelRole:
        return row.label;
    case ValueRole:
        return row.value;
    default:
        break;
    }

    return {};
}

QHash<int, QByteArray> KiriMediaInformationRowModel::roleNames() const
{
    return {
        { LabelRole, QByteArrayLiteral("label") },
        { ValueRole, QByteArrayLiteral("value") },
    };
}

void KiriMediaInformationRowModel::setRows(std::vector<Row> rows)
{
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}

KiriMediaInformation::KiriMediaInformation(KiriDocumentSession& session, QObject* parent)
    : QObject(parent)
    , m_session(session)
    , m_generalRows(this)
    , m_mediaRows(this)
    , m_cameraRows(this)
    , m_advancedRows(this)
{
    connect(&m_session, &KiriDocumentSession::publicProjectionRevisionChanged, this,
        &KiriMediaInformation::refresh);

    refresh();
}

bool KiriMediaInformation::available() const { return m_available; }

quint64 KiriMediaInformation::revision() const { return m_revision; }

QString KiriMediaInformation::title() const { return m_title; }

QString KiriMediaInformation::summary() const { return m_summary; }

QString KiriMediaInformation::mediaSectionTitle() const { return m_mediaSectionTitle; }

bool KiriMediaInformation::hasCameraSection() const { return m_cameraRows.rowCount() > 0; }

bool KiriMediaInformation::hasAdvancedSection() const { return m_advancedRows.rowCount() > 0; }

QAbstractListModel* KiriMediaInformation::generalRows() { return &m_generalRows; }

QAbstractListModel* KiriMediaInformation::mediaRows() { return &m_mediaRows; }

QAbstractListModel* KiriMediaInformation::cameraRows() { return &m_cameraRows; }

QAbstractListModel* KiriMediaInformation::advancedRows() { return &m_advancedRows; }

bool KiriMediaInformation::canCopyFilePath() const { return m_canCopyFilePath; }

bool KiriMediaInformation::canOpenContainingFolder() const { return m_canOpenContainingFolder; }

void KiriMediaInformation::copyFilePath()
{
    if (!canCopyFilePath()) {
        return;
    }

    if (qobject_cast<QGuiApplication*>(QCoreApplication::instance()) == nullptr) {
        return;
    }

    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard != nullptr) {
        clipboard->setText(copiedFilePath());
    }
}

void KiriMediaInformation::openContainingFolder()
{
    if (!canOpenContainingFolder()) {
        return;
    }

    auto* job = KIO::highlightInFileManager(QList<QUrl> { m_targetUrl });
    if (job != nullptr) {
        job->setParent(this);
    }
}

void KiriMediaInformation::refresh()
{
    const kiriview::MediaInformationProjectionSnapshot& snapshot
        = m_session.mediaInformationSnapshot();
    m_revision = snapshot.revision;
    m_available = snapshot.available;
    m_targetUrl = snapshot.targetUrl;
    m_title = snapshot.title;
    m_summary = snapshot.summary;
    m_mediaSectionTitle = snapshot.mediaSectionTitle;
    m_canCopyFilePath = snapshot.canCopyFilePath;
    m_canOpenContainingFolder = snapshot.canOpenContainingFolder;
    m_generalRows.setRows(snapshot.generalRows);
    m_mediaRows.setRows(snapshot.mediaRows);
    m_cameraRows.setRows(snapshot.cameraRows);
    m_advancedRows.setRows(snapshot.advancedRows);

    Q_EMIT changed();
}

QUrl KiriMediaInformation::targetUrl() const { return m_targetUrl; }

QString KiriMediaInformation::copiedFilePath() const
{
    return kiriview::mediaInformationDisplayPathForUrl(m_targetUrl);
}

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kirimediainformation.h"

#include "archive/archiveformat.h"
#include "facade/kiridocumentsession.h"
#include "metadata/embeddedmetadata.h"

#include <KIO/OpenFileManagerWindowJob>
#include <KLocalizedString>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QModelIndex>
#include <utility>

namespace {
QString displayPathForUrl(const QUrl &url)
{
    if (url.isLocalFile()) {
        return QDir::toNativeSeparators(url.toLocalFile());
    }

    return url.toDisplayString(QUrl::PreferLocalFile);
}

QString fileNameForUrl(const QUrl &url)
{
    QString fileName = url.fileName(QUrl::PrettyDecoded);
    if (!fileName.isEmpty()) {
        return fileName;
    }

    return displayPathForUrl(url);
}

bool hasValidDimensions(const QSize &size) { return size.width() > 0 && size.height() > 0; }

QString dimensionsText(const QSize &size)
{
    if (!hasValidDimensions(size)) {
        return {};
    }

    return QStringLiteral("%1×%2 px").arg(size.width()).arg(size.height());
}

void appendRowIfValue(
    std::vector<KiriMediaInformationRowModel::Row> &rows, QString label, const QString &value)
{
    if (!value.isEmpty()) {
        rows.push_back({ std::move(label), value });
    }
}

bool openedCollectionScopeInformationAvailable(const KiriView::OpenedCollectionScopeLocation &scope)
{
    if (scope.isEmpty() || scope.isDirectory()) {
        return true;
    }

    return KiriView::archiveRootSchemeUsesKioFuse(scope.rootUrl().scheme());
}

QUrl imageInformationTargetUrl(const KiriDocumentSession &session)
{
    if (session.documentKind() != KiriDocumentSession::DocumentKind::Image
        || session.imageDocument()->status() != KiriImageDocument::Status::Ready) {
        return {};
    }

    const KiriView::OpenedCollectionScopeLocation openedCollectionScope
        = session.imageDocument()->displayedOpenedCollectionScope();
    if (!openedCollectionScopeInformationAvailable(openedCollectionScope)) {
        return {};
    }

    return session.imageDocument()->displayedUrl();
}

QUrl videoInformationTargetUrl(const KiriDocumentSession &session)
{
    if (session.documentKind() != KiriDocumentSession::DocumentKind::Video) {
        return {};
    }

    return session.videoDocument()->sourceUrl();
}

QUrl informationTargetUrl(const KiriDocumentSession &session)
{
    switch (session.documentKind()) {
    case KiriDocumentSession::DocumentKind::Image:
        return imageInformationTargetUrl(session);
    case KiriDocumentSession::DocumentKind::Video:
        return videoInformationTargetUrl(session);
    case KiriDocumentSession::DocumentKind::Empty:
        break;
    }

    return {};
}

bool canOpenContainingLocation(const QUrl &url) { return !url.isEmpty() && !url.path().isEmpty(); }

std::vector<KiriMediaInformationRowModel::Row> generalRows(
    KiriMediaInformation::MediaKind kind, const QUrl &targetUrl)
{
    QString typeValue;
    switch (kind) {
    case KiriMediaInformation::MediaKind::Image:
        typeValue = i18nc("@info:metadata value", "Image");
        break;
    case KiriMediaInformation::MediaKind::Video:
        typeValue = i18nc("@info:metadata value", "Video");
        break;
    case KiriMediaInformation::MediaKind::Empty:
        break;
    }

    std::vector<KiriMediaInformationRowModel::Row> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Type"), typeValue);
    appendRowIfValue(rows, i18nc("@label:metadata", "Path"), displayPathForUrl(targetUrl));
    return rows;
}

std::vector<KiriMediaInformationRowModel::Row> imageRows(const QSize &size)
{
    std::vector<KiriMediaInformationRowModel::Row> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Dimensions"), dimensionsText(size));
    return rows;
}

std::vector<KiriMediaInformationRowModel::Row> videoRows(
    const QSize &size, const KiriView::EmbeddedMetadata &metadata)
{
    std::vector<KiriMediaInformationRowModel::Row> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Duration"), metadata.duration);
    appendRowIfValue(rows, i18nc("@label:metadata", "Frame Size"),
        !metadata.frameSize.isEmpty() ? metadata.frameSize : dimensionsText(size));
    return rows;
}

QString cameraText(const KiriView::EmbeddedMetadata &metadata)
{
    if (!metadata.cameraMake.isEmpty() && !metadata.cameraModel.isEmpty()) {
        return QStringLiteral("%1 %2").arg(metadata.cameraMake, metadata.cameraModel);
    }
    if (!metadata.cameraMake.isEmpty()) {
        return metadata.cameraMake;
    }
    return metadata.cameraModel;
}

std::vector<KiriMediaInformationRowModel::Row> cameraRows(
    const KiriView::EmbeddedMetadata &metadata)
{
    std::vector<KiriMediaInformationRowModel::Row> rows;
    appendRowIfValue(rows, i18nc("@label:metadata", "Camera"), cameraText(metadata));
    appendRowIfValue(rows, i18nc("@label:metadata", "Taken"), metadata.taken);
    appendRowIfValue(rows, i18nc("@label:metadata", "Location"), metadata.location);
    appendRowIfValue(rows, i18nc("@label:metadata", "Lens"), metadata.lens);
    appendRowIfValue(rows, i18nc("@label:metadata", "Exposure"), metadata.exposure);
    appendRowIfValue(rows, i18nc("@label:metadata", "ISO"), metadata.iso);
    appendRowIfValue(rows, i18nc("@label:metadata", "Focal Length"), metadata.focalLength);
    appendRowIfValue(rows, i18nc("@label:metadata", "Software"), metadata.software);
    return rows;
}

std::vector<KiriMediaInformationRowModel::Row> advancedRows(
    const KiriView::EmbeddedMetadata &metadata)
{
    std::vector<KiriMediaInformationRowModel::Row> rows;
    rows.reserve(metadata.advancedRows.size());
    for (const KiriView::EmbeddedMetadataRow &row : metadata.advancedRows) {
        appendRowIfValue(rows, row.label, row.value);
    }
    return rows;
}
}

KiriMediaInformationRowModel::KiriMediaInformationRowModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int KiriMediaInformationRowModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_rows.size());
}

QVariant KiriMediaInformationRowModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || static_cast<std::size_t>(index.row()) >= m_rows.size()) {
        return {};
    }

    const Row &row = m_rows.at(static_cast<std::size_t>(index.row()));
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

KiriMediaInformation::KiriMediaInformation(KiriDocumentSession &session, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_generalRows(this)
    , m_mediaRows(this)
    , m_cameraRows(this)
    , m_advancedRows(this)
{
    connect(
        &m_session, &KiriDocumentSession::sourceUrlChanged, this, &KiriMediaInformation::refresh);
    connect(&m_session, &KiriDocumentSession::documentKindChanged, this,
        &KiriMediaInformation::refresh);
    connect(&m_session, &KiriDocumentSession::windowTitleSubjectChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.imageDocument(), &KiriImageDocument::statusChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.imageDocument(), &KiriImageDocument::displayedUrlChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.imageDocument(), &KiriImageDocument::imageSizeChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.imageDocument(), &KiriImageDocument::imageDocumentSourceScopeChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.imageDocument(), &KiriImageDocument::embeddedMetadataChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.videoDocument(), &KiriVideoDocument::sourceUrlChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.videoDocument(), &KiriVideoDocument::videoSizeChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.videoDocument(), &KiriVideoDocument::statusChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.videoDocument(), &KiriVideoDocument::embeddedMetadataChanged, this,
        &KiriMediaInformation::refresh);

    refresh();
}

bool KiriMediaInformation::available() const { return m_available; }

QString KiriMediaInformation::title() const { return m_title; }

QString KiriMediaInformation::summary() const { return m_summary; }

QString KiriMediaInformation::mediaSectionTitle() const { return m_mediaSectionTitle; }

bool KiriMediaInformation::hasCameraSection() const { return m_cameraRows.rowCount() > 0; }

bool KiriMediaInformation::hasAdvancedSection() const { return m_advancedRows.rowCount() > 0; }

QAbstractListModel *KiriMediaInformation::generalRows() { return &m_generalRows; }

QAbstractListModel *KiriMediaInformation::mediaRows() { return &m_mediaRows; }

QAbstractListModel *KiriMediaInformation::cameraRows() { return &m_cameraRows; }

QAbstractListModel *KiriMediaInformation::advancedRows() { return &m_advancedRows; }

bool KiriMediaInformation::canCopyFilePath() const { return !m_targetUrl.isEmpty(); }

bool KiriMediaInformation::canOpenContainingFolder() const
{
    return canOpenContainingLocation(m_targetUrl);
}

void KiriMediaInformation::copyFilePath()
{
    if (!canCopyFilePath()) {
        return;
    }

    if (qobject_cast<QGuiApplication *>(QCoreApplication::instance()) == nullptr) {
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard != nullptr) {
        clipboard->setText(copiedFilePath());
    }
}

void KiriMediaInformation::openContainingFolder()
{
    if (!canOpenContainingFolder()) {
        return;
    }

    auto *job = KIO::highlightInFileManager(QList<QUrl> { m_targetUrl });
    if (job != nullptr) {
        job->setParent(this);
    }
}

void KiriMediaInformation::refresh()
{
    const Snapshot snapshot = buildSnapshot();
    m_available = snapshot.available;
    m_targetUrl = snapshot.targetUrl;
    m_title = snapshot.title;
    m_summary = snapshot.summary;
    m_mediaSectionTitle = snapshot.mediaSectionTitle;
    m_generalRows.setRows(snapshot.generalRows);
    m_mediaRows.setRows(snapshot.mediaRows);
    m_cameraRows.setRows(snapshot.cameraRows);
    m_advancedRows.setRows(snapshot.advancedRows);

    Q_EMIT changed();
}

KiriMediaInformation::Snapshot KiriMediaInformation::buildSnapshot() const
{
    Snapshot snapshot;
    snapshot.targetUrl = informationTargetUrl(m_session);
    if (snapshot.targetUrl.isEmpty()) {
        snapshot.summary = i18nc("@info:metadata unavailable", "No media selected");
        snapshot.mediaSectionTitle = i18nc("@title:group", "Media");
        return snapshot;
    }

    snapshot.available = true;
    snapshot.title = fileNameForUrl(snapshot.targetUrl);
    snapshot.generalRows = ::generalRows(snapshot.kind, snapshot.targetUrl);

    switch (m_session.documentKind()) {
    case KiriDocumentSession::DocumentKind::Image: {
        const QSize imageSize = m_session.imageDocument()->primaryImageSize();
        snapshot.kind = MediaKind::Image;
        snapshot.mediaSectionTitle = i18nc("@title:group", "Image");
        snapshot.summary = hasValidDimensions(imageSize)
            ? i18nc("@info:metadata summary", "Image, %1", dimensionsText(imageSize))
            : i18nc("@info:metadata summary", "Image");
        snapshot.generalRows = ::generalRows(snapshot.kind, snapshot.targetUrl);
        snapshot.mediaRows = imageRows(imageSize);
        const KiriView::EmbeddedMetadata &metadata = m_session.imageDocument()->embeddedMetadata();
        snapshot.cameraRows = ::cameraRows(metadata);
        snapshot.advancedRows = ::advancedRows(metadata);
        break;
    }
    case KiriDocumentSession::DocumentKind::Video: {
        const QSize videoSize = m_session.videoDocument()->videoSize();
        const KiriView::EmbeddedMetadata &metadata = m_session.videoDocument()->embeddedMetadata();
        snapshot.kind = MediaKind::Video;
        snapshot.mediaSectionTitle = i18nc("@title:group", "Video");
        snapshot.summary = hasValidDimensions(videoSize)
            ? i18nc("@info:metadata summary", "Video, %1", dimensionsText(videoSize))
            : i18nc("@info:metadata summary", "Video");
        snapshot.generalRows = ::generalRows(snapshot.kind, snapshot.targetUrl);
        snapshot.mediaRows = videoRows(videoSize, metadata);
        snapshot.cameraRows = ::cameraRows(metadata);
        snapshot.advancedRows = ::advancedRows(metadata);
        break;
    }
    case KiriDocumentSession::DocumentKind::Empty:
        break;
    }

    return snapshot;
}

QUrl KiriMediaInformation::targetUrl() const { return m_targetUrl; }

QString KiriMediaInformation::copiedFilePath() const { return displayPathForUrl(m_targetUrl); }

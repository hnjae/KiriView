// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kirimediainformation.h"

#include "archive/archiveformat.h"
#include "facade/kiridocumentsession.h"

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
        return i18nc("@info:metadata placeholder value", "Unknown dimensions (placeholder)");
    }

    return QStringLiteral("%1×%2 px").arg(size.width()).arg(size.height());
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
        typeValue = i18nc("@info:metadata placeholder value", "Image file (placeholder)");
        break;
    case KiriMediaInformation::MediaKind::Video:
        typeValue = i18nc("@info:metadata placeholder value", "Video file (placeholder)");
        break;
    case KiriMediaInformation::MediaKind::Empty:
        typeValue = i18nc("@info:metadata placeholder value", "Unknown type (placeholder)");
        break;
    }

    return {
        { i18nc("@label:metadata", "Type"), typeValue },
        { i18nc("@label:metadata", "File Size"),
            i18nc("@info:metadata placeholder value", "Unknown size (placeholder)") },
        { i18nc("@label:metadata", "Modified"),
            i18nc("@info:metadata placeholder value", "Unknown date (placeholder)") },
        { i18nc("@label:metadata", "Path"), displayPathForUrl(targetUrl) },
    };
}

std::vector<KiriMediaInformationRowModel::Row> imageRows(const QSize &size)
{
    return {
        { i18nc("@label:metadata", "Dimensions"), dimensionsText(size) },
        { i18nc("@label:metadata", "Color Space"),
            i18nc("@info:metadata placeholder value", "sRGB (placeholder)") },
        { i18nc("@label:metadata", "Bit Depth"),
            i18nc("@info:metadata placeholder value", "8-bit (placeholder)") },
    };
}

std::vector<KiriMediaInformationRowModel::Row> videoRows(const QSize &size)
{
    return {
        { i18nc("@label:metadata", "Frame Size"), dimensionsText(size) },
        { i18nc("@label:metadata", "Color Space"),
            i18nc("@info:metadata placeholder value", "BT.709 (placeholder)") },
        { i18nc("@label:metadata", "Bit Depth"),
            i18nc("@info:metadata placeholder value", "8-bit (placeholder)") },
    };
}

std::vector<KiriMediaInformationRowModel::Row> cameraRows()
{
    return {
        { i18nc("@label:metadata", "Camera"),
            i18nc("@info:metadata placeholder value", "KiriView Sample Camera (placeholder)") },
        { i18nc("@label:metadata", "Lens"),
            i18nc("@info:metadata placeholder value", "35 mm (placeholder)") },
        { i18nc("@label:metadata", "Exposure"),
            i18nc("@info:metadata placeholder value", "1/125 s at f/5.6 (placeholder)") },
    };
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
    connect(m_session.videoDocument(), &KiriVideoDocument::sourceUrlChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.videoDocument(), &KiriVideoDocument::videoSizeChanged, this,
        &KiriMediaInformation::refresh);
    connect(m_session.videoDocument(), &KiriVideoDocument::statusChanged, this,
        &KiriMediaInformation::refresh);

    refresh();
}

bool KiriMediaInformation::available() const { return m_available; }

QString KiriMediaInformation::title() const { return m_title; }

QString KiriMediaInformation::summary() const { return m_summary; }

QString KiriMediaInformation::mediaSectionTitle() const { return m_mediaSectionTitle; }

bool KiriMediaInformation::hasCameraSection() const { return m_cameraRows.rowCount() > 0; }

QAbstractListModel *KiriMediaInformation::generalRows() { return &m_generalRows; }

QAbstractListModel *KiriMediaInformation::mediaRows() { return &m_mediaRows; }

QAbstractListModel *KiriMediaInformation::cameraRows() { return &m_cameraRows; }

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
            : i18nc("@info:metadata summary", "Image metadata placeholder");
        snapshot.generalRows = ::generalRows(snapshot.kind, snapshot.targetUrl);
        snapshot.mediaRows = imageRows(imageSize);
        snapshot.cameraRows = ::cameraRows();
        break;
    }
    case KiriDocumentSession::DocumentKind::Video: {
        const QSize videoSize = m_session.videoDocument()->videoSize();
        snapshot.kind = MediaKind::Video;
        snapshot.mediaSectionTitle = i18nc("@title:group", "Video");
        snapshot.summary = hasValidDimensions(videoSize)
            ? i18nc("@info:metadata summary", "Video, %1", dimensionsText(videoSize))
            : i18nc("@info:metadata summary", "Video metadata placeholder");
        snapshot.generalRows = ::generalRows(snapshot.kind, snapshot.targetUrl);
        snapshot.mediaRows = videoRows(videoSize);
        break;
    }
    case KiriDocumentSession::DocumentKind::Empty:
        break;
    }

    return snapshot;
}

QUrl KiriMediaInformation::targetUrl() const { return m_targetUrl; }

QString KiriMediaInformation::copiedFilePath() const { return displayPathForUrl(m_targetUrl); }

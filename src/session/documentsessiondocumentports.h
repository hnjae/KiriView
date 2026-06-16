// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONDOCUMENTPORTS_H
#define KIRIVIEW_DOCUMENTSESSIONDOCUMENTPORTS_H

#include "location/imagelocation.h"
#include "metadata/embeddedmetadata.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "predecode/predecodedimage.h"
#include "rendering/staticimage.h"
#include "session/activenavigationprojection.h"
#include "system/filedeletion.h"

#include <QMetaObject>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <optional>
#include <vector>

class QObject;

namespace kiriview {
using DocumentSessionDocumentChangeHandler = std::function<void()>;
using DocumentSessionDocumentSignalConnector = std::function<std::vector<QMetaObject::Connection>(
    QObject *, DocumentSessionDocumentChangeHandler)>;

struct DocumentSessionImageDocumentSnapshot {
    QUrl sourceUrl;
    QString errorString;
    QString windowTitleFileName;
    QUrl displayedUrl;
    OpenedCollectionScopeLocation displayedOpenedCollectionScope;
    QSize primaryImageSize;
    bool ready = false;
    bool error = false;
    bool unsupportedOpenedCollectionVideo = false;
    bool fileDeletionInProgress = false;
    bool openedCollectionScopeActive = false;
    bool ordinaryDirectMediaScopeActive = false;
    bool containerNavigationAvailable = false;
    bool twoPageModeEnabled = false;
    bool twoPageModeAvailable = false;
    bool rightToLeftReadingEnabled = false;
    bool rightToLeftReadingAvailable = false;
    bool fitModeSelected = false;
    bool fitHeightModeSelected = false;
    bool fitWidthModeSelected = false;
    bool zoomPercentKnown = false;
    qreal zoomPercent = 0.0;
    EmbeddedMetadata embeddedMetadata;
    ImageDocumentPageNavigationSnapshot pageNavigationSnapshot;
    ImageDocumentPageActiveNavigationSnapshot activeNavigationSnapshot;
    std::optional<DisplayedPredecodeImage> primaryDisplayedPredecodeImage;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext;
};

struct DocumentSessionImageDocumentSnapshotPort {
    std::function<DocumentSessionImageDocumentSnapshot()> snapshot;
    DocumentSessionDocumentSignalConnector snapshotChanged;
};

struct DocumentSessionImageDocumentCommandPort {
    std::function<void(const QUrl &)> setSourceUrl;
    std::function<void()> openPreviousPage;
    std::function<void()> openNextPage;
    std::function<void(int)> openImageAtPage;
    std::function<void(FileDeletionMode)> deleteDisplayedFile;
};

struct DocumentSessionVideoDocumentSnapshot {
    QUrl sourceUrl;
    QString errorString;
    QString windowTitleFileName;
    QSize videoSize;
    bool ready = false;
    bool error = false;
    bool hasVideo = false;
    bool zoomPercentKnown = false;
    int zoomPercent = 0;
    EmbeddedMetadata embeddedMetadata;
};

struct DocumentSessionVideoDocumentSnapshotPort {
    std::function<DocumentSessionVideoDocumentSnapshot()> snapshot;
    DocumentSessionDocumentSignalConnector snapshotChanged;
};

struct DocumentSessionVideoDocumentCommandPort {
    std::function<void(const QUrl &)> setSourceUrl;
    std::function<QObject *()> videoOutput;
    std::function<void()> stop;
    std::function<void(QObject *)> setVideoOutput;
    std::function<void(const QRectF &, const QRectF &)> setVideoOutputGeometry;
};
}

#endif

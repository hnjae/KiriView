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
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <optional>

class QObject;

namespace KiriView {
using DocumentSessionDocumentChangeHandler = std::function<void()>;
using DocumentSessionDocumentSignalConnector
    = std::function<QMetaObject::Connection(QObject *, DocumentSessionDocumentChangeHandler)>;

struct DocumentSessionImageDocumentSignals {
    DocumentSessionDocumentSignalConnector sourceUrlChanged;
    DocumentSessionDocumentSignalConnector statusChanged;
    DocumentSessionDocumentSignalConnector windowTitleFileNameChanged;
    DocumentSessionDocumentSignalConnector imageSizeChanged;
    DocumentSessionDocumentSignalConnector errorStringChanged;
    DocumentSessionDocumentSignalConnector imageDocumentSourceScopeChanged;
    DocumentSessionDocumentSignalConnector unsupportedOpenedCollectionVideoChanged;
    DocumentSessionDocumentSignalConnector fileDeletionInProgressChanged;
    DocumentSessionDocumentSignalConnector zoomPercentKnownChanged;
    DocumentSessionDocumentSignalConnector zoomPercentChanged;
    DocumentSessionDocumentSignalConnector zoomModeChanged;
    DocumentSessionDocumentSignalConnector pageNavigationChanged;
    DocumentSessionDocumentSignalConnector containerNavigationChanged;
    DocumentSessionDocumentSignalConnector twoPageModeChanged;
    DocumentSessionDocumentSignalConnector rightToLeftReadingChanged;
    DocumentSessionDocumentSignalConnector embeddedMetadataChanged;
};

struct DocumentSessionImageDocumentPort {
    std::function<QUrl()> sourceUrl;
    std::function<void(const QUrl &)> setSourceUrl;
    std::function<QString()> errorString;
    std::function<QString()> windowTitleFileName;
    std::function<QUrl()> displayedUrl;
    std::function<OpenedCollectionScopeLocation()> displayedOpenedCollectionScope;
    std::function<QSize()> primaryImageSize;
    std::function<bool()> ready;
    std::function<bool()> error;
    std::function<bool()> unsupportedOpenedCollectionVideo;
    std::function<bool()> fileDeletionInProgress;
    std::function<bool()> ordinaryDirectMediaScopeActive;
    std::function<bool()> containerNavigationAvailable;
    std::function<bool()> twoPageModeEnabled;
    std::function<bool()> twoPageModeAvailable;
    std::function<bool()> rightToLeftReadingEnabled;
    std::function<bool()> rightToLeftReadingAvailable;
    std::function<bool()> fitModeSelected;
    std::function<bool()> fitHeightModeSelected;
    std::function<bool()> fitWidthModeSelected;
    std::function<bool()> zoomPercentKnown;
    std::function<qreal()> zoomPercent;
    std::function<EmbeddedMetadata()> embeddedMetadata;
    std::function<ImageDocumentPageNavigationSnapshot()> pageNavigationSnapshot;
    std::function<ImageDocumentPageActiveNavigationSnapshot()> activeNavigationSnapshot;
    std::function<std::optional<DisplayedPredecodeImage>()> primaryDisplayedPredecodeImage;
    std::function<ImageFirstDisplayDecodeContext()> firstDisplayDecodeContext;
    std::function<void()> openPreviousPage;
    std::function<void()> openNextPage;
    std::function<void(int)> openImageAtPage;
    std::function<void(FileDeletionMode)> deleteDisplayedFile;
    DocumentSessionImageDocumentSignals notifications;
};

struct DocumentSessionVideoDocumentSignals {
    DocumentSessionDocumentSignalConnector sourceUrlChanged;
    DocumentSessionDocumentSignalConnector statusChanged;
    DocumentSessionDocumentSignalConnector windowTitleFileNameChanged;
    DocumentSessionDocumentSignalConnector videoSizeChanged;
    DocumentSessionDocumentSignalConnector errorStringChanged;
    DocumentSessionDocumentSignalConnector zoomPercentKnownChanged;
    DocumentSessionDocumentSignalConnector zoomPercentChanged;
    DocumentSessionDocumentSignalConnector embeddedMetadataChanged;
};

struct DocumentSessionVideoDocumentPort {
    std::function<QUrl()> sourceUrl;
    std::function<void(const QUrl &)> setSourceUrl;
    std::function<QString()> errorString;
    std::function<QString()> windowTitleFileName;
    std::function<QSize()> videoSize;
    std::function<bool()> ready;
    std::function<bool()> error;
    std::function<bool()> zoomPercentKnown;
    std::function<int()> zoomPercent;
    std::function<EmbeddedMetadata()> embeddedMetadata;
    std::function<QObject *()> videoOutput;
    std::function<void()> stop;
    std::function<void(QObject *)> setVideoOutput;
    DocumentSessionVideoDocumentSignals notifications;
};
}

#endif

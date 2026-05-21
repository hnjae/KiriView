// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIDOCUMENTSESSION_H
#define KIRIVIEW_KIRIDOCUMENTSESSION_H

#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "session/documentsessionruntime.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <vector>

class KiriDocumentSession : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(DocumentKind documentKind READ documentKind NOTIFY documentKindChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(
        QString windowTitleFileName READ windowTitleFileName NOTIFY windowTitleFileNameChanged)
    Q_PROPERTY(QStringList openDialogNameFilters READ openDialogNameFilters CONSTANT)
    Q_PROPERTY(bool displayedFileDeletionAvailable READ displayedFileDeletionAvailable NOTIFY
            displayedFileDeletionAvailabilityChanged)
    Q_PROPERTY(bool fileDeletionInProgress READ fileDeletionInProgress NOTIFY
            fileDeletionInProgressChanged)
    Q_PROPERTY(bool mediaNavigationActive READ mediaNavigationActive NOTIFY
            mediaNavigationAvailabilityChanged)
    Q_PROPERTY(bool canOpenPreviousMedia READ canOpenPreviousMedia NOTIFY
            mediaNavigationAvailabilityChanged)
    Q_PROPERTY(
        bool canOpenNextMedia READ canOpenNextMedia NOTIFY mediaNavigationAvailabilityChanged)
    Q_PROPERTY(
        bool atKnownFirstMedia READ atKnownFirstMedia NOTIFY mediaNavigationAvailabilityChanged)
    Q_PROPERTY(
        bool atKnownLastMedia READ atKnownLastMedia NOTIFY mediaNavigationAvailabilityChanged)
    Q_PROPERTY(KiriImageDocument *imageDocument READ imageDocument CONSTANT)
    Q_PROPERTY(KiriVideoDocument *videoDocument READ videoDocument CONSTANT)

public:
    enum class DocumentKind {
        Empty,
        Image,
        Video,
    };
    Q_ENUM(DocumentKind)

    enum class DeletionMode {
        MoveToTrash,
        DeletePermanently,
    };
    Q_ENUM(DeletionMode)

    explicit KiriDocumentSession(QObject *parent = nullptr);
    explicit KiriDocumentSession(
        KiriView::DocumentSessionRuntimeDependencies dependencies, QObject *parent = nullptr);
    ~KiriDocumentSession() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    DocumentKind documentKind() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    QStringList openDialogNameFilters() const;
    bool displayedFileDeletionAvailable() const;
    bool fileDeletionInProgress() const;
    bool mediaNavigationActive() const;
    bool canOpenPreviousMedia() const;
    bool canOpenNextMedia() const;
    bool atKnownFirstMedia() const;
    bool atKnownLastMedia() const;
    KiriImageDocument *imageDocument() const;
    KiriVideoDocument *videoDocument() const;

    Q_INVOKABLE void openPreviousMedia();
    Q_INVOKABLE void openNextMedia();
    Q_INVOKABLE void deleteDisplayedFile(KiriDocumentSession::DeletionMode mode);

Q_SIGNALS:
    void sourceUrlChanged();
    void documentKindChanged();
    void errorStringChanged();
    void windowTitleFileNameChanged();
    void displayedFileDeletionAvailabilityChanged();
    void fileDeletionInProgressChanged();
    void mediaNavigationAvailabilityChanged();

private:
    void handleSessionChanges(const std::vector<KiriView::DocumentSessionChange> &changes);

    std::unique_ptr<KiriImageDocument> m_imageDocument;
    std::unique_ptr<KiriVideoDocument> m_videoDocument;
    std::unique_ptr<KiriView::DocumentSessionRuntime> m_runtime;
};

#endif

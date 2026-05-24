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
    Q_PROPERTY(QString windowTitleSubject READ windowTitleSubject NOTIFY windowTitleSubjectChanged)
    Q_PROPERTY(QStringList openDialogNameFilters READ openDialogNameFilters CONSTANT)
    Q_PROPERTY(bool displayedFileDeletionAvailable READ displayedFileDeletionAvailable NOTIFY
            displayedFileDeletionAvailabilityChanged)
    Q_PROPERTY(bool fileDeletionInProgress READ fileDeletionInProgress NOTIFY
            fileDeletionInProgressChanged)
    Q_PROPERTY(bool activeZoomPercentAvailable READ activeZoomPercentAvailable NOTIFY
            activeZoomReadoutChanged)
    Q_PROPERTY(
        bool activeZoomPercentKnown READ activeZoomPercentKnown NOTIFY activeZoomReadoutChanged)
    Q_PROPERTY(double activeZoomPercent READ activeZoomPercent NOTIFY activeZoomReadoutChanged)
    Q_PROPERTY(bool activeZoomEditable READ activeZoomEditable NOTIFY activeZoomReadoutChanged)
    Q_PROPERTY(bool activeNavigationAvailable READ activeNavigationAvailable NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool activeNavigationKnown READ activeNavigationKnown NOTIFY activeNavigationChanged)
    Q_PROPERTY(
        bool activeNavigationEditable READ activeNavigationEditable NOTIFY activeNavigationChanged)
    Q_PROPERTY(int activeNavigationCurrentNumber READ activeNavigationCurrentNumber NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(int activeNavigationCount READ activeNavigationCount NOTIFY activeNavigationChanged)
    Q_PROPERTY(bool canOpenPreviousActiveNavigation READ canOpenPreviousActiveNavigation NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool canOpenNextActiveNavigation READ canOpenNextActiveNavigation NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool atKnownFirstActiveNavigation READ atKnownFirstActiveNavigation NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(bool atKnownLastActiveNavigation READ atKnownLastActiveNavigation NOTIFY
            activeNavigationChanged)
    Q_PROPERTY(ActiveNavigationBoundaryScope activeNavigationBoundaryScope READ
            activeNavigationBoundaryScope NOTIFY activeNavigationChanged)
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

    enum class ActiveNavigationBoundaryScope {
        NoNavigationBoundary,
        MediaNavigationBoundary,
        ImageNavigationBoundary,
    };
    Q_ENUM(ActiveNavigationBoundaryScope)

    explicit KiriDocumentSession(QObject *parent = nullptr);
    explicit KiriDocumentSession(
        KiriView::DocumentSessionRuntimeDependencies dependencies, QObject *parent = nullptr);
    ~KiriDocumentSession() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    DocumentKind documentKind() const;
    QString errorString() const;
    QString windowTitleSubject() const;
    QStringList openDialogNameFilters() const;
    bool displayedFileDeletionAvailable() const;
    bool fileDeletionInProgress() const;
    bool activeZoomPercentAvailable() const;
    bool activeZoomPercentKnown() const;
    double activeZoomPercent() const;
    bool activeZoomEditable() const;
    bool activeNavigationAvailable() const;
    bool activeNavigationKnown() const;
    bool activeNavigationEditable() const;
    int activeNavigationCurrentNumber() const;
    int activeNavigationCount() const;
    bool canOpenPreviousActiveNavigation() const;
    bool canOpenNextActiveNavigation() const;
    bool atKnownFirstActiveNavigation() const;
    bool atKnownLastActiveNavigation() const;
    ActiveNavigationBoundaryScope activeNavigationBoundaryScope() const;
    KiriImageDocument *imageDocument() const;
    KiriVideoDocument *videoDocument() const;

    Q_INVOKABLE void openPreviousActiveNavigation();
    Q_INVOKABLE void openNextActiveNavigation();
    Q_INVOKABLE void openFirstActiveNavigation();
    Q_INVOKABLE void openLastActiveNavigation();
    Q_INVOKABLE void openActiveNavigationAtNumber(int number);
    Q_INVOKABLE void deleteDisplayedFile(KiriDocumentSession::DeletionMode mode);

Q_SIGNALS:
    void sourceUrlChanged();
    void documentKindChanged();
    void errorStringChanged();
    void windowTitleSubjectChanged();
    void displayedFileDeletionAvailabilityChanged();
    void fileDeletionInProgressChanged();
    void activeZoomReadoutChanged();
    void activeNavigationChanged();

private:
    void handleSessionChanges(const std::vector<KiriView::DocumentSessionChange> &changes);

    std::unique_ptr<KiriImageDocument> m_imageDocument;
    std::unique_ptr<KiriVideoDocument> m_videoDocument;
    std::unique_ptr<KiriView::DocumentSessionRuntime> m_runtime;
};

#endif

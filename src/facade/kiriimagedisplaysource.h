// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEDISPLAYSOURCE_H
#define KIRIVIEW_KIRIIMAGEDISPLAYSOURCE_H

#include "presentation/imagedisplaysourceprojection.h"

#include <QObject>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class KiriImageDisplaySource : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(bool visible READ visible NOTIFY changed)
    Q_PROPERTY(PageRole pageRole READ pageRole NOTIFY changed)
    Q_PROPERTY(QUrl providerUrl READ providerUrl NOTIFY changed)
    Q_PROPERTY(QString revisionToken READ revisionToken NOTIFY changed)
    Q_PROPERTY(QString sourceIdentity READ sourceIdentity NOTIFY changed)
    Q_PROPERTY(ScopeKind selectedSourceScopeKind READ selectedSourceScopeKind NOTIFY changed)
    Q_PROPERTY(QUrl selectedSourceScopeUrl READ selectedSourceScopeUrl NOTIFY changed)
    Q_PROPERTY(QSize originalSize READ originalSize NOTIFY changed)
    Q_PROPERTY(QSize rasterSize READ rasterSize NOTIFY changed)
    Q_PROPERTY(QSize sourceSizeHint READ sourceSizeHint NOTIFY changed)
    Q_PROPERTY(Quality quality READ quality NOTIFY changed)
    Q_PROPERTY(Status status READ status NOTIFY changed)
    Q_PROPERTY(bool cacheEnabled READ cacheEnabled NOTIFY changed)
    Q_PROPERTY(bool loadAcknowledgmentRequired READ loadAcknowledgmentRequired NOTIFY changed)
    Q_PROPERTY(int rotationDegrees READ rotationDegrees NOTIFY changed)
    Q_PROPERTY(RetentionStatus retentionStatus READ retentionStatus NOTIFY changed)
    Q_PROPERTY(bool retainWhileLoadingEligible READ retainWhileLoadingEligible NOTIFY changed)

public:
    enum class PageRole {
        Primary,
        Secondary,
    };
    Q_ENUM(PageRole)

    enum class ScopeKind {
        Empty,
        DirectImage,
        OpenedCollection,
    };
    Q_ENUM(ScopeKind)

    enum class Quality {
        Exact,
        FirstDisplay,
        ThumbnailPreview,
        BoundedDetail,
        Unsupported,
        Failed,
    };
    Q_ENUM(Quality)

    enum class Status {
        Missing,
        Ready,
        Error,
        Unsupported,
    };
    Q_ENUM(Status)

    enum class RetentionStatus {
        None,
        StaleRetained,
    };
    Q_ENUM(RetentionStatus)

    explicit KiriImageDisplaySource(KiriView::DisplayedPageRole role, QObject *parent = nullptr);

    bool visible() const;
    PageRole pageRole() const;
    QUrl providerUrl() const;
    QString revisionToken() const;
    QString sourceIdentity() const;
    ScopeKind selectedSourceScopeKind() const;
    QUrl selectedSourceScopeUrl() const;
    QSize originalSize() const;
    QSize rasterSize() const;
    QSize sourceSizeHint() const;
    Quality quality() const;
    Status status() const;
    bool cacheEnabled() const;
    bool loadAcknowledgmentRequired() const;
    int rotationDegrees() const;
    RetentionStatus retentionStatus() const;
    bool retainWhileLoadingEligible() const;

    void setProjection(KiriView::ImageDisplaySourceProjection projection);

Q_SIGNALS:
    void changed();

private:
    KiriView::ImageDisplaySourceProjection m_projection;
};

#endif

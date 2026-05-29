// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIVIDEODOCUMENT_H
#define KIRIVIEW_KIRIVIDEODOCUMENT_H

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <vector>

namespace KiriView {
class VideoDocumentRuntime;
enum class VideoDocumentChange;
}

class KiriVideoDocument : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(
        QString windowTitleFileName READ windowTitleFileName NOTIFY windowTitleFileNameChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool seekable READ seekable NOTIFY seekableChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY hasAudioChanged)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged)
    Q_PROPERTY(bool zoomPercentKnown READ zoomPercentKnown NOTIFY zoomPercentKnownChanged)
    Q_PROPERTY(int zoomPercent READ zoomPercent NOTIFY zoomPercentChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(QObject *videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)

public:
    enum class Status {
        Null,
        Loading,
        Ready,
        Error,
    };
    Q_ENUM(Status)

    explicit KiriVideoDocument(QObject *parent = nullptr);
    ~KiriVideoDocument() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    Status status() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    qint64 duration() const;
    qint64 position() const;
    bool playing() const;
    bool seekable() const;
    bool hasVideo() const;
    bool hasAudio() const;
    QSize videoSize() const;
    bool zoomPercentKnown() const;
    int zoomPercent() const;
    bool muted() const;
    QObject *videoOutput() const;
    // QML assigns a QtMultimedia VideoOutput object here. KiriVideoDocument does not own it,
    // tracks its destruction, and detaches from the media player when this is set to null.
    void setVideoOutput(QObject *videoOutput);

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void setMuted(bool muted);
    Q_INVOKABLE void toggleMuted();
    Q_INVOKABLE void setPosition(qint64 position);
    Q_INVOKABLE void seekBy(qint64 deltaMilliseconds);
    Q_INVOKABLE void setVideoOutputGeometry(const QRectF &contentRect, const QRectF &sourceRect);

Q_SIGNALS:
    void sourceUrlChanged();
    void statusChanged();
    void errorStringChanged();
    void windowTitleFileNameChanged();
    void durationChanged();
    void positionChanged();
    void playingChanged();
    void seekableChanged();
    void hasVideoChanged();
    void hasAudioChanged();
    void videoSizeChanged();
    void zoomPercentKnownChanged();
    void zoomPercentChanged();
    void mutedChanged();
    void videoOutputChanged();

private:
    void handleDocumentChanges(const std::vector<KiriView::VideoDocumentChange> &changes);

    std::unique_ptr<KiriView::VideoDocumentRuntime> m_runtime;
};

#endif

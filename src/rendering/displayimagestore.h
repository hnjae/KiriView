// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DISPLAYIMAGESTORE_H
#define KIRIVIEW_DISPLAYIMAGESTORE_H

#include "rendering/imagerendercontext.h"

#include <QImage>
#include <QQuickImageProvider>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <memory>
#include <optional>

namespace KiriView {
enum class DisplayImageRetentionPriority {
    Nearby,
    Background,
    Visible,
};

enum class DisplayImagePinKind {
    Visible,
    StaleRetained,
    PendingLoad,
    FrameRetention,
};

enum class DisplayImageQuality {
    Exact,
    FirstDisplay,
    ThumbnailPreview,
    BoundedDetail,
    Unsupported,
    Failed,
};

struct DisplayImageEntry {
    QImage image;
    QSize originalSize;
    QSize rasterSize;
    QString sourceIdentity;
    DisplayedPageRole pageRole = DisplayedPageRole::Primary;
    DisplayImageQuality quality = DisplayImageQuality::Exact;
    DisplayImageRetentionPriority priority = DisplayImageRetentionPriority::Nearby;
    quint64 generation = 0;
    QString debugLabel;
};

struct DisplayImageStoreEntry {
    QString id;
    QImage image;
    QSize originalSize;
    QSize rasterSize;
    QString sourceIdentity;
    DisplayedPageRole pageRole = DisplayedPageRole::Primary;
    DisplayImageQuality quality = DisplayImageQuality::Exact;
    DisplayImageRetentionPriority priority = DisplayImageRetentionPriority::Nearby;
    qsizetype byteCost = 0;
    quint64 generation = 0;
    QString debugLabel;
};

class DisplayImageStore final
{
public:
    explicit DisplayImageStore(qsizetype byteBudget = 0);
    ~DisplayImageStore();

    QString insert(DisplayImageEntry entry);
    std::optional<DisplayImageStoreEntry> entry(const QString &id) const;
    void updatePriority(const QString &id, DisplayImageRetentionPriority priority);
    bool acquirePinLease(const QString &id, DisplayImagePinKind kind);
    void releasePinLease(const QString &id, DisplayImagePinKind kind);
    void release(const QString &id);
    void clear();
    qsizetype byteBudget() const;
    qsizetype byteCost() const;
    qsizetype size() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

class DisplayImageProvider final : public QQuickImageProvider
{
public:
    explicit DisplayImageProvider(std::shared_ptr<DisplayImageStore> store);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    std::shared_ptr<DisplayImageStore> m_store;
};

std::shared_ptr<DisplayImageStore> sharedDisplayImageStore();
QUrl displayImageSourceForId(const QString &id);
}

#endif

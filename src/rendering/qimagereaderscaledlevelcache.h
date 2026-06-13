// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_QIMAGEREADERSCALEDLEVELCACHE_H
#define KIRIVIEW_QIMAGEREADERSCALEDLEVELCACHE_H

#include "cache/imagelrucachestate.h"
#include "staticimage.h"

#include <QImage>
#include <QMutex>
#include <QtGlobal>
#include <optional>

namespace kiriview {
class QImageReaderScaledLevelCache final
{
public:
    explicit QImageReaderScaledLevelCache(qsizetype byteBudget = imageFullDecodeFallbackByteLimit);

    qsizetype byteBudget() const;
    qsizetype byteCost() const;
    bool contains(int level) const;
    std::optional<QImage> find(int level);
    bool insert(int level, const QImage &image);
    void clear();

private:
    mutable QMutex m_mutex;
    ImageLruCacheState<int, QImage> m_cache;
};
}

#endif

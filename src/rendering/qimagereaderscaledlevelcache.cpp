// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "qimagereaderscaledlevelcache.h"

#include "cache/imagebytecost.h"

#include <QMutexLocker>

namespace kiriview {
QImageReaderScaledLevelCache::QImageReaderScaledLevelCache(qsizetype byteBudget)
    : m_cache(byteBudget)
{
}

qsizetype QImageReaderScaledLevelCache::byteBudget() const { return m_cache.byteBudget(); }

qsizetype QImageReaderScaledLevelCache::byteCost() const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.byteCost();
}

bool QImageReaderScaledLevelCache::contains(int level) const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.contains(level);
}

std::optional<QImage> QImageReaderScaledLevelCache::find(int level)
{
    QMutexLocker locker(&m_mutex);
    return m_cache.find(level);
}

bool QImageReaderScaledLevelCache::insert(int level, const QImage &image)
{
    const qsizetype byteCost = imageByteCost(image);

    QMutexLocker locker(&m_mutex);
    return m_cache.insert(level, image, byteCost);
}

void QImageReaderScaledLevelCache::clear()
{
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
}
}

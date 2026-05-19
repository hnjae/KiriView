// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "qimagereaderscaledlevelcache.h"

#include "imagebytecost.h"
#include "imagecachepolicy.h"

#include <QMutexLocker>
#include <algorithm>
#include <utility>

namespace KiriView {
QImageReaderScaledLevelCache::QImageReaderScaledLevelCache(qsizetype byteBudget)
    : m_byteBudget(std::max<qsizetype>(0, byteBudget))
{
}

qsizetype QImageReaderScaledLevelCache::byteBudget() const { return m_byteBudget; }

qsizetype QImageReaderScaledLevelCache::byteCost() const
{
    QMutexLocker locker(&m_mutex);
    return m_byteCost;
}

bool QImageReaderScaledLevelCache::contains(int level) const
{
    QMutexLocker locker(&m_mutex);
    return findEntry(level) != m_entries.cend();
}

std::optional<QImage> QImageReaderScaledLevelCache::find(int level)
{
    QMutexLocker locker(&m_mutex);
    auto entry = findEntry(level);
    if (entry == m_entries.end()) {
        return std::nullopt;
    }

    entry->lastUse = ++m_useClock;
    return entry->image;
}

bool QImageReaderScaledLevelCache::insert(int level, const QImage &image)
{
    const qsizetype byteCost = imageByteCost(image);
    if (byteCost <= 0 || byteCost > m_byteBudget) {
        return false;
    }

    QMutexLocker locker(&m_mutex);
    auto existing = findEntry(level);
    if (existing != m_entries.end()) {
        m_byteCost -= existing->byteCost;
        m_entries.erase(existing);
    }

    m_entries.push_back(Entry { level, image, byteCost, ++m_useClock });
    m_byteCost += byteCost;
    trimToBudget();
    return true;
}

void QImageReaderScaledLevelCache::clear()
{
    QMutexLocker locker(&m_mutex);
    m_entries.clear();
    m_byteCost = 0;
}

void QImageReaderScaledLevelCache::trimToBudget()
{
    std::vector<qsizetype> byteCosts;
    std::vector<quint64> lastUses;
    byteCosts.reserve(m_entries.size());
    lastUses.reserve(m_entries.size());

    for (const Entry &entry : m_entries) {
        byteCosts.push_back(entry.byteCost);
        lastUses.push_back(entry.lastUse);
    }

    const std::vector<std::size_t> retainedIndices
        = lruCacheRetainedIndices(byteCosts, lastUses, m_byteBudget);

    std::vector<Entry> retainedEntries;
    retainedEntries.reserve(retainedIndices.size());
    qsizetype retainedByteCost = 0;
    for (std::size_t index : retainedIndices) {
        if (index >= m_entries.size()) {
            continue;
        }

        retainedByteCost += m_entries[index].byteCost;
        retainedEntries.push_back(std::move(m_entries[index]));
    }

    m_entries = std::move(retainedEntries);
    m_byteCost = retainedByteCost;
}

std::vector<QImageReaderScaledLevelCache::Entry>::iterator QImageReaderScaledLevelCache::findEntry(
    int level)
{
    return std::find_if(m_entries.begin(), m_entries.end(),
        [level](const Entry &entry) { return entry.level == level; });
}

std::vector<QImageReaderScaledLevelCache::Entry>::const_iterator
QImageReaderScaledLevelCache::findEntry(int level) const
{
    return std::find_if(m_entries.cbegin(), m_entries.cend(),
        [level](const Entry &entry) { return entry.level == level; });
}
}

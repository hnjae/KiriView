// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELRUCACHESTATE_H
#define KIRIVIEW_IMAGELRUCACHESTATE_H

#include "cache/imagecachepolicy.h"

#include <QtGlobal>
#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView {
template <typename Key, typename Value> class ImageLruCacheState final
{
public:
    explicit ImageLruCacheState(qsizetype byteBudget)
        : m_byteBudget(std::max<qsizetype>(0, byteBudget))
    {
    }

    qsizetype byteBudget() const { return m_byteBudget; }
    qsizetype byteCost() const { return m_byteCost; }

    bool contains(const Key &key) const { return findEntry(key) != m_entries.cend(); }

    std::optional<Value> find(const Key &key)
    {
        auto entry = findEntry(key);
        if (entry == m_entries.end()) {
            return std::nullopt;
        }

        entry->lastUse = ++m_useClock;
        return entry->value;
    }

    std::vector<Value> values() const
    {
        std::vector<Value> cachedValues;
        cachedValues.reserve(m_entries.size());
        for (const Entry &entry : m_entries) {
            cachedValues.push_back(entry.value);
        }
        return cachedValues;
    }

    bool insert(Key key, Value value, qsizetype byteCost)
    {
        if (byteCost <= 0 || byteCost > m_byteBudget) {
            return false;
        }

        auto existing = findEntry(key);
        if (existing != m_entries.end()) {
            m_byteCost -= existing->byteCost;
            m_entries.erase(existing);
        }

        m_entries.push_back(Entry { std::move(key), std::move(value), byteCost, ++m_useClock });
        m_byteCost += byteCost;
        trimToBudget();
        return true;
    }

    void clear()
    {
        m_entries.clear();
        m_byteCost = 0;
    }

private:
    struct Entry {
        Key key;
        Value value;
        qsizetype byteCost = 0;
        quint64 lastUse = 0;
    };

    void trimToBudget()
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

    typename std::vector<Entry>::iterator findEntry(const Key &key)
    {
        return std::find_if(m_entries.begin(), m_entries.end(),
            [&key](const Entry &entry) { return entry.key == key; });
    }

    typename std::vector<Entry>::const_iterator findEntry(const Key &key) const
    {
        return std::find_if(m_entries.cbegin(), m_entries.cend(),
            [&key](const Entry &entry) { return entry.key == key; });
    }

    std::vector<Entry> m_entries;
    qsizetype m_byteBudget = 0;
    qsizetype m_byteCost = 0;
    quint64 m_useClock = 0;
};
}

#endif

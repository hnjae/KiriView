// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_QIMAGEREADERSCALEDLEVELCACHE_H
#define KIRIVIEW_QIMAGEREADERSCALEDLEVELCACHE_H

#include "staticimage.h"

#include <QImage>
#include <QMutex>
#include <QtGlobal>
#include <optional>
#include <vector>

namespace KiriView {
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
    struct Entry {
        int level = 0;
        QImage image;
        qsizetype byteCost = 0;
        quint64 lastUse = 0;
    };

    void trimToBudget();
    std::vector<Entry>::iterator findEntry(int level);
    std::vector<Entry>::const_iterator findEntry(int level) const;

    mutable QMutex m_mutex;
    std::vector<Entry> m_entries;
    qsizetype m_byteBudget = 0;
    qsizetype m_byteCost = 0;
    quint64 m_useClock = 0;
};
}

#endif

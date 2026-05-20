// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODEDISPLAYEDHISTORY_H
#define KIRIVIEW_PREDECODEDISPLAYEDHISTORY_H

#include <QUrl>
#include <cstddef>
#include <vector>

namespace KiriView {
class PredecodeDisplayedHistory
{
public:
    void clear();
    void setDisplayedUrls(const std::vector<QUrl> &urls);

    bool currentContains(const QUrl &url) const;
    bool recentContains(const QUrl &url) const;
    bool retainedContains(const QUrl &url) const;
    std::size_t currentPriority(const QUrl &url) const;
    std::size_t recentPriority(const QUrl &url) const;

    const std::vector<QUrl> &currentUrls() const;
    const std::vector<QUrl> &recentUrls() const;

private:
    static bool containsUrl(const std::vector<QUrl> &urls, const QUrl &url);
    static void removeUrl(std::vector<QUrl> &urls, const QUrl &url);
    static std::size_t priority(const std::vector<QUrl> &urls, const QUrl &url);

    std::vector<QUrl> m_currentUrls;
    std::vector<QUrl> m_recentUrls;
};
}

#endif

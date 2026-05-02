// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGENAVIGATIONTYPES_H
#define KIRIVIEW_IMAGENAVIGATIONTYPES_H

#include <QString>
#include <QUrl>
#include <vector>

namespace KiriView {
struct ImageNavigationCandidate {
    QUrl url;
    QString name;
};

enum class NavigationDirection : int {
    Previous,
    Next,
};

enum class ContainerNavigationCandidateType {
    Directory,
    ComicBookArchive,
};

struct ContainerNavigationCandidate {
    QUrl url;
    QString name;
    ContainerNavigationCandidateType type = ContainerNavigationCandidateType::Directory;
};

struct PageNavigationState {
    std::vector<QUrl> urls;
    int currentIndex = -1;
};
}

#endif

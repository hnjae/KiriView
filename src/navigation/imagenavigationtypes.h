// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGENAVIGATIONTYPES_H
#define KIRIVIEW_IMAGENAVIGATIONTYPES_H

#include <QString>
#include <QUrl>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView {
enum class ImageNavigationCandidateKind {
    Image,
    Video,
};

struct ImageNavigationCandidate {
    QUrl url;
    QString name;
    ImageNavigationCandidateKind kind = ImageNavigationCandidateKind::Image;
};

struct ImageNavigationTarget {
    ImageNavigationTarget() = default;
    explicit ImageNavigationTarget(QUrl url,
        ImageNavigationCandidateKind kind = ImageNavigationCandidateKind::Image, QString name = {})
        : url(std::move(url))
        , kind(kind)
        , name(std::move(name))
    {
        if (this->name.isEmpty()) {
            this->name = this->url.fileName(QUrl::PrettyDecoded);
        }
    }

    QUrl url;
    ImageNavigationCandidateKind kind = ImageNavigationCandidateKind::Image;
    QString name;

    friend bool operator==(const ImageNavigationTarget &left, const ImageNavigationTarget &right)
    {
        return left.url == right.url && left.kind == right.kind && left.name == right.name;
    }
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
    PageNavigationState() = default;
    PageNavigationState(std::vector<ImageNavigationTarget> targets, int currentIndex = -1)
        : targets(std::move(targets))
        , currentIndex(currentIndex)
    {
    }
    PageNavigationState(std::vector<QUrl> urls, int currentIndex = -1);

    std::vector<ImageNavigationTarget> targets;
    int currentIndex = -1;
};

struct ImagePageNavigationSnapshot {
    PageNavigationState state;

    int currentPageNumber() const;
    int imageCount() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;
};
}

#endif

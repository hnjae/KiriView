// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATIONTYPES_H
#define KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATIONTYPES_H

#include <QString>
#include <QUrl>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView {
enum class ImageDocumentPageKind {
    Image,
    Video,
};

struct ImageDocumentPageCandidate {
    QUrl url;
    QString name;
    ImageDocumentPageKind kind = ImageDocumentPageKind::Image;
};

struct ImageDocumentPageTarget {
    ImageDocumentPageTarget() = default;
    explicit ImageDocumentPageTarget(
        QUrl url, ImageDocumentPageKind kind = ImageDocumentPageKind::Image, QString name = {})
        : url(std::move(url))
        , kind(kind)
        , name(std::move(name))
    {
        if (this->name.isEmpty()) {
            this->name = this->url.fileName(QUrl::PrettyDecoded);
        }
    }

    QUrl url;
    ImageDocumentPageKind kind = ImageDocumentPageKind::Image;
    QString name;

    friend bool operator==(
        const ImageDocumentPageTarget &left, const ImageDocumentPageTarget &right)
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
    PageNavigationState(std::vector<ImageDocumentPageTarget> targets, int currentIndex = -1)
        : targets(std::move(targets))
        , currentIndex(currentIndex)
    {
    }
    PageNavigationState(std::vector<QUrl> urls, int currentIndex = -1);

    std::vector<ImageDocumentPageTarget> targets;
    int currentIndex = -1;
};

struct ImageDocumentPageNavigationSnapshot {
    PageNavigationState state;

    int currentPageNumber() const;
    int pageCount() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;
};

struct ImageDocumentPageActiveNavigationSnapshot {
    bool known = false;
    bool canOpenPrevious = false;
    bool canOpenNext = false;
    bool atKnownFirst = false;
    bool atKnownLast = false;
    int currentNumber = 0;
    int count = 0;
};
}

#endif

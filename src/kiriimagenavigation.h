// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGENAVIGATION_H
#define KIRIVIEW_KIRIIMAGENAVIGATION_H

#include <KFileItem>
#include <KIO/UDSEntry>
#include <QString>
#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
struct ImageNavigationCandidate {
    QUrl url;
    QString name;
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

QUrl normalizedImageUrl(const QUrl &url);
QUrl parentUrlForContainerNavigation(const QUrl &containerUrl);
std::vector<QUrl> predecodeWindowImageUrls(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl);
std::vector<QUrl> imageNavigationCandidateUrls(
    const std::vector<ImageNavigationCandidate> &candidates);
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url);
bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl);
std::optional<QUrl> containingComicBookArchiveRootUrl(const QUrl &url);
QString windowTitleFileNameForDisplayedUrl(
    const QUrl &displayedUrl, const QUrl &displayedComicBookRootUrl);
void sortImageNavigationCandidates(std::vector<ImageNavigationCandidate> *candidates);
std::vector<ImageNavigationCandidate> imageNavigationCandidates(const KFileItemList &items);
std::vector<ContainerNavigationCandidate> containerNavigationCandidates(const KFileItemList &items);
void appendArchiveImageNavigationCandidates(std::vector<ImageNavigationCandidate> *candidates,
    const KIO::UDSEntryList &entries, const QUrl &directoryUrl, const QUrl &archiveRootUrl);
QUrl navigationSourceUrl(const QUrl &url);
QUrl containerNavigationUrlForImage(const QUrl &imageUrl, const QUrl &comicBookRootUrl);
bool sameContainerNavigationUrl(const QUrl &left, const QUrl &right);
}

#endif

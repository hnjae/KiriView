// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATEREPOSITORY_H
#define KIRIVIEW_IMAGECANDIDATEREPOSITORY_H

#include "imageasyncdependencies.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <optional>
#include <utility>
#include <variant>

namespace KiriView {
enum class ImageCandidateRepositoryError {
    Generic,
    EmptyContainer,
    InvalidComicBookArchive,
};

class ImageCandidateListSource
{
public:
    struct Directory {
        QUrl directoryUrl;
    };

    struct ArchiveDocument {
        ArchiveDocumentLocation archiveDocument;
    };

    static ImageCandidateListSource forDirectory(QUrl directoryUrl);
    static ImageCandidateListSource forArchiveDocument(ArchiveDocumentLocation archiveDocument);

    ArchiveDocumentLocation archiveDocument() const;

    template <typename Visitor> decltype(auto) visit(Visitor &&visitor) const
    {
        return std::visit(std::forward<Visitor>(visitor), m_source);
    }

private:
    using Payload = std::variant<Directory, ArchiveDocument>;

    explicit ImageCandidateListSource(Payload source);

    Payload m_source;
};

class ImageCandidateListContext
{
public:
    using DirectoryContext = ImageCandidateListSource::Directory;
    using ArchiveDocumentContext = ImageCandidateListSource::ArchiveDocument;

    static ImageCandidateListContext forDirectory(QUrl currentUrl, QUrl directoryUrl);
    static ImageCandidateListContext forArchiveDocument(
        QUrl currentUrl, ArchiveDocumentLocation archiveDocument);

    const QUrl &currentUrl() const;
    const ImageCandidateListSource &source() const;
    ArchiveDocumentLocation archiveDocument() const;

    template <typename Visitor> decltype(auto) visit(Visitor &&visitor) const
    {
        return m_source.visit(std::forward<Visitor>(visitor));
    }

private:
    explicit ImageCandidateListContext(QUrl currentUrl, ImageCandidateListSource source);

    QUrl m_currentUrl;
    ImageCandidateListSource m_source;
};

using ContainerImageCallback = std::function<void(const QUrl &, const QUrl &)>;
using CandidateRepositoryErrorCallback
    = std::function<void(const QUrl &, ImageCandidateRepositoryError, const QString &)>;

std::optional<ImageCandidateListContext> imageCandidateListContextForDisplayedImage(
    const DisplayedImageLocation &location);

class ImageCandidateRepository
{
public:
    ImageCandidateRepository();
    explicit ImageCandidateRepository(ImageNavigationCandidateProvider provider);

    ImageIoJob loadImages(QObject *receiver, const ImageCandidateListSource &source,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadImages(QObject *receiver, const ImageCandidateListContext &context,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadDirectoryImages(QObject *receiver, const QUrl &directoryUrl,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadArchiveImages(QObject *receiver, ArchiveDocumentLocation archiveDocument,
        ImageCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadContainers(QObject *receiver, const QUrl &directoryUrl,
        ContainerCandidatesCallback callback, ErrorCallback errorCallback) const;
    ImageIoJob loadFirstImageInContainer(QObject *receiver,
        const ContainerNavigationCandidate &container, ContainerImageCallback callback,
        CandidateRepositoryErrorCallback errorCallback) const;

private:
    ImageIoJob loadImagesInContainer(QObject *receiver,
        const ContainerNavigationCandidate &container, ImageCandidatesCallback callback,
        CandidateRepositoryErrorCallback errorCallback) const;

    ImageNavigationCandidateProvider m_provider;
};
}

#endif

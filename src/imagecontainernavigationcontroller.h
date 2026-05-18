// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECONTAINERNAVIGATIONCONTROLLER_H
#define KIRIVIEW_IMAGECONTAINERNAVIGATIONCONTROLLER_H

#include "imagecandidaterepository.h"
#include "imageiojob.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <vector>

namespace KiriView {
using ContainerNavigationError = ImageCandidateRepositoryError;

class ImageContainerNavigationController final : public QObject
{
public:
    using OpenContainerImageCallback = std::function<void(const QUrl &, const QUrl &)>;
    using ContainerNavigationErrorCallback
        = std::function<void(const QUrl &, ContainerNavigationError, const QString &)>;

    struct Callbacks {
        OpenContainerImageCallback openContainerImage;
        ContainerNavigationErrorCallback containerNavigationError;
    };

    ImageContainerNavigationController(
        QObject *parent, const ImageCandidateRepository &candidateRepository, Callbacks callbacks);

    static bool canOpenAdjacentContainer(const QUrl &currentContainerUrl);

    void openAdjacentContainer(const QUrl &currentContainerUrl, NavigationDirection direction);
    void cancel();

private:
    void finishContainerNavigation(std::vector<ContainerNavigationCandidate> candidates,
        NavigationDirection direction, const QUrl &currentContainerUrl);
    void openImageFromContainerNavigation(const QUrl &imageUrl, const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString);

    const ImageCandidateRepository &m_candidateRepository;
    Callbacks m_callbacks;
    ImageIoJob m_containerListJob;
    ImageIoJob m_firstImageJob;
};
}

#endif

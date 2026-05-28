// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECONTAINERNAVIGATIONCONTROLLER_H
#define KIRIVIEW_IMAGECONTAINERNAVIGATIONCONTROLLER_H

#include "async/imageiojob.h"
#include "imagecontainernavigationstate.h"
#include "imagecontaineropenplan.h"
#include "imagedocumentpagenavigationplan.h"
#include "imagedocumentpagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <vector>

namespace KiriView {
class ImageDocumentPageCandidateRepository;

class ImageContainerNavigationController final : public QObject
{
public:
    using NavigationPlanCallback = std::function<void(ImageDocumentPageNavigationPlan)>;

    struct Callbacks {
        NavigationPlanCallback navigationPlan;
    };

    ImageContainerNavigationController(QObject *parent,
        const ImageDocumentPageCandidateRepository &candidateRepository, Callbacks callbacks);

    static bool canOpenAdjacentContainer(const QUrl &currentContainerUrl);

    void openAdjacentContainer(const QUrl &currentContainerUrl, NavigationDirection direction);
    void cancel();

private:
    void finishContainerNavigation(quint64 operationId,
        std::vector<ContainerNavigationCandidate> candidates, NavigationDirection direction,
        const QUrl &currentContainerUrl);
    void finishContainerNavigationListWithError(quint64 operationId);
    void loadFirstImageFromContainerNavigation(
        quint64 operationId, const ContainerNavigationCandidate &container);
    void finishContainerNavigationImageLoad(quint64 operationId, const QUrl &containerUrl,
        std::vector<ImageDocumentPageCandidate> candidates);
    void openImageFromContainerNavigation(
        quint64 operationId, const ImageDocumentPageTarget &target, const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(quint64 operationId, const QUrl &containerUrl,
        ContainerNavigationError error, const QString &errorString);
    void reportNavigationPlan(ImageDocumentPageNavigationPlan plan);

    const ImageDocumentPageCandidateRepository &m_candidateRepository;
    Callbacks m_callbacks;
    ImageContainerNavigationState m_navigationState;
    ImageIoJob m_containerListJob;
    ImageIoJob m_firstImageJob;
};
}

#endif

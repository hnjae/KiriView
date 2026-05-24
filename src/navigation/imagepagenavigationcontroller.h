// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPAGENAVIGATIONCONTROLLER_H
#define KIRIVIEW_IMAGEPAGENAVIGATIONCONTROLLER_H

#include "async/imageiojob.h"
#include "imagecandidaterepository.h"
#include "imagenavigationplan.h"
#include "imagenavigationtypes.h"
#include "imagepagenavigationmodel.h"

#include <QObject>
#include <QUrl>
#include <functional>
#include <optional>
#include <vector>

namespace KiriView {
class ImagePageNavigationController final : public QObject
{
public:
    using NavigationPlanCallback = std::function<void(ImageNavigationPlan)>;
    using PageNavigationChangedCallback = std::function<void()>;
    using DeletionInProgressCallback = std::function<bool()>;

    struct Callbacks {
        NavigationPlanCallback navigationPlan;
        PageNavigationChangedCallback pageNavigationChanged;
        DeletionInProgressCallback deletionInProgress;
    };

    ImagePageNavigationController(
        QObject *parent, const ImageCandidateRepository &candidateRepository, Callbacks callbacks);

    int currentPageNumber() const;
    int imageCount() const;
    ImagePageNavigationSnapshot snapshot() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;
    std::optional<QUrl> selectPage(int pageNumber);

    void openAdjacentImage(
        std::optional<ImageCandidateListContext> context, NavigationDirection direction);
    void update(std::optional<ImageCandidateListContext> context);
    void cancelNavigation();
    void cancelUpdate();
    void clear();

private:
    void finishNavigation(std::vector<ImageNavigationCandidate> candidates,
        NavigationDirection direction, const QUrl &currentUrl,
        ImageCandidateListSource candidateSource);
    void watchChanges(const ImageCandidateListContext &context);
    void updateFromChangedCandidates(
        std::vector<ImageNavigationCandidate> candidates, ImageCandidateListSource source);
    void notifyChanged();
    void reportNavigationPlan(ImageNavigationPlan plan);
    void recoverFromCurrentImageRemoved(
        std::vector<ImageNavigationCandidate> candidates, ImageCandidateListContext context);
    bool deletionInProgress() const;

    const ImageCandidateRepository &m_candidateRepository;
    Callbacks m_callbacks;
    ImagePageNavigationModel m_model;
    ImageIoJob m_navigationListerJob;
    ImageIoJob m_refreshListerJob;
    ImageIoJob m_changesJob;
};
}

#endif

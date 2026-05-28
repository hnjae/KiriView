// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATIONCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATIONCONTROLLER_H

#include "async/imageiojob.h"
#include "imagedocumentpagecandidaterepository.h"
#include "imagedocumentpagenavigationmodel.h"
#include "imagedocumentpagenavigationplan.h"
#include "imagedocumentpagenavigationtypes.h"

#include <QObject>
#include <QUrl>
#include <functional>
#include <optional>
#include <vector>

namespace KiriView {
class ImageDocumentPageNavigationController final : public QObject
{
public:
    using NavigationPlanCallback = std::function<void(ImageDocumentPageNavigationPlan)>;
    using PageNavigationChangedCallback = std::function<void()>;
    using DeletionInProgressCallback = std::function<bool()>;

    struct Callbacks {
        NavigationPlanCallback navigationPlan;
        PageNavigationChangedCallback pageNavigationChanged;
        DeletionInProgressCallback deletionInProgress;
    };

    ImageDocumentPageNavigationController(QObject *parent,
        const ImageDocumentPageCandidateRepository &candidateRepository, Callbacks callbacks);

    int currentPageNumber() const;
    int pageCount() const;
    ImageDocumentPageNavigationSnapshot snapshot() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;
    std::optional<ImageDocumentPageTarget> targetAtPage(int pageNumber) const;
    std::optional<ImageDocumentPageTarget> selectPage(int pageNumber);

    void openAdjacentPage(std::optional<ImageDocumentPageCandidateListContext> context,
        NavigationDirection direction);
    void update(std::optional<ImageDocumentPageCandidateListContext> context);
    void cancelNavigation();
    void cancelUpdate();
    void clear();

private:
    void finishNavigation(std::vector<ImageDocumentPageCandidate> candidates,
        NavigationDirection direction, const QUrl &currentUrl,
        ImageDocumentPageCandidateListSource candidateSource);
    void watchChanges(const ImageDocumentPageCandidateListContext &context);
    void updateFromChangedCandidates(std::vector<ImageDocumentPageCandidate> candidates,
        ImageDocumentPageCandidateListSource source);
    void notifyChanged();
    void reportNavigationPlan(ImageDocumentPageNavigationPlan plan);
    void recoverFromCurrentPageRemoved(std::vector<ImageDocumentPageCandidate> candidates,
        ImageDocumentPageCandidateListContext context);
    bool deletionInProgress() const;

    const ImageDocumentPageCandidateRepository &m_candidateRepository;
    Callbacks m_callbacks;
    ImageDocumentPageNavigationModel m_model;
    ImageIoJob m_navigationListerJob;
    ImageIoJob m_refreshListerJob;
    ImageIoJob m_changesJob;
};
}

#endif

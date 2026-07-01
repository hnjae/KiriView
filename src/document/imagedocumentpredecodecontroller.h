// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPREDECODECONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTPREDECODECONTROLLER_H

#include "async/timerscheduler.h"
#include "decoding/imagedecodedependencies.h"
#include "navigation/imagedocumentpagecandidatelistsource.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "predecode/predecodedimage.h"
#include "predecode/predecoderuntimefacts.h"
#include "system/powersaverprovider.h"

#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace kiriview {
class ImageDocumentState;
class ImagePredecodeCoordinator;
class ImagePageSurfaceController;
class ImagePresentationRuntime;

class ImageDocumentPredecodeController final
{
public:
    using CurrentPageNumberCallback = std::function<int()>;
    using PageCandidateSnapshotCallback
        = std::function<std::optional<ImageDocumentPageCandidateSnapshot>()>;

    ImageDocumentPredecodeController(QObject* parent, ImageDocumentState& state,
        ImagePageSurfaceController& pageSurfaceController,
        ImagePresentationRuntime& presentationRuntime,
        ImageDocumentPageCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies, qsizetype cacheByteBudget,
        CurrentPageNumberCallback currentPageNumber = {},
        PageCandidateSnapshotCallback pageCandidateSnapshot = {},
        PowerSaverProvider powerSaverProvider = {}, bool ordinaryDirectMediaPredecodeEnabled = true,
        TimerScheduler timerScheduler = {}, PredecodeThreadCountProvider threadCountProvider = {});
    ~ImageDocumentPredecodeController();

    void scheduleAdjacentImagePredecode(
        std::optional<DisplayedPredecodeImage> secondaryImage = std::nullopt);
    void scheduleImageNavigationTargetPredecode(const ImageDocumentPageTarget& target,
        int targetPageIndex, std::optional<DisplayedPredecodeImage> secondaryImage = std::nullopt);
    void cancel();
    void clear();
    std::optional<PredecodedImage> findPredecodedImage(const QUrl& url) const;

private:
    ImageDocumentState& m_state;
    ImagePageSurfaceController& m_pageSurfaceController;
    ImagePresentationRuntime& m_presentationRuntime;
    std::unique_ptr<ImagePredecodeCoordinator> m_coordinator;
    CurrentPageNumberCallback m_currentPageNumber;
    PageCandidateSnapshotCallback m_pageCandidateSnapshot;
    bool m_ordinaryDirectMediaPredecodeEnabled = true;
};
}

#endif

// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPREDECODECOORDINATOR_H
#define KIRIVIEW_IMAGEPREDECODECOORDINATOR_H

#include "imageasyncdependencies.h"
#include "imageasyncticket.h"
#include "imagecandidaterepository.h"
#include "imagedecodejob.h"
#include "imageiojob.h"
#include "imagelocation.h"
#include "predecodecache.h"
#include "predecodedimage.h"
#include "staticimage.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
class ImagePredecodeCoordinator final : public QObject
{
public:
    struct Context {
        DisplayedPredecodeImage primaryImage;
        std::optional<DisplayedPredecodeImage> secondaryImage;
        ImageFirstDisplayDecodeContext firstDisplayContext;
        int pageIndex = -1;
    };

    enum class MomentumMode {
        Neutral,
        NextBiased,
        PrevBiased,
        ScrubbingNext,
        ScrubbingPrev,
    };

    explicit ImagePredecodeCoordinator(QObject *parent = nullptr);
    ImagePredecodeCoordinator(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies);

    void schedule(Context context);
    void cancel();
    void clear();
    std::optional<PredecodedImage> tryTake(const QUrl &url) const;

private:
    struct ActivePredecodeRequest {
        ImageDecodeRequest request;
        QUrl normalizedUrl;
    };

    enum class MomentumDirection {
        None,
        Previous,
        Next,
    };

    void cacheDisplayedImages(const Context &context);
    void startDebouncedPredecode();
    void scheduleSettledNeutralPredecode();
    void scheduleAdjacentImagePredecode(const Context &context, quint64 generation);
    void startPredecodeImageLoads(const std::vector<QUrl> &urls,
        const ArchiveDocumentLocation &archiveDocument, const Context &context, quint64 generation,
        std::size_t parallelLimit);
    void startNextPredecodeImageLoad(quint64 generation);
    void startPredecodeImageLoad(
        const QUrl &url, const ArchiveDocumentLocation &archiveDocument, quint64 generation);
    void finishPredecodeImageLoadError(const ImageDecodeRequest &request);
    void finishPredecodeImageDecode(ImageDecodeRequest request, const DecodedImageResult &result);
    QUrl activePredecodeUrl() const;
    bool hasActivePredecodeRequest() const;
    std::optional<ActivePredecodeRequest> takeActivePredecodeRequest(
        const ImageDecodeRequest &request);
    bool predecodeRequestIsActive(const ImageDecodeRequest &request) const;
    void clearActivePredecodeRequest();
    void cancelBackgroundWork();
    void resetNavigationMomentum();
    void updateNavigationMomentum(int pageIndex, qint64 monotonicMsec);
    qint64 currentMonotonicMsec() const;

    ImageIoJob m_listerJob;
    ImageDecodeJob m_decodeJob;
    ImageCandidateRepository m_candidateRepository;
    PredecodeCache m_cache;
    std::optional<ActivePredecodeRequest> m_activePredecodeRequest;
    std::optional<Context> m_pendingContext;
    ImageFirstDisplayDecodeContext m_firstDisplayContext;
    ImageAsyncTicket m_generation;
    QTimer m_debounceTimer;
    QTimer m_neutralTimer;
    QElapsedTimer m_monotonicClock;
    quint64 m_pendingGeneration = 0;
    std::size_t m_parallelLimit = 1;
    int m_lastPageIndex = -1;
    qint64 m_lastNavigationMsec = -1;
    int m_sameDirectionMoveCount = 0;
    MomentumDirection m_lastMomentumDirection = MomentumDirection::None;
    MomentumMode m_momentumMode = MomentumMode::Neutral;
};
}

#endif

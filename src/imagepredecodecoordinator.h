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
#include "predecodepolicy.h"
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

    explicit ImagePredecodeCoordinator(QObject *parent = nullptr);
    ImagePredecodeCoordinator(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies);

    void schedule(Context context);
    void setPowerSaverEnabled(bool enabled);
    bool powerSaverEnabled() const;
    void cancel();
    void clear();
    std::optional<PredecodedImage> tryTake(const QUrl &url) const;

private:
    struct ActivePredecodeRequest {
        ImageDecodeRequest request;
        QUrl normalizedUrl;
        ImageDecodeJob *decodeJob = nullptr;
    };

    void cacheDisplayedImages(const Context &context);
    void startDebouncedPredecode();
    void scheduleSettledNeutralPredecode();
    void scheduleAdjacentImagePredecode(const Context &context, quint64 generation);
    void startPredecodeImageLoads(const std::vector<QUrl> &urls,
        const ArchiveDocumentLocation &archiveDocument, const Context &context, quint64 generation,
        std::size_t parallelLimit);
    void startNextPredecodeImageLoads(quint64 generation);
    bool startPredecodeImageLoad(
        const QUrl &url, const ArchiveDocumentLocation &archiveDocument, quint64 generation);
    void finishPredecodeImageLoadError(const ImageDecodeRequest &request);
    void finishPredecodeImageDecode(ImageDecodeRequest request, const DecodedImageResult &result);
    std::vector<QUrl> activePredecodeUrls() const;
    bool hasActivePredecodeRequest(const QUrl &normalizedUrl) const;
    std::optional<ActivePredecodeRequest> takeActivePredecodeRequest(
        const ImageDecodeRequest &request);
    bool predecodeRequestIsActive(const ImageDecodeRequest &request) const;
    void cancelActivePredecodeRequests();
    void cancelBackgroundWork();
    void resetNavigationMomentum();
    void updateNavigationMomentum(int pageIndex, qint64 monotonicMsec);
    qint64 currentMonotonicMsec() const;

    ImageIoJob m_listerJob;
    ImageDecodeDependencies m_decodeDependencies;
    ImageCandidateRepository m_candidateRepository;
    PredecodeCache m_cache;
    std::vector<ActivePredecodeRequest> m_activePredecodeRequests;
    std::optional<Context> m_displayedContext;
    std::optional<Context> m_pendingContext;
    ImageFirstDisplayDecodeContext m_firstDisplayContext;
    ImageAsyncTicket m_generation;
    QTimer m_debounceTimer;
    QTimer m_neutralTimer;
    QElapsedTimer m_monotonicClock;
    quint64 m_pendingGeneration = 0;
    std::size_t m_parallelLimit = 1;
    PredecodeMomentumState m_momentumState;
    bool m_powerSaverEnabled = false;
};
}

#endif

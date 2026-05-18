// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPREDECODECOORDINATOR_H
#define KIRIVIEW_IMAGEPREDECODECOORDINATOR_H

#include "imageasyncdependencies.h"
#include "imageasyncticket.h"
#include "imagecandidaterepository.h"
#include "imageiojob.h"
#include "imagelocation.h"
#include "predecodedimage.h"
#include "predecodeloadcontroller.h"
#include "predecodepolicy.h"
#include "staticimage.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <cstddef>
#include <memory>
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
        ImageDecodeDependencies decodeDependencies, PowerSaverProvider powerSaverProvider = {});

    void schedule(Context context);
    void setPowerSaverEnabled(bool enabled);
    bool powerSaverEnabled() const;
    void cancel();
    void clear();
    std::optional<PredecodedImage> tryTake(const QUrl &url) const;

private:
    void cacheDisplayedImages(const Context &context);
    std::vector<DisplayedPredecodeImage> displayedImages(const Context &context) const;
    void startDebouncedPredecode();
    void scheduleSettledNeutralPredecode();
    void scheduleAdjacentImagePredecode(const Context &context, quint64 generation);
    void startPredecodeImageLoads(const std::vector<QUrl> &urls,
        const ArchiveDocumentLocation &archiveDocument, const Context &context, quint64 generation,
        std::size_t parallelLimit);
    void cancelBackgroundWork();
    void resetNavigationMomentum();
    void updateNavigationMomentum(int pageIndex, qint64 monotonicMsec);
    qint64 currentMonotonicMsec() const;

    ImageIoJob m_listerJob;
    ImageCandidateRepository m_candidateRepository;
    PredecodeLoadController m_loadController;
    std::unique_ptr<PowerSaverStateMonitor> m_powerSaverMonitor;
    std::optional<Context> m_displayedContext;
    std::optional<Context> m_pendingContext;
    ImageAsyncTicket m_generation;
    QTimer m_debounceTimer;
    QTimer m_neutralTimer;
    QElapsedTimer m_monotonicClock;
    quint64 m_pendingGeneration = 0;
    PredecodeMomentumState m_momentumState;
    bool m_powerSaverEnabled = false;
};
}

#endif
